#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>

template <class T>
class Queue {
 protected:
  // Data
  std::queue<T> queue_;
  typename std::queue<T>::size_type size_max_;

  // Thread gubbins
  std::mutex mutex_;
  std::condition_variable full_;
  std::condition_variable empty_;

  // Exit
  std::atomic_bool quit_{false};
  std::atomic_bool finished_{false};

 public:
  explicit Queue(size_t size_max);

  bool push(T&& data);
  bool pop(T& data);

  // The queue has finished accepting input
  bool is_finished();
  void finished();
  // The queue cannot be pushed or popped
  void quit();

  void empty();
};

template <class T>
Queue<T>::Queue(size_t size_max) : size_max_{size_max} {}

template <class T>
bool Queue<T>::push(T&& data) {
  std::unique_lock<std::mutex> lock(mutex_);

  while (!quit_ && !finished_) {
    if (queue_.size() < size_max_) {
      queue_.push(std::move(data));

      empty_.notify_all();
      return true;
    }
    full_.wait(lock);
  }

  return false;
}

template <class T>
bool Queue<T>::pop(T& data) {
  std::unique_lock<std::mutex> lock(mutex_);

  while (!quit_) {
    if (!queue_.empty()) {
      data = std::move(queue_.front());
      queue_.pop();

      full_.notify_all();
      return true;
    }
    if (queue_.empty() && finished_) {
      return false;
    }
    empty_.wait(lock);
  }

  return false;
}

template <class T>
bool Queue<T>::is_finished() {
  return finished_;
}

template <class T>
void Queue<T>::finished() {
  finished_ = true;
  empty_.notify_all();
}

template <class T>
void Queue<T>::quit() {
  quit_ = true;
  empty_.notify_all();
  full_.notify_all();
}

template <class T>
void Queue<T>::empty() {
  std::unique_lock<std::mutex> lock(mutex_);

  while (!queue_.empty()) {
    queue_.pop();
  }

  full_.notify_all();
}