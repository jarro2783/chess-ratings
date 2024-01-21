#pragma once

class Timer
{
  public:
  void start()
  {
    start_ = clock_type::now();
  }

  auto stop()
  {
    return clock_type::now() - start_;
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


