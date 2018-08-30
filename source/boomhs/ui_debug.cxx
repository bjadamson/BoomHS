#include <boomhs/bounding_object.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/components.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/player.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/view_frustum.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glm.hpp>
#include <extlibs/imgui.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

void
draw_entity_editor(char const* prefix, int const window_flags, EngineState& es, LevelManager& lm,
                   EntityRegistry& registry, Camera& camera,
                   glm::mat4 const& view_mat, glm::mat4 const& proj_mat)
{
  auto& logger    = es.logger;
  auto& zs        = lm.active();
  auto& gfx_state = zs.gfx_state;
  auto& draw_handles = gfx_state.draw_handles;
  auto& sps       = gfx_state.sps;

  auto& uistate   = es.ui_state.debug;

  auto const draw = [&]() {
    std::optional<EntityID> selected;
    for (auto const eid : find_all_entities_with_component<Selectable>(registry)) {
      auto const& sel = registry.get<Selectable>(eid);
      if (sel.selected) {
        selected = eid;
        break;
      }
    }

    std::string const eid_str = selected ? std::to_string(*selected) : "none";
    ImGui::Text("eid: %s", eid_str.c_str());
    if (!selected) {
      return;
    }
    ImGui::Checkbox("Lock Selected", &uistate.lock_debugselected);
    auto const eid = *selected;

    {
      auto& isr = registry.get<IsRenderable>(eid).hidden;
      ImGui::Checkbox("Hidden From Rendering", &isr);
    }

    if (registry.has<AABoundingBox>(eid) && registry.has<Transform>(eid)) {
      auto const& tr = registry.get<Transform>(eid);
      auto const& bbox = registry.get<AABoundingBox>(eid);

      // TODO: view/proj matrix
      bool const bbox_inside = ViewFrustum::bbox_inside(view_mat, proj_mat, tr, bbox);
      std::string const msg = fmt::sprintf("In ViewFrustum: %i", bbox_inside);
      ImGui::Text("%s", msg.c_str());
    }
    if (ImGui::Button("Inhabit Selected")) {
      auto& transform = registry.get<Transform>(eid);
      //camera.set_target(transform);
    }

    if (registry.has<Name>(eid)) {
      auto& name        = registry.get<Name>(eid).value;
      char  buffer[128] = {'0'};
      FOR(i, name.size()) { buffer[i] = name[i]; }
      ImGui::InputText(name.c_str(), buffer, IM_ARRAYSIZE(buffer));
    }
    if (registry.has<AABoundingBox>(eid)) {
      if (ImGui::CollapsingHeader("BoundingBox Editor")) {
        auto const& bbox = registry.get<AABoundingBox>(eid).cube;
        ImGui::Text("min: %s", glm::to_string(bbox.min).c_str());
        ImGui::Text("max: %s", glm::to_string(bbox.max).c_str());
      }
    }
    if (ImGui::CollapsingHeader("Transform Editor")) {
      auto& transform = registry.get<Transform>(eid);
      ImGui::InputFloat3("pos:", glm::value_ptr(transform.translation));
      {
        glm::vec3 buffer = transform.get_rotation_degrees();
        if (ImGui::InputFloat3("quat rot:", glm::value_ptr(buffer))) {
          transform.rotation = glm::quat{glm::radians(buffer)};
        }
        ImGui::Text("euler rot:%s", glm::to_string(transform.get_rotation_degrees()).c_str());
      }
      ImGui::InputFloat3("scale:", glm::value_ptr(transform.scale));
    }

    if (registry.has<TreeComponent>(eid) && ImGui::CollapsingHeader("Tree Editor")) {
      auto const make_str = [](char const* text, auto const num) {
        return text + std::to_string(num);
      };
      auto& tc = registry.get<TreeComponent>(eid);

      auto const edit_treecolor = [&](char const* name, auto const num_colors,
                                      auto const& get_color) {
        FOR(i, num_colors)
        {
          auto const text = make_str(name, i);
          ImGui::ColorEdit4(text.c_str(), get_color(i), ImGuiColorEditFlags_Float);
        }
      };

      edit_treecolor("Trunk", tc.num_trunks(),
                     [&tc](auto const i) { return tc.trunk_color(i).data(); });
      edit_treecolor("Stem", tc.num_stems(),
                     [&tc](auto const i) { return tc.stem_color(i).data(); });
      edit_treecolor("Leaves", tc.num_leaves(),
                     [&tc](auto const i) { return tc.leaf_color(i).data(); });

      auto& sn = registry.get<ShaderName>(eid);
      auto& va = sps.ref_sp(sn.value).va();

      auto& dinfo = draw_handles.lookup_entity(logger, eid);
      Tree::update_colors(logger, va, dinfo, tc);
    }

    if (registry.has<PointLight>(eid) && ImGui::CollapsingHeader("Pointlight")) {
      auto& transform  = registry.get<Transform>(eid);
      auto& pointlight = registry.get<PointLight>(eid);
      auto& light      = pointlight.light;
      ImGui::InputFloat3("position:", glm::value_ptr(transform.translation));
      ImGui::ColorEdit3("diffuse:", light.diffuse.data());
      ImGui::ColorEdit3("specular:", light.specular.data());
      ImGui::Separator();

      ImGui::Text("Attenuation");
      auto& attenuation = pointlight.attenuation;
      ImGui::InputFloat("constant:", &attenuation.constant);
      ImGui::InputFloat("linear:", &attenuation.linear);
      ImGui::InputFloat("quadratic:", &attenuation.quadratic);

      if (registry.has<LightFlicker>(eid)) {
        ImGui::Separator();
        ImGui::Text("Light Flicker");

        auto& flicker = registry.get<LightFlicker>(eid);
        ImGui::InputFloat("speed:", &flicker.current_speed);

        FOR(i, flicker.colors.size())
        {
          auto& light = flicker.colors[i];
          ImGui::ColorEdit4("color:", light.data(), ImGuiColorEditFlags_Float);
          ImGui::Separator();
        }
      }
    }
    if (registry.has<Material>(eid) && ImGui::CollapsingHeader("Material")) {
      auto& material = registry.get<Material>(eid);
      ImGui::ColorEdit3("ambient:", glm::value_ptr(material.ambient));
      ImGui::ColorEdit3("diffuse:", glm::value_ptr(material.diffuse));
      ImGui::ColorEdit3("specular:", glm::value_ptr(material.specular));
      ImGui::SliderFloat("shininess:", &material.shininess, 0.0f, 1.0f);
    }
  };

  auto const title = std::string{prefix} + ":Entity Editor Window";
  imgui_cxx::with_window(draw, title.c_str(), nullptr, window_flags);
}

} // namespace

namespace boomhs::ui_debug
{

void
draw(char const* prefix, int const window_flags, EngineState& es, LevelManager& lm, Camera& camera,
     FrameTime const& ft)
{
  auto& uistate        = es.ui_state.debug;
  auto& zs             = lm.active();
  auto& registry       = zs.registry;
  auto& ldata          = zs.level_data;

  auto& player = find_player(registry);

  if (uistate.show_entitywindow) {
    auto const fs = FrameState::from_camera(es, zs, camera, camera.view_settings_ref(), es.frustum);
    auto const& view_mat = fs.view_matrix();
    auto const& proj_mat = fs.projection_matrix();
    draw_entity_editor(prefix, window_flags, es, lm, registry, camera, view_mat, proj_mat);
  }
}

} // namespace boomhs::ui_debug
