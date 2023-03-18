#include "tests-common.hpp"


struct IntruderAlloc
{
  template<class t_shared_base>
  using Intruder = pntr::IntruderAlloc<t_shared_base, pntr::ThreadUnsafe>;
};


TEMPLATE_TEST_CASE(TEST_PREFIX "Empty WeakPtr", "", IntruderAlloc)
{
  using Minimal = MinimalSharedClass<TestType::template Intruder>;
  using Shared = StaticDerivedStaticClass<TestType::template Intruder>;
  using SharedBase = typename Shared::PntrSharedBase;

  static_assert(std::is_same_v<typename pntr::WeakPtr<Minimal>::element_type, Minimal>);

  SECTION("Member functions")
  {
    pntr::WeakPtr<Minimal> m, n;
    pntr::WeakPtr<SharedBase> b;
    pntr::WeakPtr<Shared> s;

    REQUIRE(pntr::WeakPtr<Minimal>().is_empty()); // default constructor

    REQUIRE(pntr::WeakPtr<Minimal>(m).is_empty());                        // copy constructor
    REQUIRE(pntr::WeakPtr<Minimal>(pntr::WeakPtr<Minimal>()).is_empty()); // move constructor

    REQUIRE(pntr::WeakPtr<SharedBase>(s).is_empty());                       // templated copy constructor
    REQUIRE(pntr::WeakPtr<SharedBase>(pntr::WeakPtr<Shared>()).is_empty()); // templated move constructor

    REQUIRE(pntr::WeakPtr<SharedBase>(pntr::SharedPtr<Shared>()).is_empty()); // templated constructor from SharedPtr

    m = n; // copy assignment
    REQUIRE(m.is_empty());
    m = pntr::WeakPtr<Minimal>(); // move assignment
    REQUIRE(m.is_empty());
    b = s; // templated copy assignment
    REQUIRE(b.is_empty());
    b = pntr::WeakPtr<Shared>(); // templated move assignment
    REQUIRE(b.is_empty());
    b = pntr::SharedPtr<Shared>(); // templated copy assignment from SharedPtr
    REQUIRE(b.is_empty());

    m.reset();
    REQUIRE(m.is_empty());
    m.swap(n);
    REQUIRE(m.is_empty());

    REQUIRE(m.use_count() == 0u);
    REQUIRE(m.weak_count() == 0u);
    REQUIRE_FALSE(m.expired());
    REQUIRE(m.lock().get() == nullptr);

    REQUIRE_FALSE(m.owner_before(pntr::WeakPtr<Minimal>{}));
    REQUIRE_FALSE(m.owner_before(pntr::SharedPtr<Minimal>{}));

    std::swap(m, n);
    REQUIRE(m.is_empty());
  }
}


TEMPLATE_TEST_CASE(TEST_PREFIX "Valid WeakPtr", "", IntruderAlloc)
{
  using Minimal = MinimalSharedClass<TestType::template Intruder>;
  using Shared = StaticDerivedStaticClass<TestType::template Intruder>;
  using SharedBase = typename Shared::PntrSharedBase;

  SECTION("Member functions")
  {
    Minimal::get_construct_count() = 0u;
    Minimal::get_destroy_count() = 0u;
    Shared::get_construct_count() = 0u;
    Shared::get_destroy_count() = 0u;

    pntr::SharedPtr<Minimal> m = pntr::make_shared<Minimal>();
    REQUIRE(m.weak_count() == 1u);
    void * mp = Minimal::PntrAllocator::s_storage;
    REQUIRE(mp != nullptr);
    pntr::SharedPtr<Shared> s = pntr::make_shared<Shared>();
    REQUIRE(s.weak_count() == 1u);
    void * sp = Shared::PntrAllocator::s_storage;
    REQUIRE(sp != nullptr);

    pntr::WeakPtr<Shared> ws(s);
    REQUIRE(ws.weak_count() == 2u);
    pntr::WeakPtr<SharedBase> wb(s); // templated copy construction from SharedPtr
    REQUIRE(wb.weak_count() == 3u);

    pntr::WeakPtr<Minimal> wm(m);
    REQUIRE(wm.weak_count() == 2u);
    pntr::WeakPtr<Minimal> wn(wm); // copy constructor
    REQUIRE(wn.weak_count() == 3u);
    REQUIRE(pntr::WeakPtr<Minimal>(std::move(wn)).weak_count() == 3u); // move constructor
    REQUIRE(wn.is_empty());

    pntr::WeakPtr<SharedBase> wc(ws); // templated copy constructor
    REQUIRE(wc.weak_count() == 4u);
    pntr::WeakPtr<Shared> wt(ws);
    REQUIRE(pntr::WeakPtr<SharedBase>(std::move(wt)).weak_count() == 5u); // templated move constructor
    REQUIRE(wt.is_empty());

    wn = wm; // copy assignment
    REQUIRE(wn.weak_count() == 3u);
    pntr::WeakPtr<Minimal> wo;
    wo = std::move(wn); // move assignment
    REQUIRE(wo.weak_count() == 3u);
    REQUIRE(wn.is_empty());

    wb = ws; // templated copy assignment
    REQUIRE(wb.weak_count() == 4u);
    wt = ws;
    wb = std::move(wt); // templated move assignment
    REQUIRE(wb.weak_count() == 4u);
    REQUIRE(wt.is_empty());

    wb = s; // templated copy assignment from SharedPtr
    REQUIRE(wb.weak_count() == 4u);

    wm.reset();
    REQUIRE(wm.is_empty());
    REQUIRE(wo.weak_count() == 2u);
    pntr::WeakPtr<Shared> wu(ws);
    wt.swap(wu);
    REQUIRE(wt.weak_count() == 5u);
    REQUIRE(wu.is_empty());

    REQUIRE(ws.use_count() == 1u);
    REQUIRE_FALSE(ws.expired());
    REQUIRE(ws.lock().weak_count() == 5u);

    REQUIRE_FALSE(wo.owner_before(pntr::WeakPtr<Minimal>{}));
    REQUIRE_FALSE(wo.owner_before(pntr::SharedPtr<Minimal>{}));

    std::swap(wt, wu);
    REQUIRE(wt.is_empty());
    REQUIRE(wu.weak_count() == 5u);

    wm = m->weak_from_this();
    REQUIRE(wm.weak_count() == 3u);

    Minimal::PntrAllocator::s_storage = mp;
    m.reset();
    REQUIRE(wm.expired());
    REQUIRE(wm.lock().get() == nullptr);
    REQUIRE(wm.weak_count() == 2u);
    wo.reset();
    REQUIRE(wm.weak_count() == 1u);
    wm.reset();
    REQUIRE(wm.is_empty());
    REQUIRE(Minimal::PntrAllocator::s_storage == nullptr);

    Minimal::PntrAllocator::s_storage = sp;
    s.reset();
    REQUIRE(wb.expired());
    REQUIRE(wb.lock().get() == nullptr);
    REQUIRE(wb.weak_count() == 4u);
    wu.reset();
    REQUIRE(wb.weak_count() == 3u);
    ws.reset();
    REQUIRE(wb.weak_count() == 2u);
    wc.reset();
    REQUIRE(wb.weak_count() == 1u);
    wb.reset();
    REQUIRE(wb.is_empty());
    REQUIRE(Minimal::PntrAllocator::s_storage == nullptr);

    REQUIRE(Minimal::get_construct_count() == 1u);
    REQUIRE(Minimal::get_destroy_count() == 1u);
    REQUIRE(Shared::get_construct_count() == 1u);
    REQUIRE(Shared::get_destroy_count() == 1u);
  }
}
