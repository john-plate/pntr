#include "tests-common.hpp"

#include <sstream>


struct IntruderNew
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderNew<t_shared_base, pntr::ThreadUnsafe>;
};

struct IntruderAlloc
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderAlloc<t_shared_base, pntr::ThreadUnsafe>;
};


template<class t_shared_ptr, typename = void>
struct HasWeakConstructor: std::false_type
{};

template<class t_shared_ptr>
struct HasWeakConstructor<t_shared_ptr,
                          std::void_t<decltype(t_shared_ptr(pntr::WeakPtr<typename t_shared_ptr::element_type>()))>>
: std::true_type
{};


TEMPLATE_TEST_CASE(TEST_PREFIX "Empty SharedPtr", "", IntruderNew, IntruderAlloc)
{
  using Minimal = MinimalSharedClass<TestType::template Intruder>;
  using Shared = StaticDerivedPolymorphicClass<TestType::template Intruder>;
  using SharedBase = typename Shared::PntrSharedBase;
  using Deleter = typename Shared::PntrDeleter;

  static_assert(std::is_same_v<typename pntr::SharedPtr<Minimal>::element_type, Minimal>);
  static_assert(std::is_same_v<typename pntr::SharedPtr<Minimal>::weak_type,
                               std::conditional_t<Minimal::PntrSupportsWeak::value, pntr::WeakPtr<Minimal>, void>>);

  static_assert(HasWeakConstructor<pntr::SharedPtr<Minimal>>::value == Minimal::PntrSupportsWeak::value);

  SECTION("Member functions")
  {
    pntr::SharedPtr<Minimal> m, n;
    pntr::SharedPtr<SharedBase> b;
    pntr::SharedPtr<Shared> s;

    REQUIRE(pntr::SharedPtr<Minimal>().get() == nullptr);        // default constructor
    REQUIRE(pntr::SharedPtr<Minimal>(nullptr).get() == nullptr); // nullptr constructor

    REQUIRE(pntr::SharedPtr<SharedBase>(s.get()).get() == nullptr); // templated pointer constructor
    if constexpr (!std::is_void_v<Deleter>)
    {
      // Templated pointer constructor with deleter
      REQUIRE(pntr::SharedPtr<SharedBase>(s.get(), Deleter()).get() == nullptr);
      // Templated constructor from std::unique_ptr
      REQUIRE(pntr::SharedPtr<SharedBase>(std::unique_ptr<Shared, Deleter>()).get() == nullptr);
    }

    REQUIRE(pntr::SharedPtr<Minimal>(m).get() == nullptr);                          // copy constructor
    REQUIRE(pntr::SharedPtr<Minimal>(pntr::SharedPtr<Minimal>()).get() == nullptr); // move constructor

    REQUIRE(pntr::SharedPtr<SharedBase>(s).get() == nullptr);                         // templated copy constructor
    REQUIRE(pntr::SharedPtr<SharedBase>(pntr::SharedPtr<Shared>()).get() == nullptr); // templated move constructor

    if constexpr (SharedBase::PntrSupportsWeak::value)
    {
      // templated constructor from WeakPtr
      REQUIRE_THROWS_AS(pntr::SharedPtr<SharedBase>(pntr::WeakPtr<Shared>()), std::bad_weak_ptr);
    }

    m = n; // copy assignment
    REQUIRE(m.get() == nullptr);
    m = pntr::SharedPtr<Minimal>(); // move assignment
    REQUIRE(m.get() == nullptr);
    b = s; // templated copy assignment
    REQUIRE(b.get() == nullptr);
    b = pntr::SharedPtr<Shared>(); // templated move assignment
    REQUIRE(b.get() == nullptr);

    REQUIRE((m ? false : true)); // contextual conversion with bool operator
    REQUIRE(m.get() == nullptr); // get

    REQUIRE(m.use_count() == 0u);
    REQUIRE(m.weak_count() == 0u);
    REQUIRE_FALSE(m.owner_before(pntr::SharedPtr<Minimal>{}));
    if constexpr (Minimal::PntrSupportsWeak::value)
    {
      REQUIRE_FALSE(m.owner_before(pntr::WeakPtr<Minimal>{}));
    }

    m.reset();
    REQUIRE(m.get() == nullptr);
    m.reset(m.get());
    REQUIRE(m.get() == nullptr);
    if constexpr (!std::is_void_v<Deleter>)
    {
      b.reset(s.get(), Deleter());
      REQUIRE(b.get() == nullptr);
      b.reset(std::unique_ptr<Shared, Deleter>());
      REQUIRE(b.get() == nullptr);
    }
    m.swap(n);
    REQUIRE(m.get() == nullptr);
    std::swap(m, n);
    REQUIRE(m.get() == nullptr);
    std::stringstream stream;
    stream << m;
    REQUIRE_FALSE(stream.str().empty());
  }

  SECTION("Compare two SharedPtr of same type")
  {
    pntr::SharedPtr<Minimal> l, r;
    REQUIRE_FALSE(l.owner_before(r));
    REQUIRE_FALSE(r.owner_before(l));
    REQUIRE(l == r);
    REQUIRE_FALSE(l != r);
    REQUIRE_FALSE(l < r);
    REQUIRE_FALSE(l > r);
    REQUIRE(l <= r);
    REQUIRE(l >= r);
  }

  SECTION("Compare two SharedPtr of different types")
  {
    pntr::SharedPtr<SharedBase> b;
    pntr::SharedPtr<Shared> s;
    REQUIRE_FALSE(b.owner_before(s));
    REQUIRE_FALSE(s.owner_before(b));
    REQUIRE(b == s);
    REQUIRE_FALSE(b != s);
    REQUIRE_FALSE(b < s);
    REQUIRE_FALSE(b > s);
    REQUIRE(b <= s);
    REQUIRE(b >= s);
  }

  SECTION("Compare SharedPtr with null pointer")
  {
    pntr::SharedPtr<Minimal> m;
    REQUIRE(m == nullptr);
    REQUIRE(nullptr == m);
    REQUIRE_FALSE(m != nullptr);
    REQUIRE_FALSE(nullptr != m);
    REQUIRE_FALSE(m < nullptr);
    REQUIRE_FALSE(nullptr < m);
    REQUIRE_FALSE(m > nullptr);
    REQUIRE_FALSE(nullptr > m);
    REQUIRE(m <= nullptr);
    REQUIRE(nullptr <= m);
    REQUIRE(m >= nullptr);
    REQUIRE(nullptr >= m);
  }

  SECTION("SharedPtr casts")
  {
    pntr::SharedPtr<SharedBase> b;
    pntr::SharedPtr<Minimal const> m;
    REQUIRE(pntr::static_pointer_cast<Shared>(b).get() == nullptr);
    REQUIRE(pntr::static_pointer_cast<Shared>(std::move(b)).get() == nullptr);
    REQUIRE(pntr::dynamic_pointer_cast<Shared>(b).get() == nullptr);
    REQUIRE(pntr::dynamic_pointer_cast<Shared>(std::move(b)) == nullptr);
    REQUIRE(pntr::const_pointer_cast<Minimal>(m).get() == nullptr);
    REQUIRE(pntr::const_pointer_cast<Minimal>(std::move(m)).get() == nullptr);
  }
}


