#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "compute_kernel.grpc.pb.h"
#include "kernel.h"

using compute::Compute;
using compute::ComputeInputData;
using compute::ComputeKernel;
using compute::ComputeKernelID;
using compute::ComputeStatus;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

// Logic and data behind the server's behavior.
class ComputeService final : public Compute::Service {
public:
  ComputeService() : m_kernels() {}

  Status CreateKernel(ServerContext *context, const ComputeKernel *request,
                      ComputeKernelID *reply) override {
    const auto source = request->source();
    const auto inputs = request->inputs();
    const auto outputs = request->outputs();
    const std::vector<size_t> data{outputs.begin(), outputs.end()};

    Kernel* kernel = new Kernel();

    boost::uuids::uuid random = boost::uuids::random_generator()();
    const auto uuid = boost::uuids::to_string(random);

    kernel->compile(source);
    kernel->addOutputParams(data);

    reply->set_uuid(uuid);

    m_kernels.emplace(uuid, kernel);

    return Status::OK;
  }

  Status SetInputData(ServerContext *context, const ComputeInputData *request,
                      ComputeStatus *reply) override {
    const auto size = request->size();
    const auto uuid = request->uuid();
    const auto index = request->index();
    const auto bytes = request->data();

    if (m_kernels.count(uuid) == 0) {
      return Status(StatusCode::INVALID_ARGUMENT, "UUID not found");
    }
    Kernel *kernel = m_kernels[uuid];

    std::vector<uint32_t> data{bytes.begin(), bytes.end()};

    kernel->addInputData<uint32_t>(data);

    reply->set_success(true);
    reply->set_message("It worked (I think)");

    return Status::OK;
  }

  Status Compute(ServerContext *context, const ComputeKernelID *request,
                 ComputeStatus *reply) override {
    const auto uuid = request->uuid();

    if (m_kernels.count(uuid) == 0) {
      return Status(StatusCode::INVALID_ARGUMENT, "UUID not found");
    }
    Kernel* kernel = m_kernels[uuid];

    kernel->execute();

    // get the output data.
    uint32_t *c = kernel->getOutputData<uint32_t>(0);
    // print out results in 'c'
    std::cout << "c: [" << c[0] << ", " << c[1] << ", " << c[2] << ", " << c[3]
              << "]" << std::endl;

    reply->set_success(true);
    reply->set_message("Let's see if it worked (fingers crossed)");

    return Status::OK;
  }

private:
  std::map<std::string, Kernel *> m_kernels;
};

int main(int argc, char **argv) {
#if 0
    Kernel kernel;

    const char source[] =
        "__kernel void add(__global const uint *a,"
        "                  __global const uint *b,"
        "                  __global uint *c)"
        "{"
        "    const uint i = get_global_id(0);"
        "    c[i] = a[i] + b[i];"
        "}";

    std::vector<uint32_t> a{ 1, 2, 3, 4 };
    std::vector<uint32_t> b{ 5, 6, 7, 8 };

    kernel.compile(source);

    // Set the input data.
    kernel.addInputData<uint32_t>(a);
    kernel.addInputData<uint32_t>(b);

    // Allocate space for output params.
    kernel.addOutputParams({ 4 * sizeof(uint32_t) });

    // Run the kernel.
    kernel.execute();

    // get the output data.
    uint32_t* c = kernel.getOutputData<uint32_t>(0);
    // print out results in 'c'
    std::cout << "c: [" << c[0] << ", "
                        << c[1] << ", "
                        << c[2] << ", "
                        << c[3] << "]" << std::endl;

    return 0;
#else
  std::string server_address("0.0.0.0:50051");
  ComputeService service;
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
  return 0;
#endif
}
