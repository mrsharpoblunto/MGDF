#pragma once

#include <fstream>
#include <functional>

#include "MGDF/ComObject.hpp"
#include "MGDF/MGDF.h"

namespace MGDF {
namespace core {
namespace vfs {

class ReadableFileBaseImpl;

class DefaultFileReader : public ComBase<IMGDFFileReader> {
 public:
  DefaultFileReader(std::shared_ptr<std::ifstream> stream,
                    std::function<void()> cleanup);
  virtual ~DefaultFileReader();
  UINT32 __stdcall Read(void *buffer, UINT32 length) final;
  void __stdcall SetPosition(INT64 pos) final;
  INT64 __stdcall GetPosition() final;
  BOOL __stdcall EndOfFile() final;
  INT64 __stdcall GetSize() final;

 private:
  INT64 _fileSize;
  std::shared_ptr<std::ifstream> _stream;
  std::function<void()> _cleanup;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
