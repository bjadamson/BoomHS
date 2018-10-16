#include <extlibs/glm.hpp>

struct TEST
{
  // Commenting out this DUMMY field removes the memory violation.
  //
  // WHY? Static-Analysis BUG?
  // or
  // Compiler BUG?
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
  // This does NOT cause a segfault
  {
    glm::mat4 mat4;
    glm::inverse(mat4);
  }

  // This DOES cause a segfault
  {
    auto const TEST       = create_TEST();
    glm::mat4 const& mat4 = TEST.mat4;
    glm::inverse(mat4);
  }
  return EXIT_SUCCESS;
}
