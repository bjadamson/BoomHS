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

  auto &registry = zs.registry;
  auto const eid = find_player(registry);
  auto &pc = registry.get<PlayerData>(eid);
  if (pc.inventory_open) {
    ImGui::SetNextWindowSize({64, 64});

    if (ImGui::Begin("Inventory", nullptr, flags)) {
      draw_button("sword");
      ImGui::End();
    }
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
  if (ImGui::Begin("Target")) {
    ImGui::Text("Name %s", npcdata.name);
    ImGui::Text("Health %i", npcdata.health);
    ImGui::Text("Level %i", npcdata.level);
    ImGui::Text("Alignment %s", alignment_to_string(npcdata.alignment));
    ImGui::End();
  }

  auto &ttable = zs.gfx_state.texture_table;
  assert(ttable.find("test_icon") != std::nullopt);
  auto const ti = *ttable.find("text_icon");

  ImTextureID my_tex_id = reinterpret_cast<void*>(ti.id);

  auto const w = 32, h = 32;
  if (ImGui::Begin("IMAGE TEST")) {
    ImGui::Text("%.0ix%.0i", w, h);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Image(my_tex_id,
        ImVec2(w, h),
        ImVec2(0,0),
        ImVec2(1,1),
        ImColor(255,255,255,255),
        ImColor(255,255,255,128));
    ImGui::End();
  }
}

void
draw(EngineState &es, LevelManager &lm)
{
  draw_nearest_target_info(es.ui_state, lm);
  draw_player_inventory(es, lm);
}

} // ns boomhs::ui_ingame
