#include "dbvt_broadphase.hpp"
#include "physics/graphical_particle.hpp"
#include "physics/particle_force_registry.hpp"

int main() {
  constexpr size_t w{1600}, h{900};
  Window window{w, h, "VBAG 2"};

  Scene scene;

  constexpr int particle_count = 3;
  phys::Particle particles[particle_count * particle_count];
  GraphicalParticle* graphical_particles[particle_count * particle_count];
  for (int i = 0; i < particle_count; ++i) {
    for (int j = 0; j < particle_count; ++j) {
      auto index = i * particle_count + j;
      auto& particle = particles[index];
      particle.SetPosition(
          {0.5 * (i - particle_count / 2), 0, 0.5 * (j - particle_count / 2)});
      // particle.SetVelocity({0, 10, 0});
      graphical_particles[index] = new GraphicalParticle{particle};
    }
  }

  particles[0].SetInverseMass(0);
  particles[0].SetPosition({-2, 0, 0});

  phys::ParticleGravity gravity;
  phys::ParticleDrag drag;
  phys::ParticleSpring spring_0{&particles[0], 0.5, 10};
  phys::ParticleSpring spring_1{&particles[1], 0.5, 10};

  phys::ParticleForceRegistry registry;

  registry.Register(&particles[1], &spring_0);

  for (auto& particle : particles) {
    registry.Register(&particle, &gravity);
    registry.Register(&particle, &drag);
    if (&particle != particles + 1) {
      registry.Register(&particle, &spring_1);
      registry.Register(&particles[1],
                        new phys::ParticleSpring(&particle, 0.5, 10));
    }
  }

  auto cam = new Camera{"cam", glm::radians(74.0f), 16 / 9.0f, 0.01f, 1000.0f};
  cam->translate({0, 0, 5});

  auto light = new Light{"light_1", glm::vec3{1}, 100 * 100};
  light->translate({0, 100, 0});
  scene.addLight(light);
  light = new Light{"light_2", glm::vec3{1}, 100 * 100};
  light->translate({0, -100, 0});
  scene.addLight(light);

  scene.addCamera(cam);
  for (auto& graphical_particle : graphical_particles)
    scene.addActor(graphical_particle);

  scene.render(window, [&] {
    float time_step = scene.timeStep();
    registry.ApplyForces(time_step);
    for (auto& particle : particles) {
      particle.Integrate(time_step);
      particle.ClearForceAccumulator();
    }

    for (auto& graphical_particle : graphical_particles) {
      graphical_particle->Update();
    }
  });

  return 0;
}