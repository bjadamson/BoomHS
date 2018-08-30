#include <common/log.hpp>

#include <cstdlib>

int
main(int argc, char **argv)
{
  auto logger = common::LogFactory::make_stderr();
  LOG_ERROR("TEST HELLO WORLD");
  return EXIT_SUCCESS;
}
