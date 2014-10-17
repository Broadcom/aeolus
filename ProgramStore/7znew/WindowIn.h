#ifndef __STREAM_WINDOWIN_H
#define __STREAM_WINDOWIN_H

#include "IInOutStreams.h"

#include <stdio.h>

namespace NStream {
namespace NWindow {

class CIn
{
  BYTE  *m_BufferBase; // pointer to buffer with data
  ISequentialInStream* m_Stream;
  INT m_PosLimit;  // offset (from m_Buffer) of first byte when new block reading must be done
  bool m_StreamEndWasReached; // if (true) then m_StreamPos shows real end of stream

  const BYTE *m_PointerToLastSafePosition;

protected:
  BYTE  *m_Buffer;   // Pointer to virtual Buffer begin
  INT m_BlockSize;  // Size of Allocated memory block
  INT m_Pos;             // offset (from m_Buffer) of curent byte
  INT m_KeepSizeBefore;  // how many BYTEs must be kept in buffer before m_Pos
  INT m_KeepSizeAfter;   // how many BYTEs must be kept buffer after m_Pos
  INT m_KeepSizeReserv;  // how many BYTEs must be kept as reserv
  INT m_StreamPos;   // offset (from m_Buffer) of first not read byte from Stream

  virtual void BeforeMoveBlock() {};
  virtual void AfterMoveBlock() {};
  void MoveBlock();
  virtual HRESULT ReadBlock();
  void Free();
public:
  CIn();
  void Create(INT aKeepSizeBefore, INT aKeepSizeAfter, 
      INT aKeepSizeReserv = (1<<17));
  virtual ~CIn();

  HRESULT Init(ISequentialInStream *aStream);

  BYTE *GetBuffer() const { return m_Buffer; }

  const BYTE *GetPointerToCurrentPos() const { return m_Buffer + m_Pos; }

  HRESULT MovePos()
  {
    m_Pos++;
    if (m_Pos > m_PosLimit)
    {
      const BYTE *aPointerToPostion = m_Buffer + m_Pos;
      if(aPointerToPostion > m_PointerToLastSafePosition)
        MoveBlock();
      return ReadBlock();
    }
    else
      return S_OK;
  }
  // BYTE GetCurrentByte()const;
  BYTE GetIndexByte(INT anIndex)const
    {  return m_Buffer[m_Pos + anIndex]; }

  // INT GetCurPos()const { return m_Pos;};
  // BYTE *GetBufferBeg()const { return m_Buffer;};

  // aIndex + aLimit have not to exceed m_KeepSizeAfter;
  INT GetMatchLen(INT aIndex, INT aBack, INT aLimit) const
  {  
    if(m_StreamEndWasReached)
      if ((m_Pos + aIndex) + aLimit > m_StreamPos)
        aLimit = m_StreamPos - (m_Pos + aIndex);
      aBack++;
      BYTE *pby = m_Buffer + m_Pos + aIndex;
      INT i;
      for(i = 0; i < aLimit && pby[i] == pby[i - aBack]; i++);
      return i;
  }

  INT GetNumAvailableBytes() const { return m_StreamPos - m_Pos; }

  void ReduceOffsets(INT aSubValue)
  {
    m_Buffer += aSubValue;
    m_PosLimit -= aSubValue;
    m_Pos -= aSubValue;
    m_StreamPos -= aSubValue;
  }

};

}}

#endif
