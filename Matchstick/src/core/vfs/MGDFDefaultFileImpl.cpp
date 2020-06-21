#include "StdAfx.h"

#include "MGDFDefaultFileImpl.hpp"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

DefaultFileReader::DefaultFileReader(DefaultFileImpl *parent,
                                     std::shared_ptr<std::ifstream> stream)
    : _parent(parent, true), _stream(stream), _fileSize(stream->tellg()) {
  _stream->seekg(0, std::ios::beg);
}

DefaultFileReader::~DefaultFileReader() {
  _stream->close();
  const std::lock_guard<std::mutex> lock(_parent->_mutex);
  _parent->_reader = nullptr;
}

UINT32 DefaultFileReader::Read(void *buffer, UINT32 length) {
  if (buffer && length) {
    const auto oldPosition = _stream->tellg();
    _stream->read(static_cast<char *>(buffer), length);
    const auto newPosition = _stream->tellg();
    return static_cast<UINT32>(newPosition - oldPosition);
  }
  return 0;
}

void DefaultFileReader::SetPosition(INT64 pos) { _stream->seekg(pos); }

INT64 DefaultFileReader::GetPosition() { return _stream->tellg(); }

BOOL DefaultFileReader::EndOfFile() { return _stream->eof(); }

INT64 DefaultFileReader::GetSize() { return _fileSize; }

DefaultFileImpl::DefaultFileImpl(const std::wstring &name,
                                 const std::wstring &physicalPath,
                                 IMGDFFile *parent)
    : FileBaseImpl(parent),
      _name(name),
      _reader(nullptr),
      _path(physicalPath) {}

DefaultFileImpl::~DefaultFileImpl() { _ASSERTE(!_reader); }

HRESULT DefaultFileImpl::Open(IMGDFFileReader **reader) {
  std::lock_guard<std::mutex> lock(_mutex);

  if (!_reader) {
    auto fileStream = std::make_shared<std::ifstream>(
        _path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    if (fileStream && !fileStream->bad() && fileStream->is_open()) {
      _reader = new DefaultFileReader(this, fileStream);
      *reader = _reader;
      return S_OK;
    } else {
      LOG("Unable to open file stream for " << Resources::ToString(_path)
                                            << " - " << GetLastError(),
          MGDF_LOG_ERROR);
      return E_FAIL;
    }
  }
  LOG("File " << Resources::ToString(_path) << " currently in use", MGDF_LOG_ERROR);
  return E_ACCESSDENIED;
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
