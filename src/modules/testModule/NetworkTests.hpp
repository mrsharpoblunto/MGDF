#pragma once

#include <MGDF/MGDF.h>

#include <unordered_set>

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

/**
this class acts as a command console to the lua  interfaces
*/
class NetworkTests : public TestModule {
 public:
  virtual ~NetworkTests(void);
  NetworkTests();

  void Setup(IMGDFSimHost *host) final;

 private:
  ComObject<IMGDFWebServer> _server;
  ComObject<IMGDFHttpClientRequest> _request;
  ComObject<IMGDFHttpClientResponse> _response;
  ComObject<IMGDFHttpClientResponse> _groupResponse;
  ComObject<IMGDFHttpClientRequestGroup> _requestGroup;
  void *_requestGroupKey;
  ComObject<IMGDFWebSocket> _socketClient;
  ComObject<IMGDFWebSocket> _socketServer;
  std::unordered_set<std::string> _pendingRecieve;
};

}  // namespace Test
}  // namespace MGDF
