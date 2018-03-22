#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl_gl3.h>
#include <initializer_list>
#include <utility>

namespace imgui_cxx
{

template <typename FN, typename... Args>
void
with_window(FN const& fn, Args&&... args)
{
  if (ImGui::Begin(FORWARD(args)))
  {
    fn();
    ImGui::End();
  }
  else
  {
    // not sure why this would ever happen, look it up..
    std::abort();
  }
}

template <typename FN, typename... Args>
void
with_stylevar(FN const& fn, Args&&... args)
{
  ImGui::PushStyleVar(FORWARD(args));
  fn();
  ImGui::PopStyleVar();
}

template <typename FN, typename... Args>
void
with_childframe(FN const& fn, Args&&... args)
{
  if (ImGui::BeginChild(FORWARD(args)))
  {
    fn();
    ImGui::EndChild();
  }
  else
  {
    std::abort();
  }
}

template <typename FN>
void
with_menu(FN const& fn, char const* name)
{
  if (ImGui::BeginMenu(name))
  {
    fn();
    ImGui::EndMenu();
  }
}

template <typename FN, typename... Args>
void
with_mainmenubar(FN const& fn, Args&&... args)
{
  if (ImGui::BeginMainMenuBar(FORWARD(args)))
  {
    fn();
    ImGui::EndMainMenuBar();
  }
}

struct ImageButtonBuilder
{
  ImVec2 uv0 = ImVec2{0, 0};
  ImVec2 uv1 = ImVec2{1, 1};

  int frame_padding = -1;

  ImVec4 bg_color = ImVec4{0, 0, 0, 0};
  ImVec4 tint_color = ImVec4{1, 1, 1, 1};

  bool build(ImTextureID id, ImVec2 const& size) const
  {
    return ImGui::ImageButton(id, size, uv0, uv1, frame_padding, bg_color, tint_color);
  }
};

} // namespace imgui_cxx
