#include "StdAfx.h"

#include "MGDFVersionHelper.hpp"

#include <sstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

std::string VersionHelper::Format(const Version *version) {
  std::ostringstream ss;

  ss << version->Major << '.' << version->Minor;

  if (version->Build >= 0) {
    ss << '.' << version->Build;
  }
  if (version->Revision >= 0) {
    ss << '.' << version->Revision;
  }
  return ss.str();
}

Version VersionHelper::Create(const std::string &version) {
  Version result;
  result.Major = result.Minor = result.Build = result.Revision = -1;

  char *copy = new char[version.size() + 1];
  strcpy_s(copy, version.size() + 1, version.c_str());

  char *context = 0;

  char *ptr = strtok_s(copy, ".", &context);
  _ASSERTE(ptr);
  result.Major = atoi(ptr);

  ptr = strtok_s(0, ".", &context);
  _ASSERTE(ptr);
  result.Minor = atoi(ptr);

  ptr = strtok_s(0, ".", &context);
  if (ptr) {
    result.Build = atoi(ptr);

    ptr = strtok_s(0, ".", &context);
    if (ptr) {
      result.Revision = atoi(ptr);
    }
  }

  delete[] copy;
  return result;
}

Version VersionHelper::Copy(const Version *version) {
  Version result;
  result.Major = version->Major;
  result.Minor = version->Minor;
  result.Build = version->Build;
  result.Revision = version->Revision;

  return result;
}

INT32 VersionHelper::Compare(const Version *a, const Version *b) {
  if (a->Major != b->Major) {
    return a->Major > b->Major ? 1 : -1;
  } else if (a->Minor != b->Minor) {
    return a->Minor > b->Minor ? 1 : -1;
  } else if (a->Build != b->Build) {
    return a->Build > b->Build ? 1 : -1;
  } else if (a->Revision != b->Revision) {
    return a->Revision > b->Revision ? 1 : -1;
  } else {
    return 0;
  }
}

}  // namespace core
}  // namespace MGDF