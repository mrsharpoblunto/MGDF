#include "stdAfx.h"

#include "MGDFVersionInfo.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

constexpr char _mgdfVersion[] = "0.16.1";

const char *MGDFVersionInfo::MGDF_VERSION() { return _mgdfVersion; }

}  // namespace core
}  // namespace MGDF