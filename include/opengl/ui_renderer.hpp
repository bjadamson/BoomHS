#pragma once
#include <boomhs/color.hpp>
#include <boomhs/math.hpp>
#include <boomhs/viewport.hpp>
#include <common/log.hpp>

// TODO: this header is only included for static_shaders::BasicMvWithUniformColor
#include <opengl/draw_info.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/types.hpp>

namespace opengl
{
class  DrawInfo;
struct DrawState;
//class  ShaderProgram;

namespace static_shaders
{

class BasicMvWithUniformColor
{
  ShaderProgram sp_;

  BasicMvWithUniformColor(ShaderProgram&& sp) : sp_(MOVE(sp)) {}
public:

  auto& sp() { return sp_; }

  static BasicMvWithUniformColor create(common::Logger&);
};

} // namespace static_shaders

struct ProgramAndDrawInfo
{
  static_shaders::BasicMvWithUniformColor program;
  DrawInfo dinfo;

  // ctor
  ProgramAndDrawInfo(static_shaders::BasicMvWithUniformColor&&, DrawInfo&&);

  static ProgramAndDrawInfo create_rect(common::Logger&, boomhs::RectFloat const&,
                                        static_shaders::BasicMvWithUniformColor&&, GLenum);
  static ProgramAndDrawInfo create_rect(common::Logger&, boomhs::RectFloat const&, GLenum);
};
using MouseRectangle = ProgramAndDrawInfo;

class UiRenderer
{
  static_shaders::BasicMvWithUniformColor sp2d_;

  glm::mat4       pm_;

  void draw_rect_impl(common::Logger&, boomhs::ModelViewMatrix const&, DrawInfo&,
                       boomhs::Color const&, GLenum, ShaderProgram&, DrawState&);

  UiRenderer(static_shaders::BasicMvWithUniformColor&&, boomhs::Viewport const&);

  NO_MOVE_OR_COPY(UiRenderer);
public:

  void resize(boomhs::Viewport const&);

  void draw_rect(common::Logger&, boomhs::ModelMatrix const&, DrawInfo&, boomhs::Color const&,
                       GLenum, DrawState&);
  void draw_rect(common::Logger&, DrawInfo&, boomhs::Color const&, GLenum, DrawState&);

  template <typename ...Args>
  static auto
  create(common::Logger& logger, Args&&... args)
  {
    auto program = static_shaders::BasicMvWithUniformColor::create(logger);
    return UiRenderer{MOVE(program), FORWARD(args)};
  }
};

// TODO: Rename this class to something more general/sane.
//
// Render's a rectangle from an initial click location to where the user is currently clicking,
// within a supplied viewport. Maintains it's own state, but needs to be updated when the mouse
// is moved.
//
// Used for click-and-drag selection operations.
class MouseRectangleRenderer
{
  MouseRectangle mouse_rect_;
  UiRenderer     ui_renderer_;

  static ProgramAndDrawInfo
  create_rect(common::Logger& logger, glm::ivec2 const& initial_click_pos,
              glm::ivec2 const& mouse_pos_now, GLenum const dm)
  {
    float const minx = initial_click_pos.x;
    float const miny = initial_click_pos.y;
    float const maxx = mouse_pos_now.x;
    float const maxy = mouse_pos_now.y;
    boomhs::RectFloat const mouse_rect{minx, miny, maxx, maxy};

    return ProgramAndDrawInfo::create_rect(logger, mouse_rect, dm);
  }

public:
  MouseRectangleRenderer(common::Logger& logger, MouseRectangle&& mrect, boomhs::Viewport const& viewport)
      : mouse_rect_(MOVE(mrect))
      , ui_renderer_(UiRenderer::create(logger, viewport))
  {
  }

  void
  draw(common::Logger& logger, boomhs::Color const& color, DrawState& ds)
  {
    ui_renderer_.draw_rect(logger, mouse_rect_.dinfo, color, GL_LINE_LOOP, ds);
  }

  static auto
  make_rect_under_mouse(common::Logger& logger, glm::ivec2 const& init, glm::ivec2 const& now,
                         boomhs::Viewport const& viewport)
  {
    auto mouse_rect = create_rect(logger, init, now, GL_LINE_LOOP);
    return MouseRectangleRenderer{logger, MOVE(mouse_rect), viewport};
  }

  static void
  draw_mouseselect_rect(common::Logger& logger, glm::ivec2 const& pos_init,
                        glm::ivec2 const& pos_now, boomhs::Color const& color,
                        boomhs::Viewport const& viewport, DrawState& ds)
  {
    auto mrr = MouseRectangleRenderer::make_rect_under_mouse(logger, pos_init, pos_now,
                                                             viewport);
    mrr.draw(logger, color, ds);
  };

  // Factory function for creating the RectFloat with points where the user last clicked, and the
  // mouse is currently.
  //
  // initial => mouse position (in screen space) where the user clicked.
  // now     => mouse position (in screen space) where the mouse is currently (think
  // click-and-drag).
  // viewport_orgin => Origin of the viewport (0, 0) being top-left of the screen.
  static auto
  make_mouse_rect(glm::ivec2 const& initial_ss, glm::ivec2 const& now_ss, glm::ivec2 const& vp_origin)
  {
    namespace sc = boomhs::math::space_conversions;

    // screen pos -> viewport pos
    auto const init_vp = sc::screen_to_viewport(initial_ss, vp_origin);
    auto const now_vp  = sc::screen_to_viewport(now_ss, vp_origin);

    // Create a rectangle using the two points, making sure that it works out if the user clicks
    // and drags up or down, left or right.
    return boomhs::math::rect_abs_from_twopoints(init_vp, now_vp);
  }
};

} // namespace opengl
