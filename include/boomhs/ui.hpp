#pragma once
#include <imgui/imgui.hpp>
#include <boomhs/state.hpp>
#include <boomhs/entity.hpp>

namespace boomhs
{

template<typename PROXY>
void draw_ui(GameState &state, PROXY &proxy)
{
  auto &imgui = state.imgui;

  static int eid = GameState::AT_INDEX;
  ImGui::SliderInt("eid:", &eid, 0, state.entities.size());

  static float x, y, z = 0.0f;
  auto *const pe = state.entities[eid];
  assert(pe);

  auto &e = *pe;
  auto &pos = e.translation;
  ImGui::SliderFloat("x:", &pos.x, -5.0f, 5.0f);
  ImGui::SliderFloat("y:", &pos.y, -5.0f, 5.0f);
  ImGui::SliderFloat("z:", &pos.z, -5.0f, 5.0f);

  using gef = game::entity_factory;
  auto et = gef::make_transformer(state.logger, proxy);

  if(ImGui::Checkbox("Enter Pressed", &state.ui_state.enter_pressed)) {
    ecst::entity_id const id{static_cast<std::size_t>(eid)};
  }

  ImGui::Begin("MY MENU", nullptr, ImGuiWindowFlags_MenuBar);
  if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            ImGui::MenuItem("BLAH", nullptr, nullptr);
        }
        if (ImGui::BeginMenu("Examples"))
        {
            ImGui::MenuItem("Main menu bar", nullptr, nullptr);
            ImGui::MenuItem("Console", nullptr, nullptr);
            ImGui::MenuItem("Log", nullptr, nullptr);
            ImGui::MenuItem("Simple layout", nullptr, nullptr);
            ImGui::MenuItem("Property editor", nullptr, nullptr);
            ImGui::MenuItem("Long text display", nullptr, nullptr);
            ImGui::MenuItem("Auto-resizing window", nullptr, nullptr);
            ImGui::MenuItem("Constrained-resizing window", nullptr, nullptr);
            ImGui::MenuItem("Simple overlay", nullptr, nullptr);
            ImGui::MenuItem("Manipulating window titles", nullptr, nullptr);
            ImGui::MenuItem("Custom rendering", nullptr, nullptr);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("Metrics", nullptr, nullptr);
            ImGui::MenuItem("Style Editor", nullptr, nullptr);
            ImGui::MenuItem("About Dear ImGui", nullptr, nullptr);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
  ImGui::End();

  auto const framerate = imgui.Framerate;
  auto const ms_frame = 1000.0f / framerate;
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_frame, framerate);
}

} // ns boomhs
