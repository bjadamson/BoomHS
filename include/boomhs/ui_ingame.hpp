#pragma once
#include <stlw/algorithm.hpp>
#include <stlw/type_macros.hpp>

#include <string>
#include <vector>

namespace boomhs
{
struct EngineState;
class LevelManager;

class ChatBuffer
{
  static auto constexpr MAX_BUFFER_SIZE = 256;
  char buffer_[MAX_BUFFER_SIZE];

public:
  ChatBuffer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ChatBuffer);

  auto&       buffer() { return buffer_; }
  auto const& buffer() const { return buffer_; }

  int  size() const;
  void clear();
};

using ListOfMessages = std::vector<std::string>;
class ChatHistory
{
  ListOfMessages messages_;

public:
  ChatHistory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ChatHistory);

  void                  add_message(std::string&&);
  ListOfMessages const& all_messages() const;
};

} // namespace boomhs

namespace boomhs::ui_ingame
{

void
draw(EngineState&, LevelManager&);

} // namespace boomhs::ui_ingame
