#include <boomhs/entity.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/ui_state.hpp>

#include <opengl/texture.hpp>

#include <extlibs/imgui.hpp>
#include <optional>

using namespace opengl;

namespace boomhs::ui_ingame
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
  imgui_cxx::with_stylevar(draw_window, ImGuiStyleVar_ChildRounding, 5.0f);
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
    ImGui::Text("Health %i", npcdata.health);

    auto const draw_icon = [&]() {
      ImGui::Image(my_tex_id, ImVec2(ti->width, ti->height), ImVec2(0, 0), ImVec2(1, 1),
                   ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
    };

    ImGui::SameLine();
    imgui_cxx::with_stylevar(draw_icon, ImGuiStyleVar_ChildRounding, 5.0f);
  };
  imgui_cxx::with_window(draw, "Target", nullptr, WINDOW_FLAGS);
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
  auto&      inventory = player.inventory;
  if (inventory.is_open()) {
    draw_player_inventory(logger, registry, ttable);
  }
}

} // namespace boomhs::ui_ingame
