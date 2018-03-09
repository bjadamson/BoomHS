#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl_gl3.h>

namespace imgui_cxx
{

template<typename FN, typename ...Args>
void
with_stylevars(FN const& fn, Args &&... args)
{
  ImGui::PushStyleVar(FORWARD(args)...);
  fn();
  ImGui::PopStyleVar();
}

} // ns imgui_cxx
