#ifndef PHYSICS_FORCE_GENERATOR_HPP_
#define PHYSICS_FORCE_GENERATOR_HPP_

#include "physics/particle.hpp"

namespace phys {

class ParticleForceGenerator {
 public:
  virtual void ApplyForce(Particle* particle, Float time_step) = 0;
};

class ParticleGravityGenerator : public ParticleForceGenerator {
 public:
  explicit ParticleGravityGenerator(const Vector3& gravity = default_gravity_)
      : gravity_{gravity} {}

  void ApplyForce(Particle* particle, Float time_step) override {
    if (particle->HasFiniteMass()) particle->ApplyForce(gravity_);
  }

 private:
  static constexpr Vector3 default_gravity_{0, -9.8, 0};

  Vector3 gravity_;
};

class ParticleDragGenerator : public ParticleForceGenerator {
 public:
  explicit ParticleDragGenerator(Float k1 = default_k1_, Float k2 = default_k2_)
      : k1_{k1}, k2_{k2} {}

  void ApplyForce(Particle* particle, Float time_step) override {
    auto particle_speed = particle->GetSpeed();
    auto square_speed = particle_speed * particle_speed;
    auto direction = -glm::normalize(particle->GetVelocity());
    auto linear_term = k1_ * particle_speed;
    auto quadratic_term = k2_ * square_speed;
    particle->ApplyForce(direction * (linear_term + quadratic_term));
  }

 private:
  static constexpr Float default_k1_{0.001};
  static constexpr Float default_k2_{0.01125};

  Float k1_;
  Float k2_;
};

}  // namespace phys

#endif  // PHYSICS_FORCE_GENERATOR_HPP_