#include <boomhs/camera.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/ui_state.hpp>
#include <boomhs/zone_state.hpp>

#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>

#include <extlibs/imgui.hpp>
#include <sstream>

using namespace boomhs;
using namespace opengl;
using namespace gl_sdl;

// clang-format off
auto static constexpr DEFAULT_WINDOW_FLAGS = (0
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

namespace
{

void
draw_player_inventory(common::Logger& logger, Player& player, EntityRegistry& registry,
                      TextureTable const& ttable)
{
  auto& inventory = player.inventory;

  auto const draw_button = [&](TextureInfo const& ti) {
    ImTextureID im_texid = reinterpret_cast<void*>(ti.id);

    imgui_cxx::ImageButtonBuilder image_builder;
    image_builder.frame_padding = 1;
    image_builder.bg_color      = ImColor{255, 255, 255, 255};
    image_builder.tint_color    = ImColor{255, 255, 255, 128};

    auto const size = ImVec2(ti.width, ti.height);
    image_builder.build(im_texid, size);
  };

  auto const draw_icon = [&](size_t const pos) {
    {
      auto&      slot          = inventory.slot(pos);
      bool const slot_occupied = slot.occupied();
      auto* ti = slot_occupied ? slot.item(registry).ui_tinfo : ttable.find("InventorySlotEmpty");
      assert(ti);
      draw_button(*ti);
      bool const left_mouse_pressed  = ImGui::IsItemClicked(0);
      bool const right_mouse_pressed = ImGui::IsItemClicked(1);

      // If the button is pressed, and the slot is occupied (location clicked) then go ahead and
      // remove the item from the player, and removing it from the UI.
      if (slot_occupied) {
        if (right_mouse_pressed) {
          player.drop_entity(logger, slot.eid(), registry);
          slot.reset();
        }
        else if (left_mouse_pressed) {
          LOG_ERROR_SPRINTF("Equipping %s", slot.item(registry).name);
        }
      }
    }

    // remove_entity() may invalidate slot& reference, find again.
    auto& slot = inventory.slot(pos);
    if (slot.occupied() && ImGui::IsItemHovered()) {
      auto&       item    = slot.item(registry);
      auto const& tooltip = item.tooltip;
      if (item.has_single_owner()) {
        ImGui::SetTooltip("%s |Owner: %s", tooltip, item.current_owner().value.c_str());
      }
      else {
        std::stringstream ss;
        auto const&       prev_owners = item.all_owners();
        FOR(i, prev_owners.size())
        {
          // SKIP the first owner, as that's considered the current owner.
          if (i == 0) {
            continue;
          }
          else if (i > 1) {
            ss << ", ";
          }
          ss << prev_owners[i].value;
        }
        auto const names = ss.str();
        ImGui::SetTooltip("%s |Owner: %s |Previous Owners: (%s)", tooltip,
                          item.current_owner().value.c_str(), names.c_str());
      }
    }
  };

  auto const draw_inventory = [&]() { imgui_cxx::draw_grid(40, 8, draw_icon); };

  auto constexpr flags = (0 | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoTitleBar);

  auto const draw_window = [&]() {
    imgui_cxx::with_window(draw_inventory, "Inventory", nullptr, flags);
  };
  imgui_cxx::with_stylevars(draw_window, ImGuiStyleVar_ChildRounding, 5.0f);
}

void
draw_nearest_target_info(Viewport const& viewport, NearbyTargets const& nbt,
                         TextureTable const& ttable, EntityRegistry& registry)
{
  auto constexpr LEFT_OFFSET = 39;
  auto const left            = viewport.left() + LEFT_OFFSET;

  auto constexpr BOTTOM_OFFSET = 119;
  auto const top               = viewport.bottom() - BOTTOM_OFFSET;
  ImGui::SetNextWindowPos(ImVec2(left, top));

  ImVec2 const WINDOW_POS(200, 105);
  ImGui::SetNextWindowSize(WINDOW_POS);

  auto const selected_o = nbt.selected();
  if (!selected_o) {
    auto const draw = [&]() { ImGui::Text("NO TARGET"); };

    imgui_cxx::with_window(draw, "Target", nullptr, DEFAULT_WINDOW_FLAGS);
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
  imgui_cxx::with_window(draw, "Target", nullptr, DEFAULT_WINDOW_FLAGS);
}

void
draw_chatwindow_body(EngineState& es, Player& player)
{
  auto&       logger         = es.logger;
  auto&       ingame         = es.ui_state.ingame;
  auto&       chat_state     = ingame.chat_state;
  auto&       is_editing     = chat_state.currently_editing;
  auto&       chat_buffer    = ingame.chat_buffer;
  auto&       chat_history   = ingame.chat_history;
  auto const& active_channel = chat_state.active_channel;

  auto const copy_message_to_chathistory = [&]() {
    std::string copy = chat_buffer.buffer();
    common::trim(copy);
    if (!copy.empty()) {
      Message m{active_channel, MOVE(copy)};
      chat_history.add_message(MOVE(m));
    }
    chat_buffer.clear();
  };

  auto const print_chat_message = [&](Message const& m) {
    Color const active_color =
        Message::is_command_message(m) ? LOC4::RED : chat_history.channel_color(active_channel);

    ImVec4 const im_color = imgui_cxx::from_color(active_color);
    imgui_cxx::text_wrapped_colored(im_color, "%s:%s", player.name.c_str(), m.contents.c_str());
  };
  auto&      reset_yscroll = chat_state.reset_yscroll_position;
  auto const draw          = [&]() {
    auto const chat_messages = active_channel == 0
                                   ? chat_history.all_messages()
                                   : chat_history.all_messages_in_channel(active_channel);
    for (auto const& m : chat_messages) {
      print_chat_message(m);
    }

    if (reset_yscroll) {
      ui_ingame::reset_active_imguiwindow_yscroll_position(1000);
      reset_yscroll = false;
    }
    if (is_editing) {
      auto&      buffer      = chat_buffer.buffer();
      auto const buffer_size = chat_buffer.size();

      auto const font_height = ImGui::GetFontSize() * 2;
      auto const input_size  = ImVec2(-10, font_height);

      auto constexpr FLAGS =
          (0 | ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CtrlEnterForNewLine |
           ImGuiInputTextFlags_EnterReturnsTrue);
      bool const enter_pressed =
          ImGui::InputTextMultiline("", buffer, buffer_size, input_size, FLAGS);

      if (enter_pressed) {
        copy_message_to_chathistory();
        is_editing    = false;
        reset_yscroll = true;
      }

      // Keep focus on the input widget.
      ImGui::SetItemDefaultFocus();
      ImGui::SetKeyboardFocusHere(-1);
    }
  };
  auto const height = is_editing ? -20.0f : -5.0f;
  auto const size   = ImVec2(-5, height);

  bool constexpr SHOW_BORDER = false;
  auto constexpr flags       = (0 | ImGuiWindowFlags_NoTitleBar);

  for (auto const& c : chat_history.channels()) {
    if (ImGui::SmallButton(c.name.c_str())) {
      chat_state.active_channel = c.id;
      reset_yscroll             = true;
    }
    ImGui::SameLine();
  }
  ImGui::NewLine();

  IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(ImGuiStyleVar_WindowPadding, ImVec2(1, 0));
  IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(ImGuiStyleVar_WindowRounding, 1.0);
  IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(ImGuiStyleVar_ItemSpacing, ImVec2(1.0, 5.0));
  IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(ImGuiStyleVar_ChildRounding, 5.0f);

  imgui_cxx::with_childframe(draw, "ChatWindow InnerChildFrame", size, SHOW_BORDER, flags);
}

void
draw_chatwindow(EngineState& es, Player& player)
{
  auto& logger = es.logger;
  auto& ingame = es.ui_state.ingame;

  auto const draw_window = [&]() {
    auto constexpr flags =
        (0 | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoBringToFrontOnFocus |
         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar |
         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
         ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar);

    auto const vp = Viewport::from_frustum(es.frustum);

    auto const [window_w, window_h] = vp.size();
    ImVec2 const offset{10, 6};
    auto const   chat_w = 480, chat_h = 200;
    auto const   chat_x = window_w - chat_w - offset.x, chat_y = window_h - chat_h - offset.y;
    ImVec2 const chat_pos{chat_x, chat_y};
    ImGui::SetNextWindowPos(chat_pos);

    ImVec2 const chat_size{chat_w, chat_h};
    ImGui::SetNextWindowSize(chat_size);

    auto const draw_chatwindow_body_wrapper = [&]() { draw_chatwindow_body(es, player); };
    imgui_cxx::with_window(draw_chatwindow_body_wrapper, "Chat Window", nullptr, flags);
  };
  imgui_cxx::with_stylevars(draw_window, ImGuiStyleVar_ChildRounding, 5.0f);
}

void
draw_debugoverlay_window(EngineState& es, DrawState& ds)
{
  auto const framerate = es.imgui.Framerate;
  auto const ms_frame  = 1000.0f / framerate;

  auto const draw = [&]() {
    ImGui::Text("Debug Information");
    ImGui::Separator();
    ImGui::Text("#verts: %s", ds.to_string().c_str());
    ImGui::Text("FPS(avg): %.1f", framerate);
    ImGui::Text("ms/frame: %.3f", ms_frame);
  };
  auto const height = 20.0f;
  auto const size   = ImVec2(50, height);

  bool constexpr SHOW_BORDER = false;

  auto const vp                   = Viewport::from_frustum(es.frustum);
  auto const [window_w, window_h] = vp.size();
  ImVec2 const offset{100, 50};
  auto const   chat_w = 300, chat_h = 100;
  auto const   chat_x = window_w - chat_w - offset.x, chat_y = chat_h - offset.y;
  ImVec2 const chat_pos{chat_x, chat_y};
  ImGui::SetNextWindowPos(chat_pos);

  ImVec2 const chat_size{chat_w, chat_h};
  ImGui::SetNextWindowSize(chat_size);

  imgui_cxx::with_window(draw, "Debug Overlay", nullptr, DEFAULT_WINDOW_FLAGS);
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
  common::memzero(buffer(), ChatBuffer::MAX_BUFFER_SIZE);
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

class BlinkTimer
{
  bool              is_blinking_ = false;
  common::StopWatch stopwatch_;

public:
  void update() { stopwatch_.update(); }
  bool expired() const { return stopwatch_.expired(); }
  bool is_blinking() const { return is_blinking_; }
  void toggle() { is_blinking_ ^= true; }
  void set_ms(double const t) { stopwatch_.set_ms(t); }
};

void
draw_2dui(RenderState& rstate)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& ds     = rstate.ds;

  auto& logger   = es.logger;
  auto& zs       = fstate.zs;
  auto& registry = zs.registry;

  auto& ldata = zs.level_data;
  auto& nbt   = ldata.nearby_targets;

  static BlinkTimer blink_timer;
  blink_timer.update();

  bool const is_expired = blink_timer.expired();

  auto&      player           = find_player(registry);
  bool const player_attacking = player.is_attacking;

  auto const reset_attack_timer = [&]() {
    auto const BLINK_TIME_IN_MS = common::TimeConversions::seconds_to_millis(1);
    blink_timer.set_ms(BLINK_TIME_IN_MS);
  };

  if (player_attacking) {
    if (is_expired) {
      blink_timer.toggle();
      reset_attack_timer();
    }
  }
  else if (is_expired) {
    reset_attack_timer();
  }

  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  bool const  using_blink_shader = player_attacking && blink_timer.is_blinking();
  auto const& shader_name        = using_blink_shader ? "2d_ui_blinkcolor_icon" : "2dtexture";
  auto&       spp                = sps.ref_sp(shader_name);

  if (using_blink_shader) {
    auto const& player_transform = player.transform();
    auto const  player_pos       = player_transform.translation;

    // We can only be considering the blink shader if we are attacking an entity, and if we are
    // attacking an entity that means their should be a selected target.
    auto const selected_nbt = nbt.selected();
    assert(selected_nbt);
    auto const  target_eid = *selected_nbt;
    auto const& target     = registry.get<Transform>(target_eid);
    auto const& target_pos = target.translation;

    auto const distance     = glm::distance(player_pos, target_pos);
    bool const close_enough = distance < 2;
    // LOG_ERROR_SPRINTF("close: %i, distance: %f, ppos: %s tpos: %s",
    // close_enough,
    // distance,
    // glm::to_string(player_pos),
    // glm::to_string(target_pos)
    //);
    auto const blink_color =
        NPC::within_attack_range(player_pos, target_pos) ? LOC4::RED : LOC4::BLUE;
    spp.while_bound(logger, [&]() { spp.set_uniform_color(logger, "u_blinkcolor", blink_color); });
  }

  auto const view_frustum        = fstate.frustum();
  auto const draw_icon_on_screen = [&rstate, &ttable](auto& sp, glm::vec2 const& pos,
                                                      glm::vec2 const& size, char const* tex_name) {
    auto& ti = *ttable.find(tex_name);
    render::draw_fbo_testwindow(rstate, pos, size, sp, ti);
  };

  auto const proj_matrix = fstate.projection_matrix();

  auto const draw_slot_icon = [&](auto const slot_pos, char const* icon_name) {
    glm::vec2 const size{32.0f};

    auto constexpr SPACE_BETWEEN = 10;
    float const left =
        view_frustum.left + 39 + 200 + SPACE_BETWEEN + (slot_pos * (size.x + SPACE_BETWEEN));

    auto constexpr SPACE_BENEATH = 10;
    float const     bottom       = view_frustum.bottom - SPACE_BENEATH;
    glm::vec2 const pos{left, bottom};
    draw_icon_on_screen(spp, pos, size, icon_name);
  };

  draw_slot_icon(0, "fist");
  draw_slot_icon(1, "sword");
  draw_slot_icon(2, "ak47");
  draw_slot_icon(3, "first-aid");

  {
    glm::vec2 const size{16.0f};

    auto const center           = view_frustum.center();
    auto const middle_of_screen = glm::vec2{center.x, center.y};
    auto&      sp               = sps.ref_sp("2dtexture");

    ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();
    draw_icon_on_screen(sp, middle_of_screen, size, "target_selectable_cursor");
  }
}

void
draw_inventory_overlay(RenderState& rstate)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& zs     = fstate.zs;
  auto& logger = es.logger;
  auto& sps    = zs.gfx_state.sps;
  auto& sp     = sps.ref_sp("2dcolor");

  auto const& f      = es.frustum;
  auto const  vp     = Viewport::from_frustum(f);
  auto const  rect   = vp.rect_float();
  auto        buffer = RectBuilder{rect}.build();

  DrawInfo dinfo = OG::copy_rectangle(logger, sp.va(), buffer);

  ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();
  BIND_UNTIL_END_OF_SCOPE(logger, sp);

  auto constexpr NEAR = 1.0f;
  auto constexpr FAR  = -1.0f;

  auto const pm =
      glm::ortho(f.left_float(), f.right_float(), f.bottom_float(), f.top_float(), NEAR, FAR);
  sp.set_uniform_mat4(logger, "u_mv", pm);

  auto color = LOC4::GRAY;
  color.set_a(0.40);
  sp.set_uniform_color(logger, "u_color", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  render::draw_2d(rstate, GL_TRIANGLES, sp, dinfo);
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
draw(FrameState& fs, Camera& camera, DrawState& ds)
{
  auto& es       = fs.es;
  auto& zs       = fs.zs;
  auto& logger   = es.logger;
  auto& registry = zs.registry;
  auto& ttable   = zs.gfx_state.texture_table;

  auto& ldata = zs.level_data;
  auto& nbt   = ldata.nearby_targets;

  auto const& frustum = es.frustum;
  auto const  vp      = Viewport::from_frustum(frustum);
  draw_nearest_target_info(vp, nbt, ttable, registry);

  // Create a renderstate using an orthographic projection.
  auto const ztemp     = camera.ortho.zoom();
  auto const vsettings = camera.view_settings_ref();
  auto       fss = FrameState::from_camera_for_2dui_overlay(es, zs, camera, vsettings, frustum);

  RenderState rstate{fss, ds};
  draw_2dui(rstate);

  auto& player = find_player(registry);
  draw_chatwindow(es, player);
  draw_debugoverlay_window(es, ds);

  auto& inventory = player.inventory;
  if (inventory.is_open()) {
    draw_inventory_overlay(rstate);
    draw_player_inventory(logger, player, registry, ttable);
  }
}

} // namespace boomhs::ui_ingame
