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
  std::lock_guard<std::mutex> lock(_parent->_mutex);
  _parent->_reader = nullptr;
}

UINT32 DefaultFileReader::Read(void *buffer, UINT32 length) {
  if (buffer && length) {
    std::ifstream::pos_type oldPosition = _stream->tellg();
    _stream->read((char *)buffer, length);
    std::ifstream::pos_type newPosition = _stream->tellg();
    return static_cast<UINT32>(newPosition - oldPosition);
  }
  return 0;
}

void DefaultFileReader::SetPosition(INT64 pos) { _stream->seekg(pos); }

INT64 DefaultFileReader::GetPosition() const { return _stream->tellg(); }

bool DefaultFileReader::EndOfFile() const { return _stream->eof(); }

INT64 DefaultFileReader::GetSize() const { return _fileSize; }

DefaultFileImpl::DefaultFileImpl(const std::wstring &name,
                                 const std::wstring &physicalPath,
                                 IFile *parent)
    : FileBaseImpl(parent),
      _name(name),
      _reader(nullptr),
      _path(physicalPath) {}

DefaultFileImpl::~DefaultFileImpl() { _ASSERTE(!_reader); }

HRESULT DefaultFileImpl::Open(IFileReader **reader) {
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
          LOG_ERROR);
      return ERROR_OPEN_FAILED;
    }
  }
  LOG("File " << Resources::ToString(_path) << " currently in use", LOG_ERROR);
  return ERROR_ACCESS_DENIED;
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
