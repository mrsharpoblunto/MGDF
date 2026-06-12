#pragma once

#include <string>
#include <vector>

#include "../MGDFHttpClient.hpp"

namespace MGDF {
namespace core {
namespace network {
namespace impl {

const char *GetHttpStatusString(int statusCode);

// if the message requests gzip content-encoding, compress the body into
// storage and point data/length at the compressed content. If compression
// fails, stripContentEncoding is set and the original body is used.
void EncodeBody(const HttpMessage &m, std::vector<char> &storage,
                const char *&data, size_t &length, bool &stripContentEncoding);

// decompress a gzip encoded body (if the headers indicate one) into
// m.Body, setting m.Error for unsupported encodings
void DecodeBody(const char *body, size_t bodyLength, HttpMessage &m);

bool CaseInsensitiveStartsWith(const std::string &str,
                               const std::string &prefix);

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
