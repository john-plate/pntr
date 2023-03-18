#include "tests-common.hpp"


template<typename t_storage, unsigned t_usage_bits, unsigned t_weak_bits, unsigned t_offset_bits, unsigned t_size_bits,
         unsigned t_align_bits, typename t_usage, typename t_weak, typename t_data>
class ControlDataSafe
: public pntr::ControlData<pntr::CounterThreadSafe, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits,
                           t_align_bits>
{
  using Base = pntr::ControlData<pntr::CounterThreadSafe, t_storage, t_usage_bits, t_weak_bits, t_offset_bits,
                                 t_size_bits, t_align_bits>;

public:
  ControlDataSafe()
  : Base(Base::get_max_user())
  {}

  static_assert(std::is_same_v<t_usage, typename Base::UsageValueType>);
  static_assert(std::is_same_v<t_weak, typename Base::WeakValueType>);
  static_assert(std::is_same_v<t_data, typename Base::DataValueType>);
};

template<typename t_storage, unsigned t_usage_bits, unsigned t_weak_bits, unsigned t_offset_bits, unsigned t_size_bits,
         unsigned t_align_bits, typename t_usage, typename t_weak, typename t_data>
class ControlDataUnsafe
: public pntr::ControlData<pntr::CounterThreadUnsafe, t_storage, t_usage_bits, t_weak_bits, t_offset_bits, t_size_bits,
                           t_align_bits>
{
  using Base = pntr::ControlData<pntr::CounterThreadUnsafe, t_storage, t_usage_bits, t_weak_bits, t_offset_bits,
                                 t_size_bits, t_align_bits>;

public:
  ControlDataUnsafe()
  : Base(Base::get_max_user())
  {}

  static_assert(std::is_same_v<t_usage, typename Base::UsageValueType>);
  static_assert(std::is_same_v<t_weak, typename Base::WeakValueType>);
  static_assert(std::is_same_v<t_data, typename Base::DataValueType>);
};

using u08 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
inline constexpr unsigned sb = pntr::shared_bits;


// clang-format off
TEMPLATE_PRODUCT_TEST_CASE_SIG(TEST_PREFIX "ControlData", "",
                               ((typename t_storage, unsigned t_usage_bits, unsigned t_weak_bits,
                                 unsigned t_offset_bits, unsigned t_size_bits, unsigned t_align_bits,
                                 typename t_usage, typename t_weak, typename t_data),
                                t_storage, t_usage_bits, t_weak_bits, t_offset_bits,
                                t_size_bits, t_align_bits, t_usage, t_weak, t_data),
                               (ControlDataSafe, ControlDataUnsafe),
                               ((u08,  8u,  0u,  0u, 0u, 0u, u08, u08, u08),
                                (u08,  5u,  3u,  0u, 0u, 0u, u08, u08, u08),
                                (u08,  5u,  3u,  sb, 0u, 0u, u08, u08, u08),
                                (u08,  4u,  4u,  0u, 0u, 0u, u08, u08, u08),
                                (u08,  4u,  4u,  sb, 0u, 0u, u08, u08, u08),
                                (u08,  4u,  2u,  2u, 0u, 0u, u08, u08, u08),
                                (u08,  4u,  2u,  sb, 0u, 0u, u08, u08, u08),
                                (u08,  3u,  2u,  1u, 1u, 1u, u08, u08, u08),
                                (u16, 16u,  0u,  0u, 0u, 0u, u16, u16, u16),
                                (u16,  9u,  7u,  0u, 0u, 0u, u16, u16, u16),
                                (u16,  9u,  7u,  sb, 0u, 0u, u16, u16, u16),
                                (u16,  8u,  8u,  0u, 0u, 0u, u08, u08, u08),
                                (u16,  8u,  8u,  sb, 0u, 0u, u08, u08, u08),
                                (u16,  8u,  4u,  4u, 0u, 0u, u08, u08, u08),
                                (u16,  8u,  4u,  sb, 0u, 0u, u08, u08, u08),
                                (u16,  4u,  4u,  2u, 2u, 2u, u16, u16, u16),
                                (u32, 32u,  0u,  0u, 0u, 0u, u32, u32, u32),
                                (u32, 17u, 15u,  0u, 0u, 0u, u32, u32, u32),
                                (u32, 17u, 15u,  sb, 0u, 0u, u32, u32, u32),
                                (u32, 16u, 16u,  0u, 0u, 0u, u16, u16, u16),
                                (u32, 16u, 16u,  sb, 0u, 0u, u16, u16, u16),
                                (u32, 16u,  8u,  8u, 0u, 0u, u16, u08, u08),
                                (u32, 16u,  8u,  sb, 0u, 0u, u16, u08, u08),
                                (u32,  8u,  8u,  4u, 4u, 4u, u08, u08, u16),
                                (u32,  8u, 16u,  8u, 0u, 0u, u32, u32, u32),
                                (u64, 64u,  0u,  0u, 0u, 0u, u64, u64, u64),
                                (u64, 33u, 31u,  0u, 0u, 0u, u64, u64, u64),
                                (u64, 33u, 31u,  sb, 0u, 0u, u64, u64, u64),
                                (u64, 32u, 32u,  0u, 0u, 0u, u32, u32, u32),
                                (u64, 32u, 32u,  sb, 0u, 0u, u32, u32, u32),
                                (u64, 32u, 16u, 16u, 0u, 0u, u32, u16, u16),
                                (u64, 32u, 16u,  sb, 0u, 0u, u32, u16, u16),
                                (u64, 16u, 16u,  8u, 8u, 8u, u16, u16, u32),
                                (u64, 16u, 32u, 16u, 0u, 0u, u64, u64, u64)))
