#include "threads.h"

#include <atomic>
#include <ranges>
#include <vector>

class ThreadPoolWaiter
{
  public:
  ThreadPoolWaiter() = default;

  void set_jobs(const std::vector<ThreadPool::ThreadJob>& jobs)
  {
    for (auto& job: jobs)
    {
      auto runner = [this, job]() {
        job();
        running_ -= 1;
      };

      wrapped_jobs_.push_back(runner);
    }
  }

  void run_and_wait(ThreadPool& pool)
  {
    running_ = wrapped_jobs_.size();

    for (auto& job : std::views::drop(wrapped_jobs_, 1))
    {
      pool.enqueue(std::ref(job));
    }

    wrapped_jobs_.front()();

    while (running_ != 0)
    {
    }
  }

  private:
  std::vector<ThreadPool::ThreadJob> wrapped_jobs_;
  std::atomic<int> running_;
};
