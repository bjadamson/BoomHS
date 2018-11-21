#include <extlibs/glm.hpp>

// This program USED to cause a segfault, it was discovered using the clang 6.0 and static-analysi.
// Upgrading from clang 6.0 to 8.0 fixes the segfault, it seems like a codegen issue.
//
// Leaving this test here for the future, when this game gets ported to other platforms with
// different compilers, this test program can be run again (with static analysis) to help isolate a
// recurring bug.
struct TEST
{
  // Under the specific scenario, comenting out this next line FIXES the memory corruption (which
  // is suspsected to be caused by a codegen bug in clang 6.0).
  //
  // Commenting out this DUMMY field removes the memory violation.
  int DUMMY = 0;
  glm::mat4 const mat4;

  TEST(glm::mat4 const& m) : mat4(m) {}
};

auto
create_TEST()
{
  glm::mat4 ortho_pm;

  return TEST{ortho_pm};
}

int
main(int argc, char **argv)
{
  // Under the scenario in question, this does NOT cause a segfault.
  {
    glm::mat4 mat4;
    glm::inverse(mat4);
  }

  // Under the scenario in question, this DOES cause a segfault.
  {
    auto const TEST       = create_TEST();
    glm::mat4 const& mat4 = TEST.mat4;
    glm::inverse(mat4);
  }
  return EXIT_SUCCESS;
}
