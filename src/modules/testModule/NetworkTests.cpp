#include "StdAfx.h"

#include "NetworkTests.hpp"

#include <MGDF/ComObject.hpp>
#include <sstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

NetworkTests::~NetworkTests(void) {}

NetworkTests::NetworkTests() : _requestGroupKey(nullptr) {}

void NetworkTests::Setup(IMGDFSimHost *host) {
  StepOnce([this](auto state) {
    state->Text.AddLine("");
    state->Text.AddLine("Network Tests");
    state->Text.AddLine("");
  })
      .StepOnce([host, this](auto state) {
        state->Text.AddLine("Https client request");
        host->CreateHttpRequest("https://www.matchstickframework.org/",
                                _request.Assign());
        host->CreateHttpRequestGroup(_requestGroup.Assign());
        _request->SetRequestMethod("GET");
        _requestGroupKey = _request->SendRequest(_requestGroup);
      })
      .Step([host, this](auto state) {
        std::ignore = state;
        auto groupResponseKey =
            _requestGroup->GetResponse(_groupResponse.Assign());
        if (groupResponseKey) {
          if (groupResponseKey != _requestGroupKey) {
            return TestStep::FAILED;
          } else if (!_request->GetResponse(_response.Assign())) {
            // both the group and request should have a response
            return TestStep::FAILED;
          } else {
            const MGDF::ComString<&IMGDFHttpClientResponse::GetResponseHeader>
                contentType(_response, "Content-Type");
            if (_groupResponse != _response ||
                _groupResponse->GetResponseCode() != 200 ||
                contentType != "text/html; charset=utf-8") {
              return TestStep::FAILED;
            } else {
              return TestStep::PASSED;
            }
          }
        }
        return TestStep::CONT;
      })
      .Step([this](auto state) {
        state->Text.AddLine("Https request cleanup");
        _requestGroup.Clear();
        _request.Clear();
        return TestStep::PASSED;
      })
      .StepOnce([this, host](auto state) {
        state->Text.AddLine("client request cancellation");
        host->CreateHttpRequest("https://www.matchstickframework.org/",
                                _request.Assign());
        _request->SetRequestMethod("GET")->SendRequest(nullptr);
        _request->CancelRequest();
      })
      .Step([this](auto state) {
        std::ignore = state;
        if (_request->GetResponse(_response.Assign())) {
          const MGDF::ComString<&IMGDFHttpClientResponse::GetResponseError>
              error(_response);
          if (error == "Cancelled" || _response->GetResponseCode() == -1) {
            return TestStep::PASSED;
          } else {
            return TestStep::FAILED;
          }
        }
        return TestStep::CONT;
      })
      .Step([this](auto state) {
        state->Text.AddLine("cancelled request cleanup");
        _request.Clear();
        return TestStep::PASSED;
      })
      .StepOnce([this, host](auto state) {
        state->Text.AddLine("Invalid client request (1): No server");
        host->CreateHttpRequest("http://localhost:6006", _request.Assign());
        _request->SetRequestMethod("GET")->SendRequest(nullptr);
      })
      .Step([this](auto state) {
        std::ignore = state;
        if (_request->GetResponse(_response.Assign())) {
          const MGDF::ComString<&IMGDFHttpClientResponse::GetResponseError>
              error(_response);
          if (!error.str().size() || _response->GetResponseCode() != 0) {
            return TestStep::FAILED;
          } else {
            return TestStep::PASSED;
          }
        }
        return TestStep::CONT;
      })
      .Step([this](auto state) {
        state->Text.AddLine("Invalid request cleanup (1)");
        _request.Clear();
        return TestStep::PASSED;
      })
      .StepOnce([this, host](auto state) {
        state->Text.AddLine("Invalid client request (2): Invalid URL");
        host->CreateHttpRequest("arglebargle", _request.Assign());
        _request->SetRequestMethod("GET")->SendRequest(nullptr);
      })
      .Step([this](auto state) {
        std::ignore = state;
        if (_request->GetResponse(_response.Assign())) {
          const MGDF::ComString<&IMGDFHttpClientResponse::GetResponseError>
              error(_response);
          if (!error.str().size() || _response->GetResponseCode() != 0) {
            return TestStep::FAILED;
          } else {
            return TestStep::PASSED;
          }
        }
        return TestStep::CONT;
      })
      .Step([this](auto state) {
        state->Text.AddLine("Invalid request cleanup (2)");
        _request.Clear();
        return TestStep::PASSED;
      })
      .StepOnce([this, host](auto state) {
        state->Text.AddLine("Wss socket client");
        host->CreateWebSocket("wss://echo.websocket.org/",
                              _socketClient.Assign());
        std::string message("Hello");
        _socketClient->Send(message.data(), message.size(), false);
      })
      .Step([this, host](auto state) {
        std::ignore = state;
        uint64_t recvBufferSize = 0;
        MGDFWebSocketConnectionStatus status{.LastErrorLength = 0};
        if (_socketClient->CanRecieve(&recvBufferSize)) {
          std::string messageBuffer;
          messageBuffer.resize(recvBufferSize);
          BOOL binary = false;
          if (SUCCEEDED(_socketClient->Receive(messageBuffer.data(),
                                               recvBufferSize, &binary)) &&
              !binary) {
            if (messageBuffer == "Hello") {
              return TestStep::PASSED;
            }
          } else {
            return TestStep::FAILED;
          }
        } else if (FAILED(_socketClient->GetConnectionStatus(&status)) &&
                   status.LastErrorLength > 0) {
          return TestStep::FAILED;
        }
        return TestStep::CONT;
      })
      .Step([this](auto state) {
        state->Text.AddLine("Wss socket client cleanup");
        _socketClient.Clear();
        return TestStep::PASSED;
      })
      .StepOnce([this, host](auto state) {
        state->Text.AddLine("Invalid client WebSocket (1): No server");
        host->CreateWebSocket("wss://localhost:6006/", _socketClient.Assign());
        std::string message("Hello");
        _socketClient->Send(message.data(), message.size(), false);
      })
      .Step([this](auto state) {
        std::ignore = state;
        MGDFWebSocketConnectionStatus status{
            .LastError = nullptr,
            .LastErrorLength = 0,
        };
        if (FAILED(_socketClient->GetConnectionStatus(&status))) {
          std::string lastError(status.LastErrorLength, ' ');
          status.LastError = lastError.data();
          if (SUCCEEDED(_socketClient->GetConnectionStatus(&status))) {
            if (lastError.size() && status.State == MGDF_WEBSOCKET_CLOSED) {
              return TestStep::PASSED;
            }
          }
          return TestStep::FAILED;
        }
        return TestStep::CONT;
      })
      .Step([this](auto state) {
        state->Text.AddLine("Invalid client WebSocket cleanup (1)");
        _socketClient.Clear();
        return TestStep::PASSED;
      })
      .StepOnce([this, host](auto state) {
        state->Text.AddLine("Starting Web Server on port 3000");
        host->CreateWebServer(3000, "/websocket", _server.Assign());
      })
      .Step([this, host](auto state) {
        std::ignore = state;
        if (_server->Listening()) {
          return TestStep::PASSED;
        }
        return TestStep::CONT;
      })
      .StepOnce([this, host](auto state) {
        state->Text.AddLine("Web Server multiple http requests");

        for (int i = 0; i < 10; i++) {
          auto key = std::to_string(i);
          _pendingRecieve.insert(key);
          ComObject<IMGDFHttpClientRequest> request;
          std::ostringstream url;
          url << "http://localhost:3000/" << key << "?key=" << key;
          host->CreateHttpRequest(url.str().c_str(), request.Assign());
          request->SetRequestMethod("POST")
              ->SetRequestBody(key.c_str(), key.size())
              ->SetRequestHeader("Content-Type", "text/plain")
              ->SendRequest(nullptr);
        }
      })
      .Step([this, host](auto state) {
        std::ignore = state;
        MGDFWebServerRequest request{
            .WebSocket = nullptr,
            .HttpRequest = nullptr,
        };
        if (_server->RequestRecieved(&request)) {
          if (request.HttpRequest) {
            ComObject<IMGDFHttpServerRequest> httpRequest(request.HttpRequest);
            httpRequest->SetResponseCode(200)->SendResponse();

            const MGDF::ComString<&IMGDFHttpServerRequest::GetRequestMethod>
                method(httpRequest);
            const MGDF::ComString<&IMGDFHttpServerRequest::GetRequestHeader>
                contentType(httpRequest, "Content-Type");
            if (method != "POST" || contentType != "text/plain") {
              return TestStep::FAILED;
            }

            const MGDF::ComString<&IMGDFHttpServerRequest::GetRequestBody> body(
                httpRequest);
            const MGDF::ComString<&IMGDFHttpServerRequest::GetRequestPath> path(
                httpRequest);

            std::ostringstream expectedPath;
            expectedPath << "/" << body << "?key=" << body;

            if (path.str() != expectedPath.str()) {
              return TestStep::FAILED;
            }

            auto found = _pendingRecieve.find(body);
            if (found != _pendingRecieve.end()) {
              _pendingRecieve.erase(found);
            } else {
              return TestStep::FAILED;
            }

            if (_pendingRecieve.empty()) {
              return TestStep::PASSED;
            }
          }
        }
        return TestStep::CONT;
      })
      .StepOnce([this, host](auto state) {
        state->Text.AddLine("Client WebSocket -> Web Server WebSocket");
        host->CreateWebSocket("ws://localhost:3000/websocket",
                              _socketClient.Assign());
        std::string foobar("foobar");
        _socketClient->Send(foobar.data(), foobar.size(), false);
      })
      .Step([this](auto state) {
        std::ignore = state;
        MGDFWebServerRequest request{
            .WebSocket = nullptr,
            .HttpRequest = nullptr,
        };
        std::string messageBuffer;
        messageBuffer.resize(6);

        BOOL binary = false;
        if (_server->RequestRecieved(&request)) {
          if (request.WebSocket) {
            _socketServer = ComObject<IMGDFWebSocket>(request.WebSocket);
          }
        } else if (_socketServer && _socketServer->Receive(messageBuffer.data(),
                                                           messageBuffer.size(),
                                                           &binary) == S_OK) {
          if (messageBuffer != "foobar" || binary) {
            return TestStep::FAILED;
          } else {
            std::string foobarbaz("foobarbaz");
            _socketServer->Send(foobarbaz.data(), foobarbaz.size(), false);
            return TestStep::PASSED;
          }
        }
        return TestStep::CONT;
      })
      .StepOnce([this](auto state) {
        state->Text.AddLine("Web Server WebSocket -> Client WebSocket");
      })
      .Step([this](auto state) {
        std::ignore = state;
        std::string messageBuffer;
        messageBuffer.resize(9);
        BOOL binary = false;
        if (_socketClient &&
            SUCCEEDED(_socketClient->Receive(messageBuffer.data(),
                                             messageBuffer.size(), &binary))) {
          if (messageBuffer != "foobarbaz" || binary) {
            return TestStep::FAILED;
          } else {
            return TestStep::PASSED;
          }
        }
        return TestStep::CONT;
      })
      .StepOnce([this, host](auto state) {
        _socketClient.Clear();
        state->Text.AddLine("Web Server cleanup");
      })
      .Step([this, host](auto state) {
        std::ignore = state;
        MGDFWebSocketConnectionStatus status;
        if (SUCCEEDED(_socketServer->GetConnectionStatus(&status)) &&
            status.State == MGDF_WEBSOCKET_CLOSED) {
          _socketServer.Clear();
          _server.Clear();
          return TestStep::PASSED;
        }
        return TestStep::CONT;
      });
}

}  // namespace Test
}  // namespace MGDF
