#include <pntr/pntr.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib> // std::rand
#include <iostream>
#include <stack>


// Simple object pool which receives expired objects from the Deleter
template<class t_shared>
class Pool
{
public:
  static std::uint32_t
  size() noexcept
  {
    return static_cast<std::uint32_t>(get_stack().size());
  }

  static pntr::SharedPtr<t_shared>
  fetch_object() noexcept
  {
    // If the stack is not empty, try to revive the object on top.
    // Use a loop as this can theoretically fail until the stack is empty,
    // but it would only fail for managed objects, which should never
    // end up in the stack.
    while (!get_stack().empty())
    {
      // Re-initialize the control block of the top expired object.
      if (get_stack().top()->pntr_try_revive())
      {
        // Transfer ownership of the top object to a shared pointer.
        pntr::SharedPtr<t_shared> shared_ptr(get_stack().top().release());
        // Delete the now empty top stack element.
        get_stack().pop();
        // Return the object which was successfully removed from the pool.
        return shared_ptr;
      }
      else
      {
        // Release the top object which shouldn't be in the stack.
        get_stack().top().release();
        // Delete the now empty top stack element.
        get_stack().pop();
      }
    }

    // No object is available in the stack, so create and return a new object.
    // pntr::make_shared_nothrow won't throw an exception if the memory allocation or the
    // constructor fails, so the caller should check if the returned pointer is valid.
    return pntr::make_shared_nothrow<t_shared>();
  }

  // This deleter returns the object to the pool instead of deleting it.
  struct Deleter
  {
    void
    operator()(t_shared * p_shared) const noexcept
    {
      get_stack().emplace(p_shared);
    }
  };

private:
  using Stack = std::stack<std::unique_ptr<t_shared>>;

  static Stack &
  get_stack() noexcept
  {
    // The stack is destroyed at exit time. The objects should not
    // reference any other object that is destroyed at exit time.
    static Stack s_stack;
    return s_stack;
  }
};


// Object class contains IntruderNew, configured to use the Pool::Deleter.
// Thread safety is disabled and not required for the single-threaded example.
class Object: public pntr::IntruderNew<Object, pntr::ThreadUnsafe, std::uint32_t, 32u, Pool<Object>::Deleter>
{
public:
  std::uint32_t m_lifetime{};
};


inline std::uint32_t
get_random_digit()
{
  return static_cast<std::uint32_t>(1 + std::rand() % 9);
}

// Program which randomly creates and destroys objects in an array and outputs
// the number of objects in the array and the pool. Try to find a pattern. :)
// Notice that the sum of the objects in the array and the pool never exceed the
// maximum number of elements in the array.
int
main()
{
  std::srand(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

  std::array<pntr::SharedPtr<Object>, 9u> objects;
  std::uint32_t create = 1u;

  for (std::uint32_t row = 0u; row < 80u; ++row)
  {
    for (std::uint32_t col = 0u; col < 25u; ++col)
    {
      std::uint32_t alive = 0u;
      for (auto & object_ptr: objects)
      {
        if (object_ptr && --object_ptr->m_lifetime == 0u)
        {
          object_ptr.reset(); // This returns the object to the pool.
        }
        if (!object_ptr && --create == 0u)
        {
          object_ptr = Pool<Object>::fetch_object();
          if (object_ptr)
          {
            object_ptr->m_lifetime = get_random_digit();
          }
          create = get_random_digit();
        }
        if (object_ptr)
        {
          ++alive;
        }
      }
      std::cout << ' ' << alive << Pool<Object>::size() << std::flush;
    }
    std::cout << std::endl;
  }

  return 0;
}
