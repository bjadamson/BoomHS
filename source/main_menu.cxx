#include <boomhs/main_menu.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <opengl/colors.hpp>

#include <extlibs/imgui.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

auto static constexpr WINDOW_FLAGS = (0
    | ImGuiWindowFlags_AlwaysAutoResize
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoTitleBar);

auto static constexpr STYLE_VARS = (0
    | ImGuiStyleVar_ChildRounding);

void
draw_menu(EngineState& es)
{
  auto const fn = [&]() {
    es.show_main_menu = !ImGui::Button("Play Game");
    ImGui::Separator();
    es.quit = ImGui::Button("Exit");
  };
  auto const draw_window = [&]() {
    imgui_cxx::with_window(fn, "Main Menu", nullptr, WINDOW_FLAGS);
  };
  imgui_cxx::with_stylevar(draw_window, STYLE_VARS, 5.0f);

}

} // ns anon

namespace boomhs::main_menu
{

void
draw(EngineState& es)
{
  boomhs::render::clear_screen(LOC::BLACK);
  draw_menu(es);
}

} // ns boomhs::main_menu
