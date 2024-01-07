#include "threads.h"
#include <iostream>

ThreadPool::ThreadPool()
{
  int cpus = std::thread::hardware_concurrency();
  std::cout << "Starting " << cpus << " threads" << std::endl;
  for (int i = 0; i != cpus; ++i)
  {
    threads_.emplace_back(&ThreadPool::thread_loop, this);
  }
}

ThreadPool::~ThreadPool()
{
  {
    std::unique_lock<std::mutex> guard(mutex_);
    terminate_ = true;
  }

  cv_.notify_all();

  for (auto& t : threads_)
  {
    t.join();
  }
}

void ThreadPool::thread_loop()
{
  while (true)
  {
    std::decay_t<decltype(jobs_.front())> job;

    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this]() {
        return !jobs_.empty() || terminate_;
      });

      if (terminate_) { return; }
      job = jobs_.front();
      jobs_.pop();
    }

    job();
  }
}

void ThreadPool::enqueue(ThreadJob f)
{
  {
    std::unique_lock lock(mutex_);
    jobs_.push(std::move(f));
  }
  cv_.notify_one();
}

bool ThreadPool::busy()
{
  std::unique_lock lock(mutex_);
  return !jobs_.empty();
}
