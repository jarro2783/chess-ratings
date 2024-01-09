#include "threads.h"

#include <atomic>
#include <latch>
#include <vector>

class ThreadPoolWaiter
{
  public:
  ThreadPoolWaiter() = default;

  void set_jobs(const std::vector<ThreadPool::ThreadJob>& jobs)
  {
    jobs_ = jobs;
  #if 0
    for (auto& job: jobs)
    {
      auto runner = [this, job]() {
        job();
        running_ -= 1;
      };

      wrapped_jobs_.push_back(runner);
    }
    #endif
  }

  void run_and_wait(ThreadPool& pool)
  {
    std::latch latch(jobs_.size());

    for (auto& job : jobs_)
    {
      pool.enqueue([&latch, job]() {
        job();
        latch.count_down();
      });
    }

    latch.wait();
  }

  private:
  std::vector<ThreadPool::ThreadJob> jobs_;
};
