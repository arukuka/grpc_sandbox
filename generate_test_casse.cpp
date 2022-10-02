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

int main(const int argc, const char** argv)
{
  const auto seed = [&]()
  {
    if (argc >= 2)
    {
      std::istringstream iss(argv[1]);
      unsigned int seed;
      iss >> seed;
      return seed;
    }
    else
    {
      std::random_device seed_gen;
      return seed_gen();
    }
  }();
  std::mt19937 engine(seed);

  std::uniform_int_distribution<std::int64_t> dist(
      std::numeric_limits<std::int64_t>::min(),
      std::numeric_limits<std::int64_t>::max()
  );

  std::vector<std::pair<int, boost::multiprecision::cpp_int>> data;
  for (int i = 0; i < 1000000; ++i)
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