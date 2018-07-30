#include <boomhs/entity.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/ui_state.hpp>

#include <opengl/texture.hpp>

#include <stlw/algorithm.hpp>

#include <extlibs/imgui.hpp>
#include <algorithm>
#include <optional>

using namespace boomhs;
using namespace opengl;

namespace
{

void
draw_player_inventory(stlw::Logger& logger, EntityRegistry& registry, TextureTable const& ttable)
{
  auto const eid       = find_player(registry);
  auto&      player    = registry.get<PlayerData>(eid);
  auto&      inventory = player.inventory;

  auto const draw_button = [&](TextureInfo const& ti) {
    ImTextureID im_texid = reinterpret_cast<void*>(ti.id);

    imgui_cxx::ImageButtonBuilder image_builder;
    image_builder.frame_padding = 1;
    image_builder.bg_color      = ImColor{255, 255, 255, 255};
    image_builder.tint_color    = ImColor{255, 255, 255, 128};

    auto const size = ImVec2(ti.width, ti.height);
    return image_builder.build(im_texid, size);
  };

  auto const draw_icon = [&](size_t const pos) {
    {
      auto&      slot          = inventory.slot(pos);
      bool const slot_occupied = slot.occupied();
      auto* ti = slot_occupied ? slot.item(registry).ui_tinfo : ttable.find("InventorySlotEmpty");
      assert(ti);
      bool const button_pressed = draw_button(*ti);

      // If the button is pressed, and the slot is occupied (location clicked) then go ahead and
      // remove the item from the player, and removing it from the UI.
      if (button_pressed && slot_occupied) {
        Player::drop_entity(logger, slot.eid(), registry);
        slot.reset();
      }
    }

    // remove_entity() may invalidate slot& reference, find again.
    auto& slot = inventory.slot(pos);
    if (slot.occupied() && ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", slot.item(registry).tooltip);
    }
  };

  auto const draw_inventory = [&]() {
    int constexpr TOTAL        = 40;
    int constexpr ROW_COUNT    = 8;
    int constexpr COLUMN_COUNT = TOTAL / ROW_COUNT;

    assert(0 == (TOTAL % ROW_COUNT));
    assert(0 == (TOTAL % COLUMN_COUNT));

    FOR(i, TOTAL)
    {
      if (i > 0 && ((i % ROW_COUNT) == 0)) {
        ImGui::NewLine();
      }
      draw_icon(i);
      ImGui::SameLine();
    }
  };

  auto constexpr flags = (0 | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoTitleBar);

  auto const draw_window = [&]() {
    imgui_cxx::with_window(draw_inventory, "Inventory", nullptr, flags);
  };
  imgui_cxx::with_stylevars(draw_window, ImGuiStyleVar_ChildRounding, 5.0f);
}

void
draw_nearest_target_info(NearbyTargets const& nearby_targets, TextureTable const& ttable,
                         EntityRegistry& registry)
{
  auto constexpr LEFT = 39;
  auto constexpr TOP  = 768 - 125 - 6;
  ImGui::SetNextWindowPos(ImVec2(LEFT, TOP));
  ImGui::SetNextWindowSize(ImVec2(200, 105));

  // clang-format off
  auto static constexpr WINDOW_FLAGS = (0
    | ImGuiWindowFlags_NoBringToFrontOnFocus
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoInputs
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoScrollWithMouse
    | ImGuiWindowFlags_NoSavedSettings
    | ImGuiWindowFlags_NoTitleBar
  );
  // clang-format on

  auto const selected_o = nearby_targets.selected();
  if (!selected_o) {
    auto const draw = [&]() { ImGui::Text("NO TARGET"); };

    imgui_cxx::with_window(draw, "Target", nullptr, WINDOW_FLAGS);
    return;
  }

  EntityID const selected = *selected_o;
  auto const&    npcdata  = registry.get<NPCData>(selected);

  char const* name = "test_icon";
  assert(ttable.find(name) != nullptr);
  auto* ti = ttable.find(name);
  assert(ti);

  ImTextureID my_tex_id = reinterpret_cast<void*>(ti->id);

  auto const draw = [&]() {
    ImGui::Text("Name %s", npcdata.name);
    ImGui::Text("Level %i", npcdata.level);
    ImGui::Text("Alignment %s", alignment_to_string(npcdata.alignment));

    auto& hp = npcdata.health;
    ImGui::Text("Health %i/%i", hp.current, hp.max);

    auto const draw_icon = [&]() {
      ImGui::Image(my_tex_id, ImVec2(ti->width, ti->height), ImVec2(0, 0), ImVec2(1, 1),
                   ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
    };

    ImGui::SameLine();
    imgui_cxx::with_stylevars(draw_icon, ImGuiStyleVar_ChildRounding, 5.0f);
  };
  imgui_cxx::with_window(draw, "Target", nullptr, WINDOW_FLAGS);
}

void
draw_chatwindow_body(EngineState& es, PlayerData& player)
{
  auto& logger   = es.logger;
  auto& ingame = es.ui_state.ingame;
  auto& chat_state = ingame.chat_state;
  auto& is_editing = chat_state.currently_editing;
  auto& chat_buffer = ingame.chat_buffer;
  auto& chat_history = ingame.chat_history;
  auto const& active_channel = chat_state.active_channel;


  auto const copy_message_to_chathistory = [&]()
  {
    std::string copy = chat_buffer.buffer();
    stlw::trim(copy);
    if (!copy.empty()) {
      Message m{active_channel, MOVE(copy)};
      chat_history.add_message(MOVE(m));
    }
    chat_buffer.clear();
  };

  auto const print_chat_message = [&](Message const& m) {

    Color const active_color = Message::is_command_message(m)
      ? LOC::RED
      : chat_history.channel_color(active_channel);

    ImVec4 const im_color = imgui_cxx::from_color(active_color);
    imgui_cxx::text_wrapped_colored(im_color, "%s:%s", player.name.c_str(), m.contents.c_str());
  };
  auto& reset_yscroll = chat_state.reset_yscroll_position;
  auto const draw = [&]()
  {
    auto const chat_messages = active_channel == 0
      ? chat_history.all_messages()
      : chat_history.all_messages_in_channel(active_channel);
    for(auto const& m : chat_messages) {
      print_chat_message(m);
    }

    if (reset_yscroll) {
      ui_ingame::reset_active_imguiwindow_yscroll_position(1000);
      reset_yscroll = false;
    }
    if (is_editing) {
      auto& buffer = chat_buffer.buffer();
      auto const buffer_size = chat_buffer.size();

      auto const font_height = ImGui::GetFontSize() * 2;
      auto const input_size = ImVec2(-10, font_height);

      auto constexpr FLAGS = (0
        | ImGuiInputTextFlags_AllowTabInput
        | ImGuiInputTextFlags_CtrlEnterForNewLine
        | ImGuiInputTextFlags_EnterReturnsTrue
          );
      bool const enter_pressed = ImGui::InputTextMultiline("", buffer, buffer_size, input_size,
          FLAGS);

      if (enter_pressed) {
        copy_message_to_chathistory();
        is_editing = false;
        reset_yscroll = true;
      }

      // Keep focus on the input widget.
      ImGui::SetItemDefaultFocus();
      ImGui::SetKeyboardFocusHere(-1);
    }
  };
  auto const height = is_editing ? -20.0f : -5.0f;
  auto const size = ImVec2(-5, height);

  bool constexpr SHOW_BORDER = false;
  auto constexpr flags = (0 | ImGuiWindowFlags_NoTitleBar);

  for (auto const& c : chat_history.channels()) {
    if (ImGui::SmallButton(c.name.c_str())) {
      chat_state.active_channel = c.id;
      reset_yscroll = true;
    }
    ImGui::SameLine();
  }
  ImGui::NewLine();

  IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(ImGuiStyleVar_WindowPadding, ImVec2(1 ,0));
  IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(ImGuiStyleVar_WindowRounding, 1.0);
  IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(ImGuiStyleVar_ItemSpacing, ImVec2(1.0, 5.0));
  IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(ImGuiStyleVar_ChildRounding, 5.0f);

  imgui_cxx::with_childframe(draw, "ChatWindow InnerChildFrame", size, SHOW_BORDER, flags);
}

void
draw_chatwindow(EngineState& es, PlayerData& player)
{
  auto& logger = es.logger;
  auto& ingame = es.ui_state.ingame;

  auto const draw_window = [&]() {
    auto constexpr flags = (0
      | ImGuiWindowFlags_AlwaysUseWindowPadding
      | ImGuiWindowFlags_NoBringToFrontOnFocus
      | ImGuiWindowFlags_NoCollapse
      | ImGuiWindowFlags_HorizontalScrollbar
      | ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoNavFocus
      | ImGuiWindowFlags_NoScrollbar
      | ImGuiWindowFlags_NoScrollWithMouse
      | ImGuiWindowFlags_NoTitleBar
      );

    auto const [window_w, window_h] = es.dimensions.size();
    ImVec2 const offset{10, 6};
    auto const chat_w = 480, chat_h = 200;
    auto const chat_x = window_w - chat_w - offset.x, chat_y = window_h - chat_h - offset.y;
    ImVec2 const chat_pos{chat_x, chat_y};
    ImGui::SetNextWindowPos(chat_pos);

    ImVec2 const chat_size{chat_w, chat_h};
    ImGui::SetNextWindowSize(chat_size);

    auto const draw_chatwindow_body_wrapper = [&]() {
      draw_chatwindow_body(es, player);
    };
    imgui_cxx::with_window(draw_chatwindow_body_wrapper, "Chat Window", nullptr, flags);
  };
  imgui_cxx::with_stylevars(draw_window, ImGuiStyleVar_ChildRounding, 5.0f);
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// ChatBuffer
int
ChatBuffer::size() const
{
  return IM_ARRAYSIZE(buffer());
}

void
ChatBuffer::clear()
{
  stlw::memzero(buffer(), ChatBuffer::MAX_BUFFER_SIZE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Message
bool
Message::is_command_message(Message const& m)
{
  return m.contents.find("/") == 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ChatHistory
Channel const*
ChatHistory::find_channel(ChannelId const id) const
{
  Channel const* p = nullptr;
  for (auto const& c : channels()) {
    if (c.id == id) {
      p = &c;
      break;
    }
  }
  return p;
}

bool
ChatHistory::has_channel(ChannelId const id) const
{
  auto const* c = find_channel(id);
  return (c == nullptr) ? false : true;
}

void
ChatHistory::add_channel(ChannelId const id, char const* name, Color const& color)
{
  assert(!has_channel(id));
  Channel c{id, name, color};
  channels_.emplace_back(MOVE(c));
}

Color const&
ChatHistory::channel_color(ChannelId const id) const
{
  auto const* p = find_channel(id);
  assert(p);

  return p->color;
}

void
ChatHistory::add_message(Message&& m)
{
  assert(has_channel(m.id));
  messages_.emplace_back(MOVE(m));
}

ListOfMessages const&
ChatHistory::all_messages() const
{
  return messages_;
}

ListOfMessages
ChatHistory::all_messages_in_channel(ChannelId const id) const
{
  ListOfMessages filtered;
  for (auto const& m : all_messages()) {
    if (m.id == id) {
      // copy the message into the result
      filtered.push_back(m);
    }
  }
  return filtered;
}

} // namespace boomhs

namespace boomhs::ui_ingame
{

void
reset_active_imguiwindow_yscroll_position(int const offset)
{
  auto const max_y = ImGui::GetScrollMaxY();
  ImGui::SetScrollY(max_y + offset);
}

void
draw(EngineState& es, LevelManager& lm)
{
  auto& zs       = lm.active();
  auto& logger   = es.logger;
  auto& registry = zs.registry;
  auto& ttable   = zs.gfx_state.texture_table;

  auto& ldata          = zs.level_data;
  auto& nearby_targets = ldata.nearby_targets;
  draw_nearest_target_info(nearby_targets, ttable, registry);

  auto const eid       = find_player(registry);
  auto&      player    = registry.get<PlayerData>(eid);
  draw_chatwindow(es, player);

  auto& inventory = player.inventory;
  if (inventory.is_open()) {
    draw_player_inventory(logger, registry, ttable);
  }
}

} // namespace boomhs::ui_ingame
