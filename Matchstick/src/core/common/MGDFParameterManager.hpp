#pragma once

#include <MGDF/MGDF.hpp>
#include <exception>
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
  virtual MGDFError ParseParameters(const std::string &,
                                    std::map<std::string, std::string> &);

  virtual bool HasParameter(const char *param) const;
  virtual const char *GetParameter(const char *param) const;
  virtual MGDFError AddParameterString(const char *paramString);

 private:
  std::map<std::string, std::string> _parameters;
};

}  // namespace core
}  // namespace MGDF
