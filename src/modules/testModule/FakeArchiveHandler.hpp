#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <vector>

namespace MGDF {
namespace Test {

class FakeArchiveHandler : public ComBase<IMGDFArchiveHandler> {
 public:
  FakeArchiveHandler(IMGDFLogger *logger);
  ~FakeArchiveHandler();
  BOOL __stdcall TestPathSegment(const MGDFArchivePathSegment *segment) final;
  BOOL __stdcall MapArchive(const wchar_t *rootPath, const wchar_t *archivePath,
                            const MGDFArchivePathSegment *logicalPathSegments,
                            UINT64 logicalPathSegmentCount,
                            IMGDFArchiveHandler **handlers, UINT64 handlerCount,
                            IMGDFReadOnlyFile *parent,
                            IMGDFReadOnlyFile **child) final;

 private:
  IMGDFLogger *_logger;
};

}  // namespace Test
}  // namespace MGDF