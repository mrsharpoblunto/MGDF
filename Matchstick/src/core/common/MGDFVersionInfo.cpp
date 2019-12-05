#include "stdAfx.h"
#include "MGDFVersionInfo.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

const std::string MGDFVersionInfo::_mgdfVersion = "0.13.5";

}
}