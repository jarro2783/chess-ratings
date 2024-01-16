#include <cmath>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

#include "mapped_file.h"
#include "ratings.h"
#include "timer.h"

#include "cxxopts.hpp"

int main(int argc, const char** argv)
{
  if (argc < 2)
  {
    std::cerr << "Provide input file" << std::endl;
    return 1;
  }

  try
  {
    cxxopts::Options options(argv[0], "Chess ratings calculator");
    options.add_options()
      ("gpu", "Use GPU calculation", cxxopts::value<bool>())
      ("w,wadv", "Set white advantage", cxxopts::value<double>())
      ("games", "Games file to read from", cxxopts::value<std::string>())
      ;

    options.parse_positional({"games"});
    auto parsed = options.parse(argc, argv);

    RatingsCalc calc;
    calc.read_games(argv[1]);

    Timer timer;
    timer.start();
    calc.find_ratings();
    timer.stop("find_ratings");
    calc.print_ratings("ratings-out.txt");
  } catch(const std::string& e)
  {
    std::cerr << "Exception caught " << e << std::endl;
    return 1;
  }

  return 0;
}
