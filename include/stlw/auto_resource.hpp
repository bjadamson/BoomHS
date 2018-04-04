#pragma once
#include <stlw/type_macros.hpp>

namespace stlw
{

// A wrapper around a resource.
//
// Internally tracks when it is time to destroy the resource.
//
// The last non-moved-from instance of this type owns the resource, and will invoke the
// destroy() function.
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
//     // new_ar will handle calling destroy() on r, internally, automatically when new_ar is
//     // destroyed.
template <typename R>
class AutoResource
{
  R    resource_;
  bool should_destroy_ = true;

public:
  NO_COPY(AutoResource);
  explicit AutoResource(R&& resource)
      : resource_(MOVE(resource))
  {
  }

  ~AutoResource()
  {
    if (should_destroy_) {
      resource_.destroy();
    }
  }

  AutoResource& operator=(AutoResource&& other)
  {
    should_destroy_ = other.should_destroy_;
    resource_       = MOVE(other.resource_);

    // This instance takes ownership of the resource from "other"
    other.should_destroy_ = false;
    return *this;
  }

  AutoResource(AutoResource&& other)
      : resource_(MOVE(other.resource_))
      , should_destroy_(other.should_destroy_)
  {
    // This instance takes ownership of the resource from "other"
    other.should_destroy_ = false;
  }

  auto&       resource() { return resource_; }
  auto const& resource() const { return resource_; }
};

} // namespace stlw
