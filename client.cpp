#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/support/status.h>

#include <algorithm>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/format.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <cstdlib>
#include <fstream>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <indicators/setting.hpp>
#include <memory>
#include <sstream>

#include "data_transfer.grpc.pb.h"
#include "data_transfer.pb.h"

enum class Version
{
  V1,
  V2
};

template <Version VERSION>
struct value_type;

template <>
struct value_type<Version::V1>
{
  using type = sandbox::Value;
};

template <>
struct value_type<Version::V2>
{
  using type = sandbox::ValueV2;
};

template <Version VERSION>
struct values_type;

template <>
struct values_type<Version::V1>
{
  using type = sandbox::Values;
};

template <>
struct values_type<Version::V2>
{
  using type = sandbox::ValuesV2;
};

template <Version VERSION = Version::V1>
void send_one_by_one(
    std::shared_ptr<grpc::Channel> channel,
    const std::vector<std::pair<int, boost::multiprecision::cpp_int>>& input
)
{
  auto stub = sandbox::Transfer::NewStub(channel);
  indicators::BlockProgressBar bar{
      indicators::option::BarWidth{80},
      indicators::option::ShowElapsedTime{true},
      indicators::option::ShowRemainingTime{true},
      indicators::option::MaxProgress{input.size()}};

  for (const auto& kv : input)
  {
    typename value_type<VERSION>::type req;
    req.set_id(kv.first);
    if constexpr (VERSION == Version::V1)
    {
      req.set_value(kv.second.str());
    }
    else if constexpr (VERSION == Version::V2)
    {
      std::stringstream oss;
      {
        boost::archive::binary_oarchive ar(oss);
        ar << kv.second;
      }

      req.set_value(oss.str());
    }

    grpc::ClientContext context;
    google::protobuf::Empty res;

    grpc::Status status = [&]()
    {
      if constexpr (VERSION == Version::V1)
      {
        return stub->Send(&context, req, &res);
      }
      else if constexpr (VERSION == Version::V2)
      {
        return stub->SendV2(&context, req, &res);
      }
    }();

    if (!status.ok())
    {
      std::cerr << (boost::format("%d %s") % status.error_code() %
                    status.error_message())
                << std::endl;
      std::abort();
    }

    bar.tick();
  }
}

template <Version VERSION = Version::V1>
void send_by_stream(
    std::shared_ptr<grpc::Channel> channel,
    const std::vector<std::pair<int, boost::multiprecision::cpp_int>>& input,
    const int buffer_size = 1
)
{
  auto stub = sandbox::Transfer::NewStub(channel);
  grpc::ClientContext context;
  google::protobuf::Empty res;
  std::shared_ptr<grpc::ClientWriter<typename values_type<VERSION>::type>>
      stream(
          [&]()
          {
            if constexpr (VERSION == Version::V1)
            {
              return std::move(stub->SendClientStream(&context, &res));
            }
            else if constexpr (VERSION == Version::V2)
            {
              return std::move(stub->SendClientStreamV2(&context, &res));
            }
          }()
      );

  indicators::BlockProgressBar bar{
      indicators::option::BarWidth{80},
      indicators::option::ShowElapsedTime{true},
      indicators::option::ShowRemainingTime{true},
      indicators::option::MaxProgress{input.size()}};

  typename values_type<VERSION>::type req{};
  int count = 0;
  for (const auto& kv : input)
  {
    auto values = req.add_values();
    typename value_type<VERSION>::type one;

    one.set_id(kv.first);

    if constexpr (VERSION == Version::V1)
    {
      one.set_value(kv.second.str());
    }
    else if constexpr (VERSION == Version::V2)
    {
      std::stringstream oss;
      {
        boost::archive::binary_oarchive ar(oss);
        ar << kv.second;
      }

      one.set_value(oss.str());
    }

    *values = one;

    if (++count >= buffer_size)
    {
      stream->Write(req);
      req = typename values_type<VERSION>::type();
      count = 0;
    }

    bar.tick();
  }

  if (req.values_size() > 0)
  {
    stream->Write(req);
  }

  stream->WritesDone();

  const grpc::Status status = stream->Finish();

  if (!status.ok())
  {
    std::cerr << (boost::format("%d %s") % status.error_code() %
                  status.error_message())
              << std::endl;
    std::abort();
  }
}

int main(const int ac, const char* const* const av)
{
  boost::program_options::options_description description;

  description.add_options()(
      "method,m", boost::program_options::value<int>()->default_value(0), ""
  )("help,h", "")("size,s", boost::program_options::value<int>()->default_value(1000), "");
  boost::program_options::variables_map vm;
  store(parse_command_line(ac, av, description), vm);
  notify(vm);

  if (vm.count("help"))
  {
    std::cout << description << std::endl;
    return 0;
  }

  if (!vm.count("method"))
  {
    std::cerr << description << std::endl;
    return EXIT_FAILURE;
  }

  const int method = vm["method"].as<int>();
  const int buffer_size = vm["size"].as<int>();

  auto channel = grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()
  );
  auto stub = sandbox::Transfer::NewStub(channel);

  std::vector<std::pair<int, boost::multiprecision::cpp_int>> input;
  {
    std::ifstream ifs("input.bin");
    boost::archive::binary_iarchive ar(ifs);

    ar >> input;
  }

  {
    grpc::ClientContext context;
    google::protobuf::Empty req, res;
    const auto status = stub->Start(&context, req, &res);

    if (!status.ok())
    {
      std::cerr << (boost::format("%d %s") % status.error_code() %
                    status.error_message())
                << std::endl;
      std::abort();
    }
  }

  switch (method)
  {
    case 0:
      send_one_by_one(channel, input);
      break;
    case 1:
      send_by_stream(channel, input);
      break;
    case 2:
      send_by_stream(channel, input, buffer_size);
      break;
    case 10:
      send_one_by_one<Version::V2>(channel, input);
      break;
    case 11:
      send_by_stream<Version::V2>(channel, input);
      break;
    case 12:
      send_by_stream<Version::V2>(channel, input, buffer_size);
      break;
    default:
      std::cerr << boost::format("method id: %d is not implemented") % method
                << std::endl;
      return EXIT_FAILURE;
  }

  return 0;
}
