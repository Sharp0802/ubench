#include "ubench.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <numeric>
#include <sstream>

#include "rdtsc.h"

namespace {
  constexpr long double EPSILON = 1e-9;

  always_inline long double mean(const long double *v, const size_t size) {
    return std::accumulate(v, v + size, 0.0L) / size;
  }

  always_inline long double median(const long double *sorted, const size_t size) {
    if (size == 0)
      return 0;

    if (size & 1)
      return sorted[size / 2];

    return (sorted[size / 2 - 1] + sorted[size / 2]) / 2;
  }

  always_inline long double stddev(const long double *v, const size_t size, const long double mean) {
    long double squ_dev = 0;
    for (size_t i = 0; i < size; ++i) {
      const auto d = mean  - v[i];
      squ_dev += d * d;
    }
    squ_dev /= size;

    return std::sqrt(squ_dev);
  }

  always_inline long double cv(const long double stddev, const long double mean) {
    if (std::abs(mean) < EPSILON)
      return 0;
    return stddev / mean;
  }

  template<typename T>
  struct SizeOf {
    static size_t size(const T &v) {
      return v.size();
    }
  };

  template<>
  struct SizeOf<const char*> {
    static size_t size(const char *v) {
      return std::strlen(v);
    }
  };

  template<typename T>
  void padl(const T &s, const size_t size) {
    const auto size_s = SizeOf<T>::size(s);
    const auto pad = size - std::min(size, size_s);

    for (size_t i = 0; i < pad; ++i)
      std::cout << ' ';

    std::cout << s;
  }
}

namespace ubench {
  formatted_entry::formatted_entry(const entry &e)
    : mean(format(e.mean)),
      median(format(e.median)),
      stddev(format(e.stddev)),
      cv(format(e.cv)) {
    std::stringstream ss;
    ss << e.name << '(' << e.arg << ')';
    name = ss.str();
  }

  entry_size formatted_entry::size() const {
    return {
      name.size(),
      mean.size(),
      median.size(),
      stddev.size(),
      cv.size()
    };
  }

  void formatted_entry::print(const entry_size &size) const {
    padl(name, size.name);
    std::cout << " | ";
    padl(mean, size.mean);
    std::cout << " | ";
    padl(median, size.median);
    std::cout << " | ";
    padl(stddev, size.stddev);
    std::cout << " | ";
    padl(cv, size.cv);
  }

  std::vector<std::unique_ptr<options>> &get_options() {
    static std::vector<std::unique_ptr<options>> options;
    return options;
  }

  benchmark::benchmark(const std::string &name, const target_fn target) {
    if (!target)
      throw std::invalid_argument("target not specified");

    const auto &opt = get_options().emplace_back(std::make_unique<options>());

    _options = opt.get();
    _options->name = name;
    _options->target = target;
  }

  duration benchmark::operator()(const arg arg, const size_t iter) const {
    const auto target = _options->target;

    _mm_lfence();
    const auto base_begin = rdtsc();
    for (size_t i = 0; i < iter; ++i)
      asm volatile("" ::: "memory");
    const auto base_end = rdtscp();
    _mm_lfence();

    const auto base = base_end - base_begin;

    _mm_lfence();
    const auto begin = rdtsc();
    for (size_t i = 0; i < iter; ++i) {
      target(arg);
      asm volatile("" ::: "memory");
    }
    const auto end = rdtscp();
    _mm_lfence();

    auto elapsed = end - begin;
    if (elapsed < base)
      elapsed = 0;
    else
      elapsed -= base;

    return elapsed / static_cast<duration>(iter);
  }

  entry benchmark::operator()(const arg arg) const {
    if (_options->iteration < _options->step)
      throw std::out_of_range("step cannot be greater than iteration");

    std::vector<duration> samples(_options->iteration / _options->step);
    for (size_t iter = 1; iter <= samples.size(); ++iter)
      samples[iter - 1] = (*this)(arg, iter * _options->step);
    std::sort(samples.begin(), samples.end());

    const auto m = mean(samples.data(), samples.size());
    const auto sd = stddev(samples.data(), samples.size(), m);

    return {
      _options->name,
      arg,
      m,
      median(samples.data(), samples.size()),
      sd,
      cv(sd, m)
    };
  }

  void benchmark::warm_up(const arg arg) const {
    constexpr int MAX_TRIES = 64;

    int tries = 0;

    auto old = (*this)(arg);
    for (auto continuous = 0; continuous < 3 && tries < MAX_TRIES; ++tries) {
      const auto current = (*this)(arg);

      if (std::abs(old.cv) < EPSILON) {
        if (std::abs(current.cv) < EPSILON)
          continuous++;
        else
          continuous = 0;
      }
      else if (std::abs(old.cv - current.cv) / old.cv <= 0.1)
        continuous++;
      else
        continuous = 0;

      old = current;
    }

    if (tries >= MAX_TRIES)
      std::cerr << "warning: benchmark '" << old.name << "' failed to warm up correctly due to too many tries" << std::endl;
  }

  void benchmark::operator()(std::vector<entry> &entries) const {
    args args;
    if (_options->prepare)
      _options->prepare(args);

    entries.reserve(args.size());

    for (const auto arg : args) {
      warm_up(arg);
      entries.push_back((*this)(arg));
    }
  }

  std::vector<entry> run() {
    std::vector<entry> entries;
    for (const auto &p : get_options())
      benchmark(p.get())(entries);
    return entries;
  }

  void print(const std::vector<entry> &entries) {
    std::vector<formatted_entry> formatted_entries;
    formatted_entries.reserve(entries.size());
    for (const auto &e : entries)
      formatted_entries.emplace_back(e);

    const formatted_entry header{
      "name(arg)",
      "mean (cycle)",
      "median (cycle)",
      "stddev (cycle)",
      "cv"
    };

    entry_size size = header.size();
    for (size_t i = 0; i < formatted_entries.size(); ++i)
      size.adapt(formatted_entries[i].size());

    header.print(size);
    std::cout << std::endl;

    std::cout
      << std::string(size.name, '-')
      << ":|" << std::string(size.mean + 1, '-')
      << ":|" << std::string(size.median + 1, '-')
      << ":|" << std::string(size.stddev + 1, '-')
      << ":|" << std::string(size.cv, '-') << ':'
      << std::endl;

    for (auto &entry : formatted_entries) {
      entry.print(size);
      std::cout << std::endl;
    }
  }
}
