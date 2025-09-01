#include <iostream>

#include "ubench.h"

unsigned int seed;

static void prepare(ubench::args &state) {
  state = { 5, 10, 15 };
}

bool is_prime(const int v) {
  for (int i = 2; i * i <= v; i++) {
    if (v % i == 0) {
      return false;
    }
  }
  return true;
}

int nth_prime(const int nth) {
  for (auto i = 2, n = 0; ; ++i) {
    if (is_prime(i)) {
      if (++n == nth)
        return i;
    }
  }
}

static void test(const ubench::arg nth) {
  nth_prime(nth);
}

BENCHMARK(test).prepare(prepare).warmup(true);

int main() {
  const auto entries = ubench::run();
  ubench::print(entries);

  return 0;
}
