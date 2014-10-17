#include "Portable.h"
#include "WindowIn.h"

namespace NStream {
namespace NWindow {

CIn::CIn():
  m_BufferBase(0)
{}

void CIn::Free()
{
  delete []m_BufferBase;
  m_BufferBase = 0;
}

void CIn::Create(UINT32 aKeepSizeBefore, UINT32 aKeepSizeAfter, UINT32 aKeepSizeReserv)
{
  m_KeepSizeBefore = aKeepSizeBefore;
  m_KeepSizeAfter = aKeepSizeAfter;
  m_KeepSizeReserv = aKeepSizeReserv;
  m_BlockSize = aKeepSizeBefore + aKeepSizeAfter + aKeepSizeReserv;
  Free();
  m_BufferBase = new BYTE[m_BlockSize];
  m_PointerToLastSafePosition = m_BufferBase + m_BlockSize - aKeepSizeAfter;
}

CIn::~CIn()
{
  Free();
}

HRESULT CIn::Init(ISequentialInStream *aStream)
{
  m_Stream = aStream;
  m_Buffer = m_BufferBase;
  m_Pos = 0;
  m_StreamPos = 0;
  m_StreamEndWasReached = false;
  return ReadBlock();
}

///////////////////////////////////////////
// ReadBlock

// In State:
//   (m_Buffer + m_StreamPos) <= (m_BufferBase + m_BlockSize)
// Out State:
//   m_PosLimit <= m_BlockSize - m_KeepSizeAfter;
//   if(m_StreamEndWasReached == false):
//     m_StreamPos >= m_Pos + m_KeepSizeAfter
//     m_PosLimit = m_StreamPos - m_KeepSizeAfter;
//   else
//          
  
HRESULT CIn::ReadBlock()
{
  if(m_StreamEndWasReached)
    return S_OK;
  while(true)
  {
    UINT32 aSize = (m_BufferBase + m_BlockSize) - (m_Buffer + m_StreamPos);
    if(aSize == 0)
      return S_OK;
    UINT32 aNumReadBytes;
    RETURN_IF_NOT_S_OK(m_Stream->Read(m_Buffer + m_StreamPos,
        aSize, &aNumReadBytes));
    if(aNumReadBytes == 0)
    {
      m_PosLimit = m_StreamPos;
      const BYTE *aPointerToPostion = m_Buffer + m_PosLimit;
      if(aPointerToPostion > m_PointerToLastSafePosition)
        m_PosLimit = m_PointerToLastSafePosition - m_Buffer;
      m_StreamEndWasReached = true;
      return S_OK;
    }
    m_StreamPos += aNumReadBytes;
    if(m_StreamPos >= m_Pos + m_KeepSizeAfter)
    {
      m_PosLimit = m_StreamPos - m_KeepSizeAfter;
      return S_OK;
    }
  }
}

void CIn::MoveBlock()
{
  BeforeMoveBlock();
  UINT32 anOffset = (m_Buffer + m_Pos - m_KeepSizeBefore) - m_BufferBase;
  UINT32 aNumBytes = (m_Buffer + m_StreamPos) -  (m_BufferBase + anOffset);
  memmove(m_BufferBase, m_BufferBase + anOffset, aNumBytes);
  m_Buffer -= anOffset;
  AfterMoveBlock();
}


}}
