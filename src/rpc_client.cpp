#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <random>
#include <sstream>
#include <vector>
#include <thread>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "compute_kernel.grpc.pb.h"
#include "kernel.h"

using compute::Compute;
using compute::ComputeKernel;
using compute::ComputeKernelID;
using compute::ComputeStatus;
using compute::ComputeInputData;
using compute::DataType;
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::CompletionQueue;
using grpc::Status;

class ComputeClient {
public:
  explicit ComputeClient(std::shared_ptr<Channel> channel)
      : m_stub(Compute::NewStub(channel))
      {}

  std::string CreateKernel(const std::string &kernel, const uint64_t inputs, const std::vector<size_t> outputs) {
    ComputeKernel request;
    ComputeKernelID reply;
    ClientContext context;

    request.set_source(kernel);
    request.set_inputs(inputs);

    google::protobuf::RepeatedField<size_t> data(outputs.begin(), outputs.end());
    request.mutable_outputs()->Swap(&data);

    Status status = m_stub->CreateKernel(&context, request, &reply);

    if (!status.ok()) {
      std::cout << "Error: " << status.error_message()
                << std::endl;
      exit(EXIT_FAILURE);
    }

    return reply.uuid();
  }

  std::string SetInputData(std::string uuid, uint64_t index, std::vector<uint32_t> data) {
    ComputeInputData request;
    ComputeStatus reply;
    ClientContext context;

    request.set_uuid(uuid);
    request.set_index(index);
    request.set_size(data.size());

    google::protobuf::RepeatedField<uint32_t> bytes(data.begin(), data.end());
    request.mutable_data()->Swap(&bytes);

    Status status = m_stub->SetInputData(&context, request, &reply);

    if (!status.ok()) {
      std::cout << "Error: " << status.error_message()
                << std::endl;
      exit(EXIT_FAILURE);
    }

    return reply.message();
  }

  void Compute(std::string uuid) {
    ComputeKernelID request;
    ComputeStatus reply;
    ClientContext context;

    request.set_uuid(uuid);

    Status status = m_stub->Compute(&context, request, &reply);
    if (!status.ok()) {
      std::cout << "Error: " << status.error_message()
                << std::endl;
      exit(EXIT_FAILURE);
    }

    std::cout << reply.message() << std::endl;
  }

private:
  std::unique_ptr<Compute::Stub> m_stub;
};

int main(int argc, char **argv) {
  auto credentials = grpc::InsecureChannelCredentials();
  auto channel = grpc::CreateChannel("localhost:50051", credentials);
  ComputeClient client(channel);

  const char source[] =
        "__kernel void add(__global const uint *a,"
        "                  __global const uint *b,"
        "                  __global uint *c)"
        "{"
        "    const uint i = get_global_id(0);"
        "    c[i] = a[i] + b[i];"
        "}";
  const std::vector<size_t> outputs{ 4 * sizeof(uint32_t) };
  std::vector<uint32_t> a{ 1, 2, 3, 4 };
  std::vector<uint32_t> b{ 5, 6, 7, 8 };
  const uint64_t inputs = 2;

  const auto id = client.CreateKernel(source, inputs, outputs);
  std::cout << "Kernel created: " << id << std::endl;

  client.SetInputData(id, 0, a);
  client.SetInputData(id, 0, b);

  client.Compute(id);

  return 0;
}
