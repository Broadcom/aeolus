#include "CRC.h"

namespace BT_NAMESPACE {

#ifdef HASH_ARRAY_2
  static const UINT32 kHash2Size = 1 << 10;
  #ifdef HASH_ARRAY_3
    static const UINT32 kNumHashDirectBytes = 0;
    static const UINT32 kNumHashBytes = 4;
    static const UINT32 kHash3Size = 1 << 18;
    #ifdef HASH_BIG
    static const UINT32 kHashSize = 1 << 23;
    #else
    static const UINT32 kHashSize = 1 << 20;
    #endif
  #else
    static const UINT32 kNumHashDirectBytes = 3;
    static const UINT32 kNumHashBytes = 3;
    static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#else
  #ifdef HASH_ZIP 
    static const UINT32 kNumHashDirectBytes = 0;
    static const UINT32 kNumHashBytes = 3;
    static const UINT32 kHashSize = 1 << 16;
  #else
    static const UINT32 kNumHashDirectBytes = 2;
    static const UINT32 kNumHashBytes = 2;
    static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#endif

CInTree::CInTree():
  #ifdef HASH_ARRAY_2
  m_Hash2(0),
  #ifdef HASH_ARRAY_3
  m_Hash3(0),
  #endif
  #endif
  m_Hash(0),
  m_Base(0),
  m_CutValue(0xFF)
{
}

void CInTree::FreeMemory()
{
  if (m_Base != 0)
	delete [] m_Base;
  if (m_Hash != 0)
	delete [] m_Hash;
  m_Base = 0;
  m_Hash = 0;
  CIn::Free();
}

CInTree::~CInTree()
{ 
  FreeMemory();
}

HRESULT CInTree::Create(UINT32 aSizeHistory, UINT32 aKeepAddBufferBefore, 
    UINT32 aMatchMaxLen, UINT32 aKeepAddBufferAfter, UINT32 aSizeReserv)
{
  FreeMemory();
    CIn::Create(aSizeHistory + aKeepAddBufferBefore, 
      aMatchMaxLen + aKeepAddBufferAfter, aSizeReserv);
    
    if (m_BlockSize + 256 > kMaxValForNormalize)
      return E_INVALIDARG;
    
    m_HistorySize = aSizeHistory;
    m_MatchMaxLen = aMatchMaxLen;
    
    
    UINT32 aSize = kHashSize;
    #ifdef HASH_ARRAY_2
    aSize += kHash2Size;
    #ifdef HASH_ARRAY_3
    aSize += kHash3Size;
    #endif
    #endif
    
    m_Base = new CPair[m_BlockSize + 1];
    if (m_Base == 0)
	return E_OUTOFMEMORY;
    m_Hash = new CIndex[aSize + 1];
    if (m_Hash == 0)
	return E_OUTOFMEMORY;
    
    #ifdef HASH_ARRAY_2
    m_Hash2 = &m_Hash[kHashSize]; 
    #ifdef HASH_ARRAY_3
    m_Hash3 = &m_Hash2[kHash2Size]; 
    #endif
    #endif
    return S_OK;
}

static const UINT32 kEmptyHashValue = 0;

HRESULT CInTree::Init(ISequentialInStream *aStream)
{
  RETURN_IF_NOT_S_OK(CIn::Init(aStream));
  unsigned i;
  for(i = 0; i < kHashSize; i++)
    m_Hash[i] = kEmptyHashValue;

  #ifdef HASH_ARRAY_2
  for(i = 0; i < kHash2Size; i++)
    m_Hash2[i] = kEmptyHashValue;
  #ifdef HASH_ARRAY_3
  for(i = 0; i < kHash3Size; i++)
    m_Hash3[i] = kEmptyHashValue;
  #endif
  #endif

  m_Son = m_Base;

  ReduceOffsets(0 - 1);
  return S_OK;
}


#ifdef HASH_ARRAY_2
#ifdef HASH_ARRAY_3
inline UINT32 Hash(const BYTE *aPointer, UINT32 &aHash2Value, UINT32 &aHash3Value)
{
  UINT32 aTemp = CCRC::m_Table[aPointer[0]] ^ aPointer[1];
  aHash2Value = aTemp & (kHash2Size - 1);
  aHash3Value = (aTemp ^ (UINT32(aPointer[2]) << 8)) & (kHash3Size - 1);
  return (aTemp ^ (UINT32(aPointer[2]) << 8) ^ (CCRC::m_Table[aPointer[3]] << 5)) & 
      (kHashSize - 1);
}
#else // no HASH_ARRAY_3
inline UINT32 Hash(const BYTE *aPointer, UINT32 &aHash2Value)
{
  aHash2Value = (CCRC::m_Table[aPointer[0]] ^ aPointer[1]) & (kHash2Size - 1);
  return (*((const UINT32 *)aPointer)) & 0xFFFFFF;
}
#endif // HASH_ARRAY_3
#else // no HASH_ARRAY_2
#ifdef HASH_ZIP 
inline UINT32 Hash(const BYTE *aPointer)
{
  return ((UINT32(aPointer[0]) << 8) ^ 
      CCRC::m_Table[aPointer[1]] ^ aPointer[2]) & (kHashSize - 1);
}
#else // no HASH_ZIP 
inline UINT32 Hash(const BYTE *aPointer)
{
  return aPointer[0] ^ (UINT32(aPointer[1]) << 8);
}
#endif // HASH_ZIP
#endif // HASH_ARRAY_2

UINT32 CInTree::GetLongestMatch(UINT32 *aDistances)
{
  UINT32 aCurrentLimit;
  if (m_Pos + m_MatchMaxLen <= m_StreamPos)
    aCurrentLimit = m_MatchMaxLen;
  else
  {
    aCurrentLimit = m_StreamPos - m_Pos;
    if(aCurrentLimit < kNumHashBytes)
      return 0; 
  }

  UINT32 aMatchMinPos = (m_Pos > m_HistorySize) ? (m_Pos - m_HistorySize) : 1;
  BYTE *aCur = m_Buffer + m_Pos;
  
  
  UINT32 aMatchHashLenMax = 0;

  #ifdef HASH_ARRAY_2
  UINT32 aHash2Value;
  #ifdef HASH_ARRAY_3
  UINT32 aHash3Value;
  UINT32 aHashValue = Hash(aCur, aHash2Value, aHash3Value);
  #else
  UINT32 aHashValue = Hash(aCur, aHash2Value);
  #endif
  #else
  UINT32 aHashValue = Hash(aCur);
  #endif

  UINT32 aCurMatch = m_Hash[aHashValue];
  #ifdef HASH_ARRAY_2
  UINT32 aCurMatch2 = m_Hash2[aHash2Value];
  #ifdef HASH_ARRAY_3
  UINT32 aCurMatch3 = m_Hash3[aHash3Value];
  #endif
  m_Hash2[aHash2Value] = m_Pos;
  bool aMatchLen2Exist = false;
  UINT32 aLen2Distance = 0;
  if(aCurMatch2 >= aMatchMinPos)
  {
    if (m_Buffer[aCurMatch2] == aCur[0])
    {
      aLen2Distance = m_Pos - aCurMatch2 - 1;
      aMatchHashLenMax = 2;
      aMatchLen2Exist = true;
    }
  }

  #ifdef HASH_ARRAY_3
  m_Hash3[aHash3Value] = m_Pos;
  UINT32 aMatchLen3Exist = false;
  UINT32 aLen3Distance = 0;
  if(aCurMatch3 >= aMatchMinPos)
  {
    if (m_Buffer[aCurMatch3] == aCur[0])
    {
      aLen3Distance = m_Pos - aCurMatch3 - 1;
      aMatchHashLenMax = 3;
      aMatchLen3Exist = true;
      if (aMatchLen2Exist)
      {
        if (aLen3Distance < aLen2Distance)
          aLen2Distance = aLen3Distance;
      }
      else
      {
        aLen2Distance = aLen3Distance;
        aMatchLen2Exist = true;
      }
    }
  }
  #endif
  #endif

  m_Hash[aHashValue] = m_Pos;
  if(aCurMatch < aMatchMinPos)
  {
    m_Son[m_Pos].Left = kEmptyHashValue; 
    m_Son[m_Pos].Right = kEmptyHashValue; 

    #ifdef HASH_ARRAY_2
    aDistances[2] = aLen2Distance;
    #ifdef HASH_ARRAY_3
    aDistances[3] = aLen3Distance;
    #endif
    #endif

    return aMatchHashLenMax;
  }
  CIndex *aPtrLeft = &m_Son[m_Pos].Right;
  CIndex *aPtrRight = &m_Son[m_Pos].Left;

  UINT32 aMax, aMinSameLeft, aMinSameRight, aMinSame;
  aMax = aMinSameLeft = aMinSameRight = aMinSame = kNumHashDirectBytes;

  #ifdef HASH_ARRAY_2
  #ifndef HASH_ARRAY_3
    if (aMatchLen2Exist)
      aDistances[2] = aLen2Distance;
    else
      if (kNumHashDirectBytes >= 2)
        aDistances[2] = m_Pos - aCurMatch - 1;
  #endif
  #endif

  aDistances[aMax] = m_Pos - aCurMatch - 1;
  
  for(UINT32 aCount = m_CutValue; aCount > 0; aCount--)
  {
    BYTE *pby1 = m_Buffer + aCurMatch;
    UINT32 aCurrentLen;
    for(aCurrentLen = aMinSame; aCurrentLen < aCurrentLimit; aCurrentLen++/*, dwComps++*/)
      if (pby1[aCurrentLen] != aCur[aCurrentLen])
        break;
    while (aCurrentLen > aMax)
      aDistances[++aMax] = m_Pos - aCurMatch - 1;
    if (aCurrentLen != aCurrentLimit)
    {
      if (pby1[aCurrentLen] < aCur[aCurrentLen])
      {
        *aPtrRight = aCurMatch;
        aPtrRight = &m_Son[aCurMatch].Right;
        aCurMatch = m_Son[aCurMatch].Right;
        if(aCurrentLen > aMinSameLeft)
        {
          aMinSameLeft = aCurrentLen;
          aMinSame = MyMin(aMinSameLeft, aMinSameRight);
        }
      }
      else
      {
        *aPtrLeft = aCurMatch;
        aPtrLeft = &m_Son[aCurMatch].Left;
        aCurMatch = m_Son[aCurMatch].Left;
        if(aCurrentLen > aMinSameRight)
        {
          aMinSameRight = aCurrentLen;
          aMinSame = MyMin(aMinSameLeft, aMinSameRight);
        }
      }
    }
    else
    {
      if(aCurrentLen < m_MatchMaxLen)
      {
        *aPtrLeft = aCurMatch;
        aPtrLeft = &m_Son[aCurMatch].Left;
        aCurMatch = m_Son[aCurMatch].Left;
        if(aCurrentLen > aMinSameRight)
        {
          aMinSameRight = aCurrentLen;
          aMinSame = MyMin(aMinSameLeft, aMinSameRight);
        }
      }
      else
      {
        *aPtrLeft = m_Son[aCurMatch].Right;
        *aPtrRight = m_Son[aCurMatch].Left;

        #ifdef HASH_ARRAY_2
        if (aMatchLen2Exist && aLen2Distance < aDistances[2])
          aDistances[2] = aLen2Distance;
        #ifdef HASH_ARRAY_3
        if (aMatchLen3Exist && aLen3Distance < aDistances[3])
          aDistances[3] = aLen3Distance;
        #endif
        #endif

        return aMax;
      }
    }
    if(aCurMatch < aMatchMinPos)
      break;
  }
  *aPtrLeft = kEmptyHashValue;
  *aPtrRight = kEmptyHashValue;
  #ifdef HASH_ARRAY_2
  if (aMatchLen2Exist)
  {
    if (aMax < 2)
    {
      aDistances[2] = aLen2Distance;
      aMax = 2;
    }
    else if (aLen2Distance < aDistances[2])
      aDistances[2] = aLen2Distance;
  }
  #ifdef HASH_ARRAY_3
  if (aMatchLen3Exist)
  {
    if (aMax < 3)
    {
      aDistances[3] = aLen3Distance;
      aMax = 3;
    }
    else if (aLen3Distance < aDistances[3])
      aDistances[3] = aLen3Distance;
  }
  #endif
  #endif
  return aMax;
}

void CInTree::DummyLongestMatch()
{
  UINT32 aCurrentLimit;
  if (m_Pos + m_MatchMaxLen <= m_StreamPos)
    aCurrentLimit = m_MatchMaxLen;
  else
  {
    aCurrentLimit = m_StreamPos - m_Pos;
    if(aCurrentLimit < kNumHashBytes)
      return; 
  }
  UINT32 aMatchMinPos = (m_Pos > m_HistorySize) ? (m_Pos - m_HistorySize) : 1;
  BYTE *aCur = m_Buffer + m_Pos;
  
  #ifdef HASH_ARRAY_2
  UINT32 aHash2Value;
  #ifdef HASH_ARRAY_3
  UINT32 aHash3Value;
  UINT32 aHashValue = Hash(aCur, aHash2Value, aHash3Value);
  m_Hash3[aHash3Value] = m_Pos;
  #else
  UINT32 aHashValue = Hash(aCur, aHash2Value);
  #endif
  m_Hash2[aHash2Value] = m_Pos;
  #else
  UINT32 aHashValue = Hash(aCur);
  #endif

  UINT32 aCurMatch = m_Hash[aHashValue];
  m_Hash[aHashValue] = m_Pos;
  if(aCurMatch < aMatchMinPos)
  {
    m_Son[m_Pos].Left = kEmptyHashValue; 
    m_Son[m_Pos].Right = kEmptyHashValue; 
    return;
  }
  CIndex *aPtrLeft = &m_Son[m_Pos].Right;
  CIndex *aPtrRight = &m_Son[m_Pos].Left;

  UINT32 aMax, aMinSameLeft, aMinSameRight, aMinSame;
  aMax = aMinSameLeft = aMinSameRight = aMinSame = kNumHashDirectBytes;
  for(UINT32 aCount = m_CutValue; aCount > 0; aCount--)
  {
    BYTE *pby1 = m_Buffer + aCurMatch;
    // CIndex aLeft = m_Son[aCurMatch].Left; // it's prefetch
    UINT32 aCurrentLen;
    for(aCurrentLen = aMinSame; aCurrentLen < aCurrentLimit; aCurrentLen++/*, dwComps++*/)
      if (pby1[aCurrentLen] != aCur[aCurrentLen])
        break;
    if (aCurrentLen != aCurrentLimit)
    {
      if (pby1[aCurrentLen] < aCur[aCurrentLen])
      {
        *aPtrRight = aCurMatch;
        aPtrRight = &m_Son[aCurMatch].Right;
        aCurMatch = m_Son[aCurMatch].Right;
        if(aCurrentLen > aMinSameLeft)
        {
          aMinSameLeft = aCurrentLen;
          aMinSame = MyMin(aMinSameLeft, aMinSameRight);
        }
      }
      else 
      {
        *aPtrLeft = aCurMatch;
        aPtrLeft = &m_Son[aCurMatch].Left;
        aCurMatch = m_Son[aCurMatch].Left;
        // aCurMatch = aLeft;
        if(aCurrentLen > aMinSameRight)
        {
          aMinSameRight = aCurrentLen;
          aMinSame = MyMin(aMinSameLeft, aMinSameRight);
        }
      }
    }
    else
    {
      if(aCurrentLen < m_MatchMaxLen)
      {
        *aPtrLeft = aCurMatch;
        aPtrLeft = &m_Son[aCurMatch].Left;
        aCurMatch = m_Son[aCurMatch].Left;
        if(aCurrentLen > aMinSameRight)
        {
          aMinSameRight = aCurrentLen;
          aMinSame = MyMin(aMinSameLeft, aMinSameRight);
        }
      }
      else
      {
        *aPtrLeft = m_Son[aCurMatch].Right;
        *aPtrRight = m_Son[aCurMatch].Left;
        return;
      }
    }
    if(aCurMatch < aMatchMinPos)
      break;
  }
  *aPtrLeft = kEmptyHashValue;
  *aPtrRight = kEmptyHashValue;
}

void CInTree::AfterMoveBlock()
{
  UINT32 aNumBytesToMove = m_HistorySize * sizeof(CPair);
  UINT32 aSpecOffset = ((m_Son + m_Pos) - m_Base) - m_HistorySize;
  memmove(m_Base, m_Base + aSpecOffset, aNumBytesToMove);
  m_Son -= aSpecOffset;
}

void CInTree::NormalizeLinks(CIndex *anArray, UINT32 aNumItems, INT32 aSubValue)
{
  for (UINT32 i = 0; i < aNumItems; i++)
  {
    UINT32 aValue = anArray[i];
    if (aValue <= aSubValue)
      aValue = kEmptyHashValue;
    else
      aValue -= aSubValue;
    anArray[i] = aValue;
  }
}

void CInTree::Normalize()
{
  UINT32 aStartItem = m_Pos - m_HistorySize;
  INT32 aSubValue = aStartItem - 1;
  NormalizeLinks((CIndex *)(m_Son + aStartItem), m_HistorySize * 2, aSubValue);
  NormalizeLinks(m_Hash, kHashSize, aSubValue);

  #ifdef HASH_ARRAY_2
  NormalizeLinks(m_Hash2, kHash2Size, aSubValue);
  #ifdef HASH_ARRAY_3
  NormalizeLinks(m_Hash3, kHash3Size, aSubValue);
  #endif
  #endif

  ReduceOffsets(aSubValue);
}
 
}
