#include "StdAfx.h"

#include "MGDFVersionHelper.hpp"

#include <sstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

std::string VersionHelper::Format(const MGDFVersion &version) {
  std::ostringstream ss;

  ss << version.Major << '.' << version.Minor;

  if (version.Build >= 0) {
    ss << '.' << version.Build;
  }
  if (version.Revision >= 0) {
    ss << '.' << version.Revision;
  }
  return ss.str();
}

MGDFVersion VersionHelper::Create(const std::string &version) {
  MGDFVersion result;
  result.Major = result.Minor = result.Build = result.Revision = -1;

  std::string copy(version);

  char *context = 0;

  char *ptr = strtok_s(copy.data(), ".", &context);
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

  return result;
}

INT32 VersionHelper::Compare(const MGDFVersion &a, const MGDFVersion &b) {
  if (a.Major != b.Major) {
    return a.Major > b.Major ? 1 : -1;
  } else if (a.Minor != b.Minor) {
    return a.Minor > b.Minor ? 1 : -1;
  } else if (a.Build != b.Build) {
    return a.Build > b.Build ? 1 : -1;
  } else if (a.Revision != b.Revision) {
    return a.Revision > b.Revision ? 1 : -1;
  } else {
    return 0;
  }
}

}  // namespace core
}  // namespace MGDF