#pragma once
#include <stlw/type_macros.hpp>

namespace opengl
{

// A wrapper around an opengl resource.
//
// Internally tracks when it is time to deallocate the resource.
//
// The last non-moved-from instance of this type owns the resource, and will invoke the
// deallocate() function.
//
// It is undefined behavior to reference the internal reference after an AutoResource has been
// moved-from.
//
// To transfer the resource to a new owner, move the instance.
// ie:
//     auto r = create_resource();
//     r.init();
//
//     ...
//
//     // take ownership of the resource
//     auto ar = AutoResource{std::move(r)};
//
//     // give ownership to a new variable, it is undefined behavior to continue to reference ar.
//     auto new_ar = std::move(r);
//
//     // new_ar will handle calling deallocate() on r, internally, automatically when new_ar is
//     // destroyed.
template<typename R>
class AutoResource
{
  R resource_;
  bool should_destroy_ = true;
public:

  explicit AutoResource(R &&resource)
    : resource_(MOVE(resource))
  {
  }

  ~AutoResource()
  {
    if (should_destroy_) {
      resource_.deallocate();
    }
  }

  auto const& resource() const { return resource_; }

  NO_COPY(AutoResource);
  NO_MOVE_ASSIGN(AutoResource);
  AutoResource(AutoResource &&other)
    : resource_(MOVE(other.resource_))
    , should_destroy_(other.should_destroy_)
  {
    other.should_destroy_ = false;
  }
};

} // ns opengl
