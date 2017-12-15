#pragma once
#include <imgui/imgui.hpp>
#include <boomhs/state.hpp>
#include <boomhs/entity.hpp>

namespace boomhs::detail
{

void
draw_entity_editor(GameState &state)
{
  auto &imgui = state.imgui;
  auto &eid_buffer = state.ui_state.eid_buffer;

  ImGui::InputInt("eid:", &eid_buffer, 0, state.entities.size());
  auto *const pe = state.entities[eid_buffer];
  assert(pe);
  auto &e = *pe;

  ImGui::InputFloat3("pos:", glm::value_ptr(e.translation));

  auto &euler_buffer = state.ui_state.euler_angle_buffer;
  euler_buffer = glm::degrees(glm::eulerAngles(e.rotation));

  if (ImGui::InputFloat3("rot:", glm::value_ptr(euler_buffer))) {
    e.rotation = glm::quat(euler_buffer * (3.14159f / 180.f));
  }

  ImGui::InputFloat3("scale:", glm::value_ptr(e.scale));
}

void
draw_camera_info(GameState &state)
{
  ImGui::Begin("CAMERA INFO WINDOW");
  auto const f = state.camera.direction_facing_degrees();

  auto const x = std::to_string(f.x);
  auto const y = std::to_string(f.y);
  auto const z = std::to_string(f.z);
  ImGui::Text("x: '%s', y: '%s', z: '%s'", x.c_str(), y.c_str(), z.c_str());

  ImGui::End();
}

} // ns boomhs::detail

namespace boomhs
{

template<typename PROXY>
void draw_ui(GameState &state, PROXY &proxy)
{
  using namespace detail;
  draw_entity_editor(state);
  draw_camera_info(state);

  ImGui::Checkbox("Draw Skybox", &state.draw_skybox);

  if(ImGui::Checkbox("Enter Pressed", &state.ui_state.enter_pressed)) {
    int const& eid = state.ui_state.eid_buffer;;
    ecst::entity_id const id{static_cast<std::size_t>(eid)};
  }

  ImGui::Begin("MY MENU", nullptr, ImGuiWindowFlags_MenuBar);
  if (ImGui::BeginMenuBar())
    {
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

  auto const& imgui = state.imgui;
  auto const framerate = imgui.Framerate;
  auto const ms_frame = 1000.0f / framerate;
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_frame, framerate);
}

} // ns boomhs
