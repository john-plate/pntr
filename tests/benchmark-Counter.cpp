#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <pntr/pntr.hpp>

#include <cstdlib>


template<class t_counter>
typename t_counter::ValueType
benchmark(std::uint32_t const p_random)
{
  using T = typename t_counter::ValueType;
  t_counter c(static_cast<T>(1u));
  for (std::uint32_t u = p_random; u < p_random + 250u; ++u)
  {
    c.increment(static_cast<T>(u << 1u));
    c.decrement(static_cast<T>(u));
    T expected = c.get_count();
    while (!c.compare_exchange_weak(expected, expected + static_cast<T>(u)))
    {}
  }
  return c.get_count();
}


TEST_CASE("Counter benchmark")
{
  std::uint32_t const r = static_cast<std::uint32_t>(std::rand());

  BENCHMARK("CounterThreadSafe<std::uint8_t>")
  {
    return benchmark<pntr::CounterThreadSafe<std::uint8_t>>(r);
  };

  BENCHMARK("CounterThreadSafe<std::uint16_t>")
  {
    return benchmark<pntr::CounterThreadSafe<std::uint16_t>>(r);
  };

  BENCHMARK("CounterThreadSafe<std::uint32_t>")
  {
    return benchmark<pntr::CounterThreadSafe<std::uint32_t>>(r);
  };

  BENCHMARK("CounterThreadSafe<std::uint64_t>")
  {
    return benchmark<pntr::CounterThreadSafe<std::uint64_t>>(r);
  };

  BENCHMARK("CounterThreadUnsafe<std::uint8_t>")
  {
    return benchmark<pntr::CounterThreadUnsafe<std::uint8_t>>(r);
  };

  BENCHMARK("CounterThreadUnsafe<std::uint16_t>")
  {
    return benchmark<pntr::CounterThreadUnsafe<std::uint16_t>>(r);
  };

  BENCHMARK("CounterThreadUnsafe<std::uint32_t>")
  {
    return benchmark<pntr::CounterThreadUnsafe<std::uint32_t>>(r);
  };

  BENCHMARK("CounterThreadUnsafe<std::uint64_t>")
  {
    return benchmark<pntr::CounterThreadUnsafe<std::uint64_t>>(r);
  };
}


static std::uint32_t
benchmark_regular(std::uint32_t const p_random)
{
  pntr::CounterThreadUnsafe<std::uint32_t> c(1u);
  for (std::uint32_t u = p_random; u < p_random + 250u; ++u)
  {
    c.increment(u << 1u);
    c.decrement(u);
    std::uint32_t expected = c.get_count();
    while (!c.compare_exchange_weak(expected, expected + u))
    {}
  }
  return c.get_count();
}

static std::uint32_t
benchmark_launder(std::uint32_t const p_random)
{
  using Counter = pntr::CounterThreadUnsafe<std::uint32_t>;
  alignas(Counter) std::byte storage[sizeof(Counter)];
  new (&storage) Counter(1u);
  for (std::uint32_t u = p_random; u < p_random + 250u; ++u)
  {
    std::launder(reinterpret_cast<Counter *>(&storage))->increment(u << 1u);
    std::launder(reinterpret_cast<Counter *>(&storage))->decrement(u);
    std::uint32_t expected = std::launder(reinterpret_cast<Counter *>(&storage))->get_count();
    while (!std::launder(reinterpret_cast<Counter *>(&storage))->compare_exchange_weak(expected, expected + u))
    {}
  }
  return std::launder(reinterpret_cast<Counter *>(&storage))->get_count();
}

TEST_CASE("Launder benchmark")
{
  std::uint32_t const r = static_cast<std::uint32_t>(std::rand());

  BENCHMARK("benchmark_regular")
  {
    return benchmark_regular(r);
  };
  BENCHMARK("benchmark_launder")
  {
    return benchmark_launder(r);
  };
}
