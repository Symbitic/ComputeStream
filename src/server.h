#ifndef Server_H
#define Server_H

#include <drogon/drogon.h>
#include "kernel.h"

using namespace drogon;

using HttpCallback = std::function<void(const HttpResponsePtr &)> &&;

enum DataType {
    UINT32,
    FLOAT,
};

struct KernelItem
{
    unsigned int type;
    Kernel kernel;
    std::mutex mtx;
};

class Server : public HttpController<Server>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Server::createKernel, "/create", Post);
    ADD_METHOD_VIA_REGEX(Server::kernelInfo, "/([a-f0-9]{64})", Get);
    ADD_METHOD_VIA_REGEX(Server::updateKernel, "/update/([a-f0-9]{64})", Put);
    ADD_METHOD_VIA_REGEX(Server::executeKernel, "/compute/([a-f0-9]{64})", Get);
    METHOD_LIST_END

    void kernelInfo(const HttpRequestPtr&, HttpCallback callback, const std::string& id);
    void createKernel(const HttpRequestPtr& req, HttpCallback callback);
    void updateKernel(const HttpRequestPtr& req, HttpCallback callback, const std::string& id);
    void executeKernel(const HttpRequestPtr&, HttpCallback callback, const std::string& id);

private:
    std::map<std::string, std::shared_ptr<KernelItem>> m_kernels;
    std::mutex m_mutex;
};

#endif
