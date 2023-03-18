#include "tests-common.hpp"


TEMPLATE_PRODUCT_TEST_CASE(TEST_PREFIX "Counter", "", (pntr::CounterThreadSafe, pntr::CounterThreadUnsafe),
                           (std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, std::int8_t, std::int16_t,
                            std::int32_t, std::int64_t))
{
  using T = typename TestType::ValueType;

  TestType c(static_cast<T>(23));
  REQUIRE(c.get_count() == static_cast<T>(23));
  c.increment(static_cast<T>(1));
  REQUIRE(c.get_count() == static_cast<T>(24));
  c.increment(static_cast<T>(18));
  REQUIRE(c.get_count() == static_cast<T>(42));
  c.decrement(static_cast<T>(7));
  REQUIRE(c.get_count() == static_cast<T>(35));
  c.decrement(static_cast<T>(17));
  REQUIRE(c.get_count() == static_cast<T>(18));
  T expected = static_cast<T>(5);
  REQUIRE_FALSE(c.compare_exchange_weak(expected, static_cast<T>(7)));
  REQUIRE(expected == static_cast<T>(18));
  while (!c.compare_exchange_weak(expected, static_cast<T>(7)))
  {
    REQUIRE(expected == static_cast<T>(18));
    REQUIRE(c.get_count() == static_cast<T>(18));
  }
  REQUIRE(expected == static_cast<T>(18));
  REQUIRE(c.get_count() == static_cast<T>(7));
}
