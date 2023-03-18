// Copyright (c) 2023 John Plate (john.plate@gmx.com)

#pragma once

#include <pntr/common.hpp>

PNTR_NAMESPACE_BEGIN


template<typename t_value>
class CounterThreadUnsafe
{
public:
  using ValueType = t_value;

  explicit CounterThreadUnsafe(t_value const p_init_value) noexcept
  : m_value(p_init_value)
  {}

  // Return the current count.
  t_value
  get_count() const noexcept
  {
    return m_value;
  }

  // Increment the counter by the given value and return the previous value.
  t_value
  increment(t_value const p_value) noexcept
  {
    t_value const previous = m_value;
    m_value += p_value;
    return previous;
  }

  // Decrement the counter by the given value and return the previous value.
  t_value
  decrement(t_value const p_value) noexcept
  {
    t_value const previous = m_value;
    m_value -= p_value;
    return previous;
  }

  // If the counter equals 'p_expected' replace it with the desired value and return true.
  // Otherwise save the counter value in 'p_expected' and return false.
  bool
  compare_exchange_weak(t_value & p_expected, t_value const p_desired) noexcept
  {
    if (m_value == p_expected)
    {
      m_value = p_desired;
      return true;
    }
    p_expected = m_value;
    return false;
  }

private:
  t_value m_value;
};


PNTR_NAMESPACE_END
