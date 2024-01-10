#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
  public:
  using ThreadJob = std::function<void()>;

  ThreadPool();
  ~ThreadPool();

  void enqueue(std::reference_wrapper<ThreadJob> f);
  bool busy();
  int pool_size()
  {
    return threads_.size();
  }

  private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::vector<std::thread> threads_;
  std::queue<std::reference_wrapper<ThreadJob>> jobs_;

  bool terminate_ = false;

  void thread_loop();
};
