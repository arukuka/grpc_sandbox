#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/program_options.hpp>

int main(const int ac, const char* const* const av)
{
  boost::program_options::options_description description;

  // clang-format off
  description.add_options()
      ("seed", "")
      ("size", boost::program_options::value<std::size_t>()->default_value(1000000), "")
      ("help,h", "")
      ;
  // clang-format on

  boost::program_options::variables_map vm;
  store(parse_command_line(ac, av, description), vm);
  notify(vm);

  const auto seed = [&]()
  {
    if (vm.count("seed"))
    {
      return vm["seed"].as<unsigned int>();
    }
    else
    {
      std::random_device seed_gen;
      return seed_gen();
    }
  }();
  const std::size_t size = vm["size"].as<std::size_t>();

  std::mt19937 engine(seed);

  std::uniform_int_distribution<std::int64_t> dist(
      std::numeric_limits<std::int64_t>::min(),
      std::numeric_limits<std::int64_t>::max()
  );

  std::vector<std::pair<int, boost::multiprecision::cpp_int>> data;
  for (std::size_t i = 0; i < size; ++i)
  {
    data.emplace_back(
        std::make_pair(i, boost::multiprecision::cpp_int(dist(engine)))
    );
  }

  {
    std::ofstream ofs("input.bin");
    boost::archive::binary_oarchive ar(ofs);

    ar << data;
  }

  {
    std::ofstream ofs("input.txt");
    for (const auto& v : data)
    {
      ofs << v.first << " " << v.second << std::endl;
    }
  }
}