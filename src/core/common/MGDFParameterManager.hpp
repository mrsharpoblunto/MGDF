#pragma once

#include <map>
#include <string>

namespace MGDF {
namespace core {

class ParameterManager {
 public:
  virtual ~ParameterManager() {}
  static ParameterManager &Instance() {
    static ParameterManager pm;
    return pm;
  }

  /**
   parse a parameter string into the given string,string hashmap
   the parameter string must be in the following format
       -flag [value] -flag ["value"]
   where [] indicates optional components of the parameter string
   NOTE: flags and values are case sensitive
   NOTE: leading and trailing whitespace are stripped from values
  */
  virtual HRESULT ParseParameters(const std::string &,
                                  std::map<std::string, std::string> &);

  /**
   add every process environment variable whose name begins with prefix as a
   parameter, with the prefix stripped and the name lower-cased
   e.g. with prefix "MGDF_", the variable MGDF_LOGLEVEL=log_high becomes the
   parameter loglevel=log_high
   NOTE: existing parameters are overwritten, so add these before the command
   line if the command line should take precedence
  */
  virtual HRESULT AddEnvironmentParameters(const char *prefix);

  virtual bool HasParameter(const char *param) const;
  virtual const char *GetParameter(const char *param) const;
  virtual HRESULT AddParameterString(const char *paramString);

 private:
  std::map<std::string, std::string> _parameters;
};

}  // namespace core
}  // namespace MGDF
