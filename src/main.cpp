#include "dbvt_broadphase.hpp"
#include "physics/graphical_particle.hpp"

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "VBAG 2"};

  Scene scene;

  phys::Particle particles[20];
  GraphicalParticle* graphical_particles[20];
  for (int i = 0; i < 20; ++i) {
    auto& particle = particles[i];
    particle.SetPosition({0.5 * (i - 10), 0, 0});
    particle.SetVelocity({0, 10, 0});
    graphical_particles[i] = new GraphicalParticle{particle};
  }

  auto cam = new Camera{"cam", glm::radians(74.0f), 16 / 9.0f, 0.01f, 1000.0f};
  cam->translate({0, 0, 10});

  auto light = new Light{"light_1", glm::vec3{1}, 10};
  light->translate({0, 4, 0});
  scene.addLight(light);
  light = new Light{"light_2", glm::vec3{1}, 10};
  light->translate({0, 0, 4});
  scene.addLight(light);
  light = new Light{"light_3", glm::vec3{1}, 10};
  light->translate({0, 0, -4});
  scene.addLight(light);

  // scene.addActor(ground);
  scene.addCamera(cam);
  for (auto& graphical_particle : graphical_particles)
    scene.addActor(graphical_particle);

  scene.render(window, [&] {
    for (auto& particle : particles) {
      particle.ClearForceAccumulator();
      particle.ApplyForce({0, -9.8, 0});
      particle.Integrate(scene.dt());
    }

    for (auto& graphical_particle : graphical_particles) {
      graphical_particle->Update();
    }
  });

  return 0;
}