// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/common.hpp>

#include <atomic>

PNTR_NAMESPACE_BEGIN


template<typename t_value>
class CounterThreadSafe
{
public:
  using ValueType = t_value;

  explicit CounterThreadSafe(t_value const p_init_value) noexcept
  : m_value(p_init_value)
  {}

  // Return the current count.
  t_value
  get_count() const noexcept
  {
    return m_value.load(std::memory_order_relaxed);
  }

  // Increment the counter by the given value and return the previous value.
  t_value
  increment(t_value const p_value) noexcept
  {
    return m_value.fetch_add(p_value, std::memory_order_relaxed);
  }

  // Decrement the counter by the given value and return the previous value.
  t_value
  decrement(t_value const p_value) noexcept
  {
    return m_value.fetch_sub(p_value, std::memory_order_acq_rel);
  }

  // If the counter equals 'p_expected' replace it with the desired value and return true.
  // Otherwise save the counter value in 'p_expected' and return false.
  // Might fail sporadically even if the counter equals expected.
  bool
  compare_exchange_weak(t_value & p_expected, t_value const p_desired) noexcept
  {
    return m_value.compare_exchange_weak(p_expected, p_desired, std::memory_order_relaxed);
  }

private:
  std::atomic<t_value> m_value;
};


PNTR_NAMESPACE_END
