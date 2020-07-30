#include "StdAfx.h"

#include "MGDFDefaultFileReader.hpp"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

DefaultFileReader::DefaultFileReader(std::shared_ptr<std::ifstream> stream,
                                     std::function<void()> cleanup)
    : _stream(stream), _fileSize(stream->tellg()), _cleanup(cleanup) {
  _stream->seekg(0, std::ios::beg);
}

DefaultFileReader::~DefaultFileReader() {
  _stream->close();
  _cleanup();
}

UINT32 DefaultFileReader::Read(void *buffer, UINT32 length) {
  if (buffer && length) {
    const auto oldPosition = _stream->tellg();
    _stream->read(static_cast<char *>(buffer), length);
    const auto newPosition = _stream->tellg();
    return static_cast<UINT32>((_stream->eof() ? _fileSize : newPosition) -
                               oldPosition);
  }
  return 0;
}

void DefaultFileReader::SetPosition(INT64 pos) { _stream->seekg(pos); }

INT64 DefaultFileReader::GetPosition() { return _stream->tellg(); }

BOOL DefaultFileReader::EndOfFile() { return _stream->eof(); }

INT64 DefaultFileReader::GetSize() { return _fileSize; }

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
