#pragma once

#include <iomanip>
#include <ios>
#include <memory>
#include <string>
#include <vector>

namespace ubench {
  using duration = long double;

  using arg = size_t;
  using args = std::vector<arg>;

  template<typename T>
  struct formatter;

  template<>
  struct formatter<arg> {
    static std::string format(const arg v) {
      return std::to_string(v);
    }
  };

  template<>
  struct formatter<duration> {
    static std::string format(const duration v) {
      std::stringstream ss;
      ss << std::fixed << std::setprecision(2) << v;
      return ss.str();
    }
  };

  template<typename T>
  std::string format(T v) {
    return formatter<T>::format(v);
  }

  struct entry {
    std::string_view name;
    arg arg;
    duration mean;
    duration median;
    duration stddev;
    duration cv;
  };

  struct entry_size {
    size_t name;
    size_t mean;
    size_t median;
    size_t stddev;
    size_t cv;

    void adapt(const entry_size other) {
      name = std::max(name, other.name);
      mean = std::max(mean, other.mean);
      median = std::max(median, other.median);
      stddev = std::max(stddev, other.stddev);
      cv = std::max(cv, other.cv);
    }
  };

  struct formatted_entry {
    std::string name;
    std::string mean;
    std::string median;
    std::string stddev;
    std::string cv;

    formatted_entry(
      std::string name,
      std::string mean,
      std::string median,
      std::string stddev,
      std::string cv)
      : name(std::move(name)),
        mean(std::move(mean)),
        median(std::move(median)),
        stddev(std::move(stddev)),
        cv(std::move(cv)) {
    }

    explicit formatted_entry(const entry &e);

    entry_size size() const;
    void print(const entry_size &size) const;
  };

  using prepare_fn = void(*)(args &);
  using target_fn = void(*)(arg);

  struct options {
    std::string name;
    target_fn target = nullptr;
    prepare_fn prepare = nullptr;
    size_t iteration = 10000;
    size_t step = 20;
  };

  class benchmark;

  std::vector<std::unique_ptr<options>> &get_options();

  class benchmark {
    options *_options;

  public:
    explicit benchmark(options *options) : _options(options) {
    }

    benchmark(const std::string &name, target_fn target);

    /**
     * Register a routine as preparing routine.
     *
     * @param fn Preparing routine to register
     * @return Changed benchmark object
     */
    benchmark prepare(const prepare_fn fn) const {
      _options->prepare = fn;
      return *this;
    }

    /**
     * Sets iteration count.
     *
     * @param iteration Iteration count to use
     * @return Changed benchmark object
     */
    benchmark iteration(const size_t iteration) const {
      _options->iteration = iteration;
      return *this;
    }

    /**
     * Sets step count for iteration.
     *
     * @param step Step count to use
     * @return Changed benchmark object
     */
    benchmark step(const size_t step) const {
      _options->step = step;
      return *this;
    }

    /**
     * Runs operation with given argument and iteration count.
     *
     * @param arg Argument to be passed
     * @param iter Iteration count
     * @return Elapsed time per operation
     */
    [[nodiscard]]
    duration operator()(arg arg, size_t iter) const;

    /**
     * Runs operation with given argument.
     *
     * @param arg Argument to be passed
     * @return Benchmark result entry
     */
    [[nodiscard]]
    entry operator()(arg arg) const;

    /**
     * Warms up this benchmark.
     *
     * @param arg Argument to be passed
     */
    void warm_up(arg arg) const;

    /**
     * Runs whole benchmark.
     *
     * @param entries Entry vector to store results
     */
    void operator()(std::vector<entry> &entries) const;
  };

  /**
   * Runs all registered benchmarks and gets results of them.
   *
   * @return Results of run benchmarks
   */
  std::vector<entry> run();

  /**
   * Prints given benchmark results using Markdown table format.
   *
   * @param entries Benchmark results
   */
  void print(const std::vector<entry> &entries);
}

#define UB_CONCAT_(a, b) a##b
#define UB_CONCAT(a, b) UB_CONCAT_(a, b)
#define BENCHMARK(fn) auto UB_CONCAT(__ubench_, __COUNTER__) = ubench::benchmark(#fn, fn)
