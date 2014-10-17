#include "OutByte.h"

namespace NStream {

COutByte::COutByte(INT aBufferSize):
  m_BufferSize(aBufferSize)
{
  m_Buffer = new BYTE[m_BufferSize];
}

COutByte::~COutByte()
{
  delete []m_Buffer;
}

void COutByte::Init(ISequentialOutStream *aStream)
{
  m_Stream = aStream;
  m_ProcessedSize = 0;
  m_Pos = 0;
}

HRESULT COutByte::Flush()
{
  if (m_Pos == 0)
    return S_OK;
  INT aProcessedSize;
  HRESULT aResult = m_Stream->Write(m_Buffer, m_Pos, &aProcessedSize);
  if (aResult != S_OK)
    return aResult;
  if (m_Pos != aProcessedSize)
    return E_FAIL;
  m_ProcessedSize += aProcessedSize;
  m_Pos = 0;
  return S_OK;
}

void COutByte::WriteBlock()
{
  HRESULT aResult = Flush();
  if (aResult != S_OK)
    throw aResult;
}

}
