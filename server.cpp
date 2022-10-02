
#include <grpcpp/completion_queue.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/status_code_enum.h>

#include <atomic>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/format.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#include "data_transfer.grpc.pb.h"
#include "data_transfer.pb.h"

std::atomic<std::size_t> count = 0;
std::mutex mtx;
std::condition_variable cond;
bool finished = false;

std::chrono::system_clock::time_point start;

class Server final : public sandbox::Transfer::Service
{
public:
  Server(const std::vector<std::pair<int, boost::multiprecision::cpp_int>>&
             excepted)
  {
    done.resize(excepted.size());
    for (const auto& kv : excepted)
    {
      dict[kv.first] = kv.second;
    }
  }

  grpc::Status Start(
      grpc::ServerContext* context,
      const google::protobuf::Empty* request_,
      google::protobuf::Empty* response
  ) override
  {
    start = std::chrono::system_clock::now();

    return grpc::Status::OK;
  }

  grpc::Status Send(
      grpc::ServerContext* context,
      const sandbox::Value* request_,
      google::protobuf::Empty* response
  ) override
  {
    const sandbox::Value request = *request_;
    const std::string serialized = request.value();
    const int id = request.id();
    const boost::multiprecision::cpp_int value{serialized};

    return process(id, value);
  }

  grpc::Status SendClientStream(
      grpc::ServerContext* context,
      grpc::ServerReader<sandbox::Values>* request,
      google::protobuf::Empty* response
  ) override
  {
    sandbox::Values values;
    while (request->Read(&values))
    {
      for (int i = 0; i < values.values_size(); ++i)
      {
        const auto one = values.values(i);
        const std::string serialized = one.value();
        const int id = one.id();
        const boost::multiprecision::cpp_int value{serialized};

        const auto status = process(id, value);

        if (!status.ok())
        {
          return status;
        }
      }
    }
    return grpc::Status::OK;
  }

  grpc::Status SendV2(
      grpc::ServerContext* context,
      const sandbox::ValueV2* request_,
      google::protobuf::Empty* response
  ) override
  {
    const sandbox::ValueV2 request = *request_;
    const std::string serialized = request.value();
    const int id = request.id();
    const boost::multiprecision::cpp_int value = [&serialized]()
    {
      boost::multiprecision::cpp_int dst;
      std::stringstream ss(serialized);

      boost::archive::binary_iarchive ar(ss);

      ar >> dst;

      return dst;
    }();

    return process(id, value);
  }

  grpc::Status SendClientStreamV2(
      grpc::ServerContext* context,
      grpc::ServerReader<sandbox::ValuesV2>* request,
      google::protobuf::Empty* response
  ) override
  {
    sandbox::ValuesV2 values;
    while (request->Read(&values))
    {
      for (int i = 0; i < values.values_size(); ++i)
      {
        const auto one = values.values(i);
        const std::string serialized = one.value();
        const int id = one.id();
        const boost::multiprecision::cpp_int value = [&serialized]()
        {
          boost::multiprecision::cpp_int dst;
          std::stringstream ss(serialized);

          boost::archive::binary_iarchive ar(ss);

          ar >> dst;

          return dst;
        }();

        const auto status = process(id, value);

        if (!status.ok())
        {
          return status;
        }
      }
    }
    return grpc::Status::OK;
  }

  bool check() const
  {
    bool ok = true;
    for (const auto& v : done)
    {
      ok = ok && v;
    }
    return ok;
  }

private:
  grpc::Status process(
      const int id, const boost::multiprecision::cpp_int& value
  )
  {
    const auto answer = std::make_pair(id, value);

    if (dict.count(id) == 0)
    {
      return grpc::Status(
          grpc::StatusCode::NOT_FOUND,
          (boost::format("id: %d does not found") % id).str()
      );
    }

    const auto expected = dict[id];
    if (value != expected)
    {
      return grpc::Status(
          grpc::StatusCode::UNKNOWN,
          (boost::format("[%d] expected: %s, actual: %s") % id %
           expected.str() % value.str())
              .str()
      );
    }

    done[id] = true;

    if (++count >= done.size())
    {
      {
        std::lock_guard<std::mutex> lk(mtx);
        finished = true;
      }
      cond.notify_all();
    }

    return grpc::Status::OK;
  }

  std::map<int, boost::multiprecision::cpp_int> dict;
  std::vector<bool> done;
};

int main()
{
  std::vector<std::pair<int, boost::multiprecision::cpp_int>> excepted;
  {
    std::ifstream ifs("input.bin");
    boost::archive::binary_iarchive ar(ifs);

    ar >> excepted;
  }
  Server server(excepted);
  grpc::ServerBuilder builder;

  builder.RegisterService(&server);
  builder.AddListeningPort(
      "localhost:50051", grpc::InsecureServerCredentials()
  );

  auto listener = builder.BuildAndStart();

  std::thread thread(
      [&]
      {
        std::unique_lock<std::mutex> lk(mtx);
        cond.wait(lk, [] { return finished; });

        const auto tp = std::chrono::system_clock::now();
        const auto dur = std::chrono::duration_cast<std::chrono::duration<double>>(tp - start).count();
        std::cout << "elapsed time: " << dur << std::endl;

        const bool ok = server.check();
        std::cout << (ok ? "OK" : "ERROR") << std::endl;

        listener->Shutdown();
      }
  );

  listener->Wait();
  thread.join();

  return 0;
}
