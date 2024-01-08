#pragma once

#include <chrono>
#include <iostream>

class Timer
{
  public:
  void start()
  {
    start_ = clock_type::now();
  }

  void stop(const char* prefix)
  {
    auto end = clock_type::now();
    std::cout << prefix << ": " << std::chrono::duration_cast<std::chrono::microseconds>(end - start_) << std::endl;
  }

  private:
  using clock_type = std::chrono::high_resolution_clock;
  using time_point = std::chrono::time_point<clock_type>;
  time_point start_;
};


