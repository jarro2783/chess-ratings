#include "threads.h"

#include <atomic>
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
        {
          std::unique_lock lock(mutex_);
          running_ -= 1;
        }

        cv_.notify_one();
      };

      wrapped_jobs_.push_back(runner);
    }
  }

  void run_and_wait(ThreadPool& pool)
  {
    running_ = wrapped_jobs_.size();

    for (auto& job : wrapped_jobs_)
    {
      pool.enqueue(job);
    }

    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this]() {
      return running_ == 0;
    });
  }

  private:
  std::vector<ThreadPool::ThreadJob> wrapped_jobs_;
  std::condition_variable cv_;
  std::mutex mutex_;
  std::atomic<int> running_;
};
