#include "stdafx.h"

#include "VFSCommon.hpp"

void ReadLines(const MGDF::ComObject<IMGDFFileReader> &reader,
               std::vector<std::string> &list) {
  const UINT32 size = static_cast<UINT32>(reader->GetSize());

  std::string data;
  data.resize(size);
  reader->Read(data.data(), size);

  std::string copy;
  copy.resize(size);
  char *copyPtr = copy.data();

  size_t index = 0;
  for (size_t i = 0; i < size; ++i) {
    if (data[i] != '\r' && data[i] != '\t') {
      copyPtr[index++] = data[i];
    }
  }
  copyPtr[index] = '\0';

  char *context = 0;
  char *ptr = strtok_s(copyPtr, "\n", &context);
  while (ptr) {
    list.push_back(std::string(ptr));
    ptr = strtok_s(0, "\n", &context);
  }
}
