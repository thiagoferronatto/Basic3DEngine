#include "dbvt_broadphase.hpp"
#include "physics/graphical_particle.hpp"
#include "physics/particle_force_registry.hpp"

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "VBAG 2"};

  Scene scene;

  constexpr int particleCount = 10;
  phys::Particle particles[particleCount * particleCount];
  GraphicalParticle* graphical_particles[particleCount * particleCount];
  for (int i = 0; i < particleCount; ++i) {
    for (int j = 0; j < particleCount; ++j) {
      auto index = i * particleCount + j;
      auto& particle = particles[index];
      particle.SetPosition(
          {0.5 * (i - particleCount / 2), 0, 0.5 * (j - particleCount / 2)});
      particle.SetVelocity({0, 10, 0});
      graphical_particles[index] = new GraphicalParticle{particle};
    }
  }

  phys::ParticleGravityGenerator grav_gen;
  phys::ParticleDragGenerator drag_gen;
  phys::ParticleForceRegistry registry;

  for (auto& particle : particles) {
    registry.Register(&particle, &grav_gen);
    // registry.Register(&particle, &drag_gen);
  }

  auto cam = new Camera{"cam", glm::radians(74.0f), 16 / 9.0f, 0.01f, 1000.0f};
  cam->translate({0, 0, 10});

  auto light = new Light{"light_1", glm::vec3{1}, 100 * 100 * 100};
  light->translate({0, 100, 0});
  scene.addLight(light);

  scene.addCamera(cam);
  for (auto& graphical_particle : graphical_particles)
    scene.addActor(graphical_particle);

  scene.render(window, [&] {
    // Separate physics simulation
    float dt = scene.dt();
    registry.ApplyForces(dt);
    for (auto& particle : particles) {
      particle.Integrate(dt);
      particle.ClearForceAccumulator();
    }

    // Graphics are decoupled from the physics by default
    for (auto& graphical_particle : graphical_particles) {
      graphical_particle->Update();
    }
  });

  return 0;
}