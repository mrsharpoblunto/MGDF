#include "StdAfx.h"

#include "ZipFileImpl.hpp"

#include "../../../common/MGDFLoggerImpl.hpp"
#include "../../../common/MGDFResources.hpp"

// std min&max are used instead of the macros
#ifdef min
#undef min
#undef max
#endif

constexpr UINT32 BUFFER_SIZE = 1024 * 1024;

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

  const UINT32 maxRead =
      std::min<UINT32>(length, static_cast<UINT32>(_size - _readPosition));
  memcpy(buffer, _data + _readPosition, maxRead);
  _readPosition += maxRead;
  return maxRead;
}

void ZipFileImplReader::SetPosition(INT64 pos) { _readPosition = pos; }

INT64 ZipFileImplReader::GetPosition() { return _readPosition; }

BOOL ZipFileImplReader::EndOfFile() { return _readPosition >= _size; }

ZipFileImpl::~ZipFileImpl() { _ASSERTE(!_reader); }

HRESULT ZipFileImpl::CopyTo(IMGDFWriteableFile* destination) {
  std::vector<char> buffer(BUFFER_SIZE);

  ComObject<IMGDFFileReader> reader;
  if (S_OK != Open(reader.Assign())) {
    return E_FAIL;
  }
  ComObject<IMGDFFileWriter> writer;
  if (S_OK != destination->OpenWrite(writer.Assign())) {
    return E_FAIL;
  }

  auto position = reader->GetPosition();
  while (position < reader->GetSize()) {
    const auto read = reader->Read(buffer.data(), BUFFER_SIZE);
    position += read;
    if (writer->Write(buffer.data(), read) != read) {
      return E_FAIL;
    }
  }
  return S_OK;
}

BOOL ZipFileImpl::IsOpen() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _reader != nullptr;
}

HRESULT ZipFileImpl::GetAllChildren(IMGDFReadOnlyFile** buffer,
                                    UINT64* length) {
  std::ignore = buffer;
  *length = 0;
  return S_OK;
}

BOOL ZipFileImpl::GetChild(const wchar_t* name, IMGDFReadOnlyFile** child) {
  std::ignore = name;
  std::ignore = child;
  return FALSE;
}

HRESULT ZipFileImpl::Open(IMGDFFileReader** reader) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_reader) {
    return E_ACCESSDENIED;
  }

  // if the entry is already in the map then the file is already open
  // if its not in the hashmap then open it
  unzGoToFilePos(_context->Zip, &_header.filePosition);

  if (_header.size > UINT32_MAX) {
    std::string message = "Archive files cannot be over 4GB in size";
    LOG("Archive files cannot be over 4GB in size "
            << Resources::ToString(_header.name),
        MGDF_LOG_ERROR);
    return E_FAIL;
  }

  ZipFileData data;
  data.readPosition = 0;
  data.data = static_cast<char*>(malloc(static_cast<UINT32>(_header.size)));

  if (unzOpenCurrentFile(_context->Zip) == UNZ_OK &&
      unzReadCurrentFile(_context->Zip, data.data,
                         static_cast<UINT32>(_header.size)) >= 0 &&
      unzCloseCurrentFile(_context->Zip) != UNZ_CRCERROR) {
    _reader = new ZipFileImplReader(this, _header, data);
    *reader = _reader;
    return S_OK;
  } else {
    LOG("Invalid archive file " << Resources::ToString(_header.name),
        MGDF_LOG_ERROR);
    free(data.data);
    data.data = nullptr;
    return E_FAIL;
  }
}

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF