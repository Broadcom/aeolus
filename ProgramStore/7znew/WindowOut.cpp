#include "WindowOut.h"

namespace NStream {
namespace NWindow {

void COut::Create(INT aKeepSizeBefore, INT aKeepSizeAfter, INT aKeepSizeReserv)
{
  m_Pos = 0;
  m_PosLimit = aKeepSizeReserv + aKeepSizeBefore;
  m_KeepSizeBefore = aKeepSizeBefore;
  m_KeepSizeAfter = aKeepSizeAfter;
  m_KeepSizeReserv = aKeepSizeReserv;
  m_StreamPos = 0;
  m_MoveFrom = m_KeepSizeReserv;
  m_WindowSize = aKeepSizeBefore;
  INT aBlockSize = m_KeepSizeBefore + m_KeepSizeAfter + m_KeepSizeReserv;
  delete []m_Buffer;
  m_Buffer = new BYTE[aBlockSize];
}

COut::~COut()
{
  delete []m_Buffer;
}

void COut::SetWindowSize(INT aWindowSize)
{
  m_WindowSize = aWindowSize;
  m_MoveFrom = m_KeepSizeReserv + m_KeepSizeBefore - aWindowSize;
}

void COut::Init(ISequentialOutStream *aStream, bool aSolid)
{
  m_Stream = aStream;

  if(aSolid)
    m_StreamPos = m_Pos;
  else
  {
    m_Pos = 0;
    m_PosLimit = m_KeepSizeReserv + m_KeepSizeBefore;
    m_StreamPos = 0;
  }
}

HRESULT COut::Flush()
{
  INT aSize = m_Pos - m_StreamPos;
  if(aSize == 0)
    return S_OK;
  INT aProcessedSize;
  HRESULT aResult = m_Stream->Write(m_Buffer + m_StreamPos, aSize, &aProcessedSize);
  if (aResult != S_OK)
    return aResult;
  if (aSize != aProcessedSize)
    return E_FAIL;
  m_StreamPos = m_Pos;
  return S_OK;
}

void COut::MoveBlockBackward()
{
  HRESULT aResult = Flush();
  if (aResult != S_OK)
    throw aResult;
  memmove(m_Buffer, m_Buffer + m_MoveFrom, m_WindowSize + m_KeepSizeAfter);
  m_Pos -= m_MoveFrom;
  m_StreamPos -= m_MoveFrom;
}

}}
