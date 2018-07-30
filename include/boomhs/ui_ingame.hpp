#pragma once
#include <opengl/colors.hpp>

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

using ChannelId = uint32_t;
struct Channel
{
  ChannelId     id;
  std::string   name;
  opengl::Color color;
};

struct ChatState
{
  bool currently_editing      = false;
  bool reset_yscroll_position = true;

  ChannelId active_channel = 0;
};

using MessageContents = std::string;

struct Message
{
  ChannelId       id;
  MessageContents contents;

  static bool is_command_message(Message const&);
};

using ListOfMessages = std::vector<Message>;
class ChatHistory
{
  ListOfMessages       messages_;
  std::vector<Channel> channels_;

  bool           has_channel(ChannelId) const;
  Channel const* find_channel(ChannelId) const;

public:
  ChatHistory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ChatHistory);

  void        add_channel(ChannelId, char const*, opengl::Color const&);
  auto const& channels() const { return channels_; }

  opengl::Color const& channel_color(ChannelId) const;

  void                  add_message(Message&&);
  ListOfMessages const& all_messages() const;

  ListOfMessages all_messages_in_channel(ChannelId) const;
};

} // namespace boomhs

namespace boomhs::ui_ingame
{

void
reset_active_imguiwindow_yscroll_position(int);

void
draw(EngineState&, LevelManager&);

} // namespace boomhs::ui_ingame
