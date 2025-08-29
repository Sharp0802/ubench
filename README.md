# uBench : A Microbenchmark Framework

uBench is a microbenchmark framework focusing on simplicity and precision, written with C++17.

## At a glance

```cpp
static void prepare(ubench::args &state) {
  state = { 5, 10, 15 };
}

static void test(const ubench::arg nth) {
  nth_prime(nth);
}

BENCHMARK(test).prepare(prepare);

int main() {
  ubench::print(ubench::run());
  return 0;
}
```

Above boilerplate outputs below table in `stdout`:

```
name(arg) | mean (cycle) | median (cycle) | stddev (cycle) |   cv
---------:|-------------:|---------------:|---------------:|----:
  test(5) |        96.73 |          95.22 |           6.37 | 0.07
 test(10) |       292.77 |         291.13 |          11.65 | 0.04
 test(15) |       518.54 |         512.19 |          21.59 | 0.04
```

\* See full sample in `test/main.cxx`.

## Current Limitation

- Only support x86 (`rdtsc`, `rdtscp`, `lfence`)
