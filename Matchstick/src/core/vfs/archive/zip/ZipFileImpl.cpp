#include "StdAfx.h"

#include "ZipFileImpl.hpp"

#include <algorithm>

#include "../../../common/MGDFLoggerImpl.hpp"
#include "../../../common/MGDFResources.hpp"

// std min&max are used instead of the macros
#ifdef min
#undef min
#undef max
#endif

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

ZipFileImplReader::ZipFileImplReader(ZipFileImpl* zip,
                                     const ZipFileHeader& header,
                                     const ZipFileData& data)
    : _zip(zip, true),
      _readPosition(data.readPosition),
      _data(data.data),
      _size(header.size) {}

ZipFileImplReader::~ZipFileImplReader() {
  free(_data);
  _zip->_reader = nullptr;
}

UINT32 ZipFileImplReader::Read(void* buffer, UINT32 length) {
  if (!buffer) return 0;

  UINT32 maxRead =
      std::min<UINT32>(length, static_cast<UINT32>(_size - _readPosition));
  memcpy(buffer, _data + _readPosition, maxRead);
  _readPosition += maxRead;
  return maxRead;
}

void ZipFileImplReader::SetPosition(INT64 pos) { _readPosition = pos; }

INT64 ZipFileImplReader::GetPosition() const { return _readPosition; }

bool ZipFileImplReader::EndOfFile() const { return _readPosition >= _size; }

ZipFileImpl::~ZipFileImpl() { _ASSERTE(!_reader); }

HRESULT ZipFileImpl::Open(IFileReader** reader) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_reader) {
    return ERROR_ACCESS_DENIED;
  }

  // if the entry is already in the map then the file is already open
  // if its not in the hashmap then open it
  unzGoToFilePos(_zip, &_header.filePosition);

  if (_header.size > UINT32_MAX) {
    std::string message = "Archive files cannot be over 4GB in size";
    LOG("Archive files cannot be over 4GB in size "
            << Resources::ToString(_header.name),
        LOG_ERROR);
    return E_FAIL;
  }

  ZipFileData data;
  data.readPosition = 0;
  data.data = (char*)malloc(static_cast<UINT32>(_header.size));

  if (unzOpenCurrentFile(_zip) != UNZ_OK) {
    goto cleanup;
  }
  if (unzReadCurrentFile(_zip, data.data, static_cast<UINT32>(_header.size)) <
      0) {
    goto cleanup;
  }
  if (unzCloseCurrentFile(_zip) == UNZ_CRCERROR) {
    goto cleanup;
  }

  _reader = new ZipFileImplReader(this, _header, data);
  *reader = _reader;
  return S_OK;

cleanup:
  LOG("Invalid archive file " << Resources::ToString(_header.name), LOG_ERROR);
  free(data.data);
  data.data = nullptr;
  return E_FAIL;
}

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF