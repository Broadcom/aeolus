#include "Portable.h"
#include "IInOutStreams.h"

HRESULT ISequentialInStream::Read(void *aData, INT aSize, INT* aProcessedSize) {
	if (aSize > size)
		aSize = size;
	*aProcessedSize = aSize;
	memcpy(aData, data, aSize);
	size -= aSize;
	data += aSize;
	return S_OK;
}

HRESULT ISequentialOutStream::Write(const void *aData, INT aSize, INT* aProcessedSize) {
	if (aSize > size) {
		overflow = true;
		aSize = size;
	}
	*aProcessedSize = aSize;
	memcpy(data, aData, aSize);
	size -= aSize;
	data += aSize;
	total += aSize;
	return S_OK;
}
