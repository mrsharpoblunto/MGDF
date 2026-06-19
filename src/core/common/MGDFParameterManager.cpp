#include "StdAfx.h"

#include "MGDFParameterManager.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

bool ParameterManager::HasParameter(const char *param) const {
  _ASSERTE(param);
  std::string p = param;
  return _parameters.find(p) != _parameters.end();
}

const char *ParameterManager::GetParameter(const char *param) const {
  _ASSERTE(param);
  std::string p = param;
  const auto iter = _parameters.find(p);
  if (iter != _parameters.end()) {
    return iter->second.c_str();
  }
  return nullptr;
}

HRESULT ParameterManager::AddParameterString(const char *paramString) {
  _ASSERTE(paramString);
  std::string ps = paramString;
  return ParseParameters(ps, _parameters);
}

HRESULT ParameterManager::AddEnvironmentParameters(const char *prefix) {
  _ASSERTE(prefix);
  const size_t prefixLen = strlen(prefix);

  // GetEnvironmentStrings reflects any SetEnvironmentVariable calls made during
  // start up (e.g. values loaded from a .env file), unlike the CRT environ.
  LPCH env = GetEnvironmentStringsA();
  if (!env) {
    return E_FAIL;
  }

  for (LPCH var = env; *var != '\0'; var += strlen(var) + 1) {
    if (_strnicmp(var, prefix, prefixLen) != 0) {
      continue;
    }
    const char *name = var + prefixLen;
    const char *eq = strchr(name, '=');
    // skip entries with no name after the prefix (drive-letter entries like
    // "=C:=..." also have an empty name and are ignored here)
    if (!eq || eq == name) {
      continue;
    }
    std::string key(name, eq);
    for (char &c : key) {
      c = tolowerChar(c);
    }
    _parameters[key] = std::string(eq + 1);
  }

  FreeEnvironmentStringsA(env);
  return S_OK;
}

HRESULT ParameterManager::ParseParameters(
    const std::string &paramString,
    std::map<std::string, std::string> &paramMap) {
  auto iter = paramString.begin();

  for (;;) {
    std::string key, value;

    // Skip past any white space preceding the next token
    while (iter != paramString.end() && *iter <= ' ') {
      ++iter;
    }

    if (iter == paramString.end()) break;

    // Skip past the flag marker
    if (*iter == '-') {
      ++iter;
    } else {
      // expected to get a flag marker
      return E_INVALIDARG;
    }

    // get the flag string
    while (iter != paramString.end() && (*iter > ' ')) {
      key += *(iter++);
    }

    // check that the key is valid (i.e non null)
    if (key.length() == 0) {
      return E_INVALIDARG;
    }

    // get up until the next non-whitespace character
    while (iter != paramString.end() && (*iter <= ' ')) {
      ++iter;
    }

    // parse the value (if present). The next token is a value unless it starts a
    // new flag (a bare '-'). A quoted value runs to its matching closing quote and
    // may contain spaces and '-'; an unquoted value runs to the next whitespace,
    // so paths and URLs containing '-' are preserved intact.
    if (iter != paramString.end() && *iter != '-') {
      if (*iter == '"' || *iter == '\'') {
        const char quote = *iter;
        ++iter;  // skip opening quote
        while (iter != paramString.end() && *iter != quote) {
          value += *(iter++);
        }
        if (iter != paramString.end()) {
          ++iter;  // skip closing quote
        }
      } else {
        while (iter != paramString.end() && *iter > ' ') {
          value += *(iter++);
        }
      }
    }

    paramMap[key] = value;
  }
  return S_OK;
}

}  // namespace core
}  // namespace MGDF
