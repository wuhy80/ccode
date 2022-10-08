#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class thread_guard {
  std::thread t;

 public:
  explicit thread_guard(std::thread& t_) : t(std::move(t_)) {
    if (!t.joinable()) {
      throw std::logic_error("No thread");
    }
  }

  ~thread_guard() {
    if (t.joinable()) {
      t.join();
    }
  }

  thread_guard(thread_guard const&) = delete;
  thread_guard& operator=(thread_guard const&) = delete;
};

class background_task {
 public:
  void operator()() const {
    do_something();
    do_something_else();
  }

  static void do_something() { std::cout << "do_something\n"; }

  static void do_something_else() { std::cout << "do_something_else\n"; }
};

void hello() { std::cout << "Hello concurrent world\n"; }

std::mutex mtx;

template <typename Iterator, typename T>
struct accumulate_block {
  void operator()(Iterator first, Iterator last, T& result) {
    auto start = std::chrono::steady_clock::now();

    for (auto it = first; it != last; it++) {
      result += (*it * 3 / 2);
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::unique_lock<std::mutex> lock(mtx);
    std::cout << "thread:" << std::this_thread::get_id()
              << " result: " << (unsigned long)result << " range: " << *first
              << "-" << *last << " times:" << diff.count() << std::endl;
  }
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
  unsigned long const length = std::distance(first, last);
  if (!length) {
    return init;
  }

  unsigned long const min_per_thread = 25;
  unsigned long const max_threads =
      (length + min_per_thread - 1) / min_per_thread;

  unsigned long const hardware_threads = std::thread::hardware_concurrency();
  unsigned long const num_threads =
      std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

  unsigned long const block_size = length / num_threads;

  std::vector<T> results(num_threads);
  std::vector<std::thread> threads(num_threads - 1);

  std::cout << "num threads:" << num_threads << std::endl;

  Iterator block_start = first;
  for (unsigned long i = 0; i < (num_threads - 1); ++i) {
    Iterator block_end = block_start;
    std::advance(block_end, block_size);
    threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start,
                             block_end, std::ref(results[i]));
    block_start = block_end;
  }

  accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);

  for (auto& entry : threads) {
    entry.join();
  }

  for (auto it = results.begin(); it != results.end(); it++) {
    init += *it;
  }

  return init;
}

// 效果不大
// multi thread accumulated result: 3051657985 tot time: 8.61885s
// single thread accumulated result: 3051657984 tot time: 10.0691s
int main() {
  std::cout << "max_concurrency:" << std::thread::hardware_concurrency()
            << std::endl;
  background_task f;
  std::thread t(f);
  thread_guard g(t);

  std::vector<int> vec(1000000000, 1);
  int inc_num = 0;
  for (int& it : vec) {
    it = inc_num++;
  }

  auto start = std::chrono::steady_clock::now();
  unsigned long tot = parallel_accumulate(vec.begin(), vec.end(), 1);
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "multi thread accumulated result: " << tot
            << " tot time: " << diff.count() << "s" << std::endl;

  start = std::chrono::steady_clock::now();
  tot = 0;
  for (auto it = vec.begin(); it != vec.end(); it++) {
    tot += (*it * 3 / 2);
  }
  end = std::chrono::steady_clock::now();
  diff = end - start;
  std::cout << "single thread accumulated result: " << tot
            << " tot time: " << diff.count() << "s" << std::endl;
  return 0;
}
