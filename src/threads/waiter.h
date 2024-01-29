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
