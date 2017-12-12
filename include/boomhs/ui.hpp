#pragma once
#include <imgui/imgui.hpp>
#include <boomhs/state.hpp>

namespace boomhs
{

void draw_ui(GameState &state)
{
  auto &imgui = state.imgui;

  static int i = 0;
  ImGui::SliderInt("eid:", &i, 0, state.entities.size());

  static float x, y, z = 0.0f;
  auto *const pe = state.entities[i];
  assert(pe);

  auto &e = *pe;
  auto &pos = e.translation;
  ImGui::SliderFloat("x:", &pos.x, -5.0f, 5.0f);
  ImGui::SliderFloat("y:", &pos.y, -5.0f, 5.0f);
  ImGui::SliderFloat("z:", &pos.z, -5.0f, 5.0f);

  //auto const framerate = imgui.Framerate;
  //auto const ms_frame = 1000.0f / framerate;
  //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_frame, framerate);
}

} // ns boomhs
