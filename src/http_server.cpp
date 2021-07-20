/*
https://github.com/an-tao/drogon/blob/master/examples/login_session/main.cc
https://github.com/an-tao/drogon/blob/master/examples/helloworld/main.cc
https://github.com/an-tao/drogon/wiki/ENG-18-Testing-Framework
https://github.com/an-tao/drogon/blob/master/examples/jsonstore/main.cc
https://github.com/drogonframework/drogon/blob/master/examples/login_session/main.cc

curl --location --request POST 'localhost:8080/' \
--header 'Content-Type: application/json' \
--data-raw '{"value": "bar"}'

https://github.com/drogonframework/drogon/blob/master/examples/jsonstore/README.md
curl -XGET http://localhost:8848/get-token
curl -XPOST http://localhost:8848/3a322920d42ef0763152a6efff2ed51985530aedd45370f92fd0f0b8dcc30220 \
        -H 'content-type: application/json' -d '{"foo":{"bar":42}}'

*/
#include <drogon/drogon.h>
#include "server.h"

using namespace drogon;

using HttpCallback = std::function<void(const HttpResponsePtr &)> &&;

static inline HttpResponsePtr makeFailedResponse(std::string msg = "")
{
    Json::Value json;
    json["success"] = false;
    json["data"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(json);
    resp->setStatusCode(k500InternalServerError);
    return resp;
}

static inline HttpResponsePtr makeSuccessResponse(std::string msg = "")
{
    Json::Value json;
    json["success"] = true;
    json["data"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(json);
    return resp;
}

int main() {
  auto rootHandler = [](const HttpRequestPtr &, HttpCallback callback) {
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody("Hello, World!");
    callback(resp);
  };

  // `registerHandler()` adds a handler to the desired path. The handler is
  // responsible for generating a HTTP response upon an HTTP request being
  // sent to Drogon
  app().registerHandler("/", rootHandler, {Get});

  // Ask Drogon to listen on 127.0.0.1 port 8848. Drogon supports listening
  // on multiple IP addresses by adding multiple listeners. For example, if
  // you want the server also listen on 127.0.0.1 port 5555. Just add another
  // line of addListener("127.0.0.1", 5555)
  LOG_INFO << "Server running on 127.0.0.1:8848";

  app()
    //.setLogLevel(trantor::Logger::kWarn)
    .addListener("127.0.0.1", 8848)
    .run();
}
