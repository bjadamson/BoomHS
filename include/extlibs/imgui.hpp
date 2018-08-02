#pragma once
#include <opengl/colors.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl_gl3.h>

#include <initializer_list>
#include <string>
#include <utility>

namespace imgui_cxx
{

inline ImVec4
from_color(opengl::Color const& c)
{
  auto const* data = c.data();
  return ImVec4{data[0], data[1], data[2], data[3]};
}

template <typename... Args>
inline void
text_wrapped_colored(ImVec4 const& c, char const* fmt, Args&&... args)
{
  ImGui::PushStyleColor(ImGuiCol_Text, c);
  ImGui::TextWrapped(fmt, FORWARD(args));
  ImGui::PopStyleColor();
}

template <typename T>
auto
combo(std::string const& init, T* buffer, std::string const& list)
{
  return ImGui::Combo(init.c_str(), buffer, list.c_str());
}

template <typename T, size_t N>
auto
combo_from_array(char const* init_text, char const* list_text, int* buffer,
                 std::array<T, N> const& list)
{
  ImGui::Combo(init_text, buffer, list_text);
  auto const v = static_cast<uint64_t>(*buffer);

  FOR(i, list.size())
  {
    if (v == i) {
      return list[i];
    }
  }

  std::abort();
}

template <typename FN, typename... Args>
auto
with_combo(FN const& fn, Args&&... args)
{
  if (ImGui::BeginCombo(FORWARD(args))) {
    ON_SCOPE_EXIT([]() { ImGui::EndCombo(); });
    fn();
  }
}

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

template <typename... Args>
auto
input_sizet(char const* label, size_t* data, Args&&... args)
{
  int* casted = reinterpret_cast<int*>(data);
  return ImGui::InputInt(label, casted, FORWARD(args));
}

inline auto
main_menu_bar_size()
{
  ImGui::BeginMainMenuBar();
  ON_SCOPE_EXIT([]() { ImGui::EndMainMenuBar(); });

  auto const main_menu_size = ImGui::GetWindowSize();
  return main_menu_size;
}

template <typename FN, typename... Args>
auto
with_window(FN const& fn, Args&&... args)
{
  if (ImGui::Begin(FORWARD(args))) {
    ON_SCOPE_EXIT([]() { ImGui::End(); });
    return fn();
  }
  else {
    // not sure why this would ever happen, look it up..
    std::abort();
  }
}

template <typename FN, typename... Args>
void
with_window_logerrors(stlw::Logger& logger, FN const& fn, Args&&... args)
{
  auto r = with_window(fn, FORWARD(args));
  if (r.isErr()) {
    LOG_ERROR(r.unwrapErr());
  }
}

template <typename FN, typename... Args>
void
with_stylevars(FN const& fn, Args&&... args)
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
    ON_SCOPE_EXIT([]() { ImGui::EndChild(); });
    fn();
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
    ON_SCOPE_EXIT([]() { ImGui::EndMenu(); });
    fn();
  }
}

template <typename FN, typename... Args>
void
with_mainmenubar(FN const& fn, Args&&... args)
{
  if (ImGui::BeginMainMenuBar(FORWARD(args))) {
    ON_SCOPE_EXIT([]() { ImGui::EndMainMenuBar(); });
    fn();
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

#define IMGUI_PUSH_STYLEVAR_SCOPE_EXIT(NAME, VAL)                                                  \
  ImGui::PushStyleVar(NAME, VAL);                                                                  \
  ON_SCOPE_EXIT([]() { ImGui::PopStyleVar(); });

} // namespace imgui_cxx
