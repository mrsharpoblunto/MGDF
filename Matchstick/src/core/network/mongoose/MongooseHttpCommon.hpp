#pragma once

#include "../../common/MGDFResources.hpp"
#include "../MGDFHttpClient.hpp"
#include "mongoose.h"

namespace MGDF {
namespace core {
namespace network {
namespace mongoose {

#define mg_stdstr(str) mg_str_n(str.c_str(), str.size())

static const std::string S_CONTENT_ENCODING;
static const std::string S_ACCEPT_ENCODING;
static const std::string S_CONTENT_LENGTH;
static const std::string S_GZIP;

void CreateHttpMessage(mg_http_message *hm, HttpMessage &m);
void SendHttpResponse(mg_connection *c, const HttpMessage &m);
void SendHttpRequest(mg_connection *c, const std::string &host,
                     const HttpMessage &m);

}  // namespace mongoose
}  // namespace network
}  // namespace core
}  // namespace MGDF
