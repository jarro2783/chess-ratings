#include <cmath>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

#include "mapped_file.h"
#include "ratings.h"
#include "timer.h"

int main(int argc, const char** argv)
{
  if (argc < 2)
  {
    std::cerr << "Provide input file" << std::endl;
    return 1;
  }

  try
  {
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