// clang-format on
{
  using U = typename TestType::UsageValueType;
  using W = typename TestType::WeakValueType;

  //  static_assert(sizeof(TestType) == sizeof(typename TestType::StorageType));
  static_assert(TestType::SupportsWeak::value == (TestType::s_weak_bits != 0u));

  // Initialization
  TestType data;
  REQUIRE(data.try_set_offset(TestType::get_max_offset()) != TestType::s_shared_offset);
  REQUIRE(data.try_set_size(TestType::get_max_size()));
  REQUIRE(data.try_set_align(TestType::get_max_align()));
  REQUIRE(data.is_uncontrolled());

  // Taking control
  REQUIRE(data.try_control() == pntr::ControlStatus::e_acquired);
  REQUIRE(data.use_count() == static_cast<U>(1u));
  if constexpr (TestType::SupportsWeak::value)
  {
    REQUIRE(data.weak_count() == static_cast<W>(1u));
  }

  // Sharing control
  REQUIRE(data.try_add_ref());
  REQUIRE(data.use_count() == static_cast<U>(2u));
  REQUIRE(data.try_control() == pntr::ControlStatus::e_shared);
  REQUIRE(data.use_count() == static_cast<U>(3u));
  data.add_ref();
  REQUIRE(data.use_count() == static_cast<U>(4u));
  if constexpr (TestType::SupportsWeak::value)
  {
    data.weak_add_ref();
    REQUIRE(data.weak_count() == static_cast<W>(2u));
    REQUIRE_FALSE(data.weak_release());
    REQUIRE(data.weak_count() == static_cast<W>(1u));
  }
  REQUIRE_FALSE(data.release());
  REQUIRE(data.use_count() == static_cast<U>(3u));
  REQUIRE_FALSE(data.release());
  REQUIRE(data.use_count() == static_cast<U>(2u));
  REQUIRE_FALSE(data.release());
  REQUIRE(data.use_count() == static_cast<U>(1u));

  // Destroy
  REQUIRE(data.release());
  REQUIRE(data.use_count() == static_cast<U>(0u));
  REQUIRE_FALSE(data.try_add_ref());
  REQUIRE(data.use_count() == static_cast<U>(0u));
  REQUIRE(data.try_control() == pntr::ControlStatus::e_invalid);
  REQUIRE(data.use_count() == static_cast<U>(0u));

  // Time to use offset shared with usage counter
  if constexpr (TestType::s_shared_offset)
  {
    REQUIRE(data.try_set_offset(TestType::get_max_offset()));
  }

  // Deallocate
  REQUIRE(data.get_offset() == TestType::get_max_offset());
  REQUIRE(data.get_size() == TestType::get_max_size());
  REQUIRE(data.get_align() == TestType::get_max_align());
  REQUIRE(data.get_user() == TestType::get_max_user());
  if constexpr (TestType::SupportsWeak::value)
  {
    REQUIRE(data.weak_release());
    REQUIRE(data.weak_count() == static_cast<U>(0u));
  }
}