TEMPLATE_TEST_CASE(TEST_PREFIX "Valid SharedPtr", "", IntruderNew, IntruderAlloc)
{
  using Minimal = MinimalSharedClass<TestType::template Intruder>;
  using Shared = StaticDerivedPolymorphicClass<TestType::template Intruder>;
  using SharedBase = typename Shared::PntrSharedBase;
  using Deleter = typename Shared::PntrDeleter;

  using VirtualShared = VirtualDerivedPolymorphicClass<TestType::template Intruder>;
  using VirtualSharedBase = typename VirtualShared::PntrSharedBase;

  SECTION("Member functions")
  {
    Minimal::get_construct_count() = 0u;
    Minimal::get_destroy_count() = 0u;
    Shared::get_construct_count() = 0u;
    Shared::get_destroy_count() = 0u;

    pntr::SharedPtr<Minimal> m = pntr::make_shared_nothrow<Minimal>();
    REQUIRE(m.get() != nullptr);
    pntr::SharedPtr<Shared> s = pntr::make_shared_nothrow<Shared>();
    REQUIRE(s.get() != nullptr);
    pntr::SharedPtr<SharedBase> b(s.get()); // templated pointer constructor, share
    REQUIRE(b.get() != nullptr);

    if constexpr (!std::is_void_v<Deleter>)
    {
      // Templated pointer constructor with deleter
      REQUIRE(pntr::SharedPtr<SharedBase>(s.get(), Deleter()).get() != nullptr);
      // Templated constructor from std::unique_ptr
      REQUIRE(pntr::SharedPtr<SharedBase>(std::unique_ptr<Shared, Deleter>(s.get())).get() != nullptr);
    }

    pntr::SharedPtr<Minimal> n(m); // copy constructor
    REQUIRE(n.get() == m.get());
    REQUIRE(pntr::SharedPtr<Minimal>(std::move(n)).get() == m.get()); // move constructor
    REQUIRE(n.get() == nullptr);

    pntr::SharedPtr<SharedBase> c(s); // templated copy constructor
    REQUIRE(c.get() == s.get());
    pntr::SharedPtr<Shared> t(s);
    REQUIRE(pntr::SharedPtr<SharedBase>(std::move(t)).get() == c.get()); // templated move constructor
    REQUIRE(t.get() == nullptr);

    if constexpr (SharedBase::PntrSupportsWeak::value)
    {
      // templated constructor from WeakPtr
      REQUIRE(pntr::SharedPtr<SharedBase>(pntr::WeakPtr<Shared>(s)).get() != nullptr);
    }

    n = m; // copy assignment
    REQUIRE(n.get() == m.get());
    pntr::SharedPtr<Minimal> o;
    o = std::move(n); // move assignment
    REQUIRE(o.get() == m.get());
    REQUIRE(n.get() == nullptr);
    b = s; // templated copy assignment
    REQUIRE(b.get() == s.get());
    t = s;
    b = std::move(t); // templated move assignment
    REQUIRE(b.get() == s.get());
    REQUIRE(t.get() == nullptr);

    REQUIRE((m ? true : false)); // contextual conversion with bool operator
    REQUIRE(m.get() != nullptr); // get

    REQUIRE((*s).m_shared_value == 71u);
    REQUIRE(s->m_shared_value == 71u);

    REQUIRE(pntr::make_shared<Minimal>().use_count() == 1u);
    REQUIRE(pntr::make_shared<Minimal>().weak_count() == (SharedBase::PntrSupportsWeak::value ? 1u : 0u));
    REQUIRE_FALSE(m.owner_before(pntr::SharedPtr<Minimal>{}));
    if constexpr (SharedBase::PntrSupportsWeak::value)
    {
      REQUIRE_FALSE(m.owner_before(pntr::WeakPtr<Minimal>{}));
    }

    m.reset();
    REQUIRE(m.get() == nullptr);
    m.reset(o.get());
    REQUIRE(m.get() != nullptr);
    if constexpr (!std::is_void_v<Deleter>)
    {
      b.reset(s.get(), Deleter());
      REQUIRE(b.get() != nullptr);
      b.reset(std::unique_ptr<Shared, Deleter>(s.get()));
      REQUIRE(b.get() != nullptr);
    }
    pntr::SharedPtr<Shared> u(s);
    t.swap(u);
    REQUIRE(t.get() == s.get());
    REQUIRE(u.get() == nullptr);
    std::swap(t, u);
    REQUIRE(t.get() == nullptr);
    REQUIRE(u.get() == s.get());
    std::stringstream stream;
    stream << s;
    REQUIRE_FALSE(stream.str().empty());

    m = o->shared_from_this();
    REQUIRE(m.get() == o.get());

    b.reset();
    c.reset();
    m.reset();
    n.reset();
    o.reset();
    s.reset();
    t.reset();
    u.reset();
    REQUIRE(Minimal::get_construct_count() == 3u);
    REQUIRE(Minimal::get_destroy_count() == 3u);
    REQUIRE(Shared::get_construct_count() == 1u);
    REQUIRE(Shared::get_destroy_count() == 1u);
  }

  SECTION("Compare two SharedPtr of same type")
  {
    pntr::SharedPtr<Minimal> l = pntr::make_shared<Minimal>();
    pntr::SharedPtr<Minimal> r = pntr::make_shared<Minimal>();
    REQUIRE(l.owner_before(r) != r.owner_before(l));
    REQUIRE_FALSE(l == r);
    REQUIRE(l != r);
    REQUIRE((l < r) != (l > r));
    REQUIRE((l <= r) != (l >= r));
  }

  SECTION("Compare two SharedPtr of different types")
  {
    pntr::SharedPtr<SharedBase> b = pntr::make_shared<SharedBase>();
    pntr::SharedPtr<Shared> s = pntr::make_shared<Shared>();
    REQUIRE(b.owner_before(s) != s.owner_before(b));
    REQUIRE_FALSE(b == s);
    REQUIRE(b != s);
    REQUIRE((b < s) != (b > s));
    REQUIRE((b <= s) != (b >= s));
  }

  SECTION("Compare SharedPtr with null pointer")
  {
    pntr::SharedPtr<Minimal> m = pntr::make_shared<Minimal>();
    REQUIRE_FALSE(m == nullptr);
    REQUIRE_FALSE(nullptr == m);
    REQUIRE(m != nullptr);
    REQUIRE(nullptr != m);
    REQUIRE_FALSE(m < nullptr);
    REQUIRE(nullptr < m);
    REQUIRE(m > nullptr);
    REQUIRE_FALSE(nullptr > m);
    REQUIRE_FALSE(m <= nullptr);
    REQUIRE(nullptr <= m);
    REQUIRE(m >= nullptr);
    REQUIRE_FALSE(nullptr >= m);
  }

  SECTION("SharedPtr casts")
  {
    pntr::SharedPtr<Shared> s = pntr::make_shared<Shared>();
    pntr::SharedPtr<SharedBase> b(s);
    REQUIRE(pntr::static_pointer_cast<Shared>(b).get() == s.get());
    REQUIRE(pntr::static_pointer_cast<Shared>(std::move(b)).get() == s.get());
    REQUIRE(b.get() == nullptr);

    b = s;
    REQUIRE(pntr::dynamic_pointer_cast<Shared>(b).get() == s.get());
    REQUIRE(pntr::dynamic_pointer_cast<Shared>(std::move(b)).get() == s.get());
    REQUIRE(b.get() == nullptr);

    b = pntr::make_shared<SharedBase>();
    REQUIRE(pntr::dynamic_pointer_cast<Shared>(b).get() == nullptr);
    REQUIRE(pntr::dynamic_pointer_cast<Shared>(std::move(b)).get() == nullptr);
    REQUIRE(b.get() != nullptr);

    pntr::SharedPtr<VirtualShared> vs = pntr::make_shared<VirtualShared>();
    pntr::SharedPtr<VirtualSharedBase> vb(vs);
    REQUIRE(pntr::dynamic_pointer_cast<VirtualShared>(vb).get() == vs.get());
    REQUIRE(pntr::dynamic_pointer_cast<VirtualShared>(std::move(vb)).get() == vs.get());
    REQUIRE(vb.get() == nullptr);

    vb = pntr::make_shared<VirtualSharedBase>();
    REQUIRE(pntr::dynamic_pointer_cast<VirtualShared>(vb).get() == nullptr);
    REQUIRE(pntr::dynamic_pointer_cast<VirtualShared>(std::move(vb)).get() == nullptr);
    REQUIRE(vb.get() != nullptr);

    pntr::SharedPtr<Minimal const> m = pntr::make_shared<Minimal const>();
    pntr::SharedPtr<Minimal const> n(m);
    REQUIRE(pntr::const_pointer_cast<Minimal>(n).get() == m.get());
    REQUIRE(pntr::const_pointer_cast<Minimal>(std::move(n)).get() == m.get());
    REQUIRE(n.get() == nullptr);
  }
}
