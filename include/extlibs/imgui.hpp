#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl_gl3.h>
#include <initializer_list>
#include <utility>

namespace imgui_cxx
{

inline auto
input_string(char const* text, std::string& val)
{
  static constexpr auto SIZE = 32;
  assert(val.size() < SIZE);

  char buffer[SIZE] = {'\0'};
  FOR(i, val.size()) { buffer[i] = val[i]; }
  auto const result = ImGui::InputText(text, buffer, IM_ARRAYSIZE(buffer));
  if (result) {
    val = buffer;
  }
  return result;
}

template<typename ...Args>
inline auto
input_sizet(char const* label, size_t* data, Args&&... args)
{
  int* casted = reinterpret_cast<int*>(data);
  return ImGui::InputInt(label, casted, FORWARD(args));
}

inline auto
main_menu_bar_size()
{
  ImGui::BeginMainMenuBar();
  auto const main_menu_size = ImGui::GetWindowSize();
  ImGui::EndMainMenuBar();
  return main_menu_size;
}

template <typename FN, typename... Args>
void
with_window(FN const& fn, Args&&... args)
{
  if (ImGui::Begin(FORWARD(args))) {
    fn();
    ImGui::End();
  }
  else {
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
  if (ImGui::BeginChild(FORWARD(args))) {
    fn();
    ImGui::EndChild();
  }
  else {
    std::abort();
  }
}

template <typename FN>
void
with_menu(FN const& fn, char const* name)
{
  if (ImGui::BeginMenu(name)) {
    fn();
    ImGui::EndMenu();
  }
}

template <typename FN, typename... Args>
void
with_mainmenubar(FN const& fn, Args&&... args)
{
  if (ImGui::BeginMainMenuBar(FORWARD(args))) {
    fn();
    ImGui::EndMainMenuBar();
  }
}

struct ImageButtonBuilder
{
  ImVec2 uv0 = ImVec2{0, 0};
  ImVec2 uv1 = ImVec2{1, 1};

  int frame_padding = -1;

  ImVec4 bg_color   = ImVec4{0, 0, 0, 0};
  ImVec4 tint_color = ImVec4{1, 1, 1, 1};

  bool build(ImTextureID id, ImVec2 const& size) const
  {
    return ImGui::ImageButton(id, size, uv0, uv1, frame_padding, bg_color, tint_color);
  }
};

} // namespace imgui_cxx
