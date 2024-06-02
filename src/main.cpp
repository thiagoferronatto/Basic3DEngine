#include "dbvt_broadphase.hpp"

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "VBAG 2"};

  Scene scene;

  auto planeMesh = new TriangleMesh{TriangleMesh{TriangleMeshData::plane()}};

  auto ground = new Actor{"ground", planeMesh};
  ground->scale(10);

  auto cam = new Camera{"cam", glm::radians(74.0f), 16.0f / 9.0f, 0.01f, 1000.0f};
  cam->translate({0, 1, 5});

  auto light = new Light{"light", glm::vec3{1}, 10};
  light->translate({0, 4, 0});

  scene.addActor(ground);
  scene.addCamera(cam);
  scene.addLight(light);

  scene.render(window);

  return 0;
}