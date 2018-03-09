#include <boomhs/ui_ingame.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/ui_state.hpp>
#include <boomhs/state.hpp>

#include <extlibs/imgui.hpp>
#include <optional>

namespace boomhs::ui_ingame
{

void
draw_player_inventory(EngineState &es, LevelManager &lm)
{
  auto &zs = lm.active();
  auto &ttable = zs.gfx_state.texture_table;

  auto constexpr flags = (0
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoTitleBar
    );

  auto const draw_button = [&](char const* filename) {
    assert(ttable.find(filename) != std::nullopt);
    auto const ti = *ttable.find(filename);

    ImTextureID my_tex_id = reinterpret_cast<void*>(ti.id);

    ImGui::ImageButton(my_tex_id,
        ImVec2(32, 32),
        ImVec2(0,0),
        ImVec2(1,1),
        ImColor(255,255,255,255),
        ImColor(255,255,255,128));
  };
  auto const draw_button_window = [&]() {
    auto const draw_sword = [&]()
    {
      int constexpr TOTAL = 40;
      int constexpr ROW_COUNT = 8;
      int constexpr COLUMN_COUNT = TOTAL / ROW_COUNT;

      assert(0 == (TOTAL % ROW_COUNT));
      assert(0 == (TOTAL % COLUMN_COUNT));

      FOR(i, COLUMN_COUNT) {
        FOR(i, ROW_COUNT) {
          draw_button("sword");
          ImGui::SameLine();
        }
        ImGui::NewLine();
      }
    };

    auto const draw_sword_window = [&]() {
      imgui_cxx::with_window(draw_sword, "Inventory");
    };
    imgui_cxx::with_stylevar(draw_sword_window, ImGuiStyleVar_ChildRounding, 5.0f);
  };

  auto &registry = zs.registry;
  auto const eid = find_player(registry);
  auto &pc = registry.get<PlayerData>(eid);
  if (pc.inventory_open) {
    draw_button_window();
  }
}

void
draw_nearest_target_info(UiState &ui, LevelManager &lm)
{
  auto &zs = lm.active();
  auto &registry = zs.registry;
  auto &ldata = zs.level_data;

  auto &nearby_targets = ldata.nearby_targets;
  if (nearby_targets.empty()) {
    // nothing to draw
    return;
  }

  EntityID const closest = nearby_targets.closest();

  auto const& npcdata = registry.get<NPCData>(closest);

  auto &ttable = zs.gfx_state.texture_table;
  assert(ttable.find("test_icon") != std::nullopt);
  auto const ti = *ttable.find("text_icon");
  ImTextureID my_tex_id = reinterpret_cast<void*>(ti.id);

  auto const w = 32, h = 32;
  auto const draw = [&]() {
    ImGui::Text("Name %s", npcdata.name);
    ImGui::Text("Health %i", npcdata.health);
    ImGui::Text("Level %i", npcdata.level);
    ImGui::Text("Alignment %s", alignment_to_string(npcdata.alignment));

    auto const draw_icon = [&]()
    {
      ImGui::ImageButton(my_tex_id,
          ImVec2(w, h),
          ImVec2(0,0),
          ImVec2(1,1),
          ImColor(255,255,255,255),
          ImColor(255,255,255,128));
    };
    imgui_cxx::with_stylevar(draw_icon, ImGuiStyleVar_ChildRounding, 5.0f);
  };
  imgui_cxx::with_window(draw, "Target");
}

void
draw(EngineState &es, LevelManager &lm)
{
  draw_nearest_target_info(es.ui_state, lm);
  draw_player_inventory(es, lm);
}

} // ns boomhs::ui_ingame
