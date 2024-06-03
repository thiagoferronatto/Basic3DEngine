#ifndef PHYSICS_FORCE_GENERATOR_HPP_
#define PHYSICS_FORCE_GENERATOR_HPP_

#include "glm/gtx/projection.hpp"
#include "physics/particle.hpp"

namespace phys {

class ParticleForceGenerator {
 public:
  virtual void ApplyForce(Particle* particle, Float time_step) = 0;
};

class ParticleGravity : public ParticleForceGenerator {
 public:
  explicit ParticleGravity(const Vector3& gravity = default_gravity_)
      : gravity_{gravity} {}

  void ApplyForce(Particle* particle, Float time_step) override {
    if (particle->HasFiniteMass())
      particle->ApplyForce(gravity_ * particle->GetMass());
  }

 private:
  static constexpr Vector3 default_gravity_{0, -9.8, 0};

  Vector3 gravity_;
};

class ParticleDrag : public ParticleForceGenerator {
 public:
  explicit ParticleDrag(Float k_1 = default_k_1_, Float k_2 = default_k_2_)
      : k_1_{k_1}, k_2_{k_2} {}

  void ApplyForce(Particle* particle, Float time_step) override {
    auto particle_speed = particle->GetSpeed();
    if (particle_speed == 0) return;
    auto square_speed = particle_speed * particle_speed;
    auto direction = -glm::normalize(particle->GetVelocity());
    auto linear_term = k_1_ * particle_speed;
    auto quadratic_term = k_2_ * square_speed;
    particle->ApplyForce(direction * (linear_term + quadratic_term));
  }

 private:
  static constexpr Float default_k_1_{0.001};
  static constexpr Float default_k_2_{0.01125};

  Float k_1_;  // Linear drag coefficient
  Float k_2_;  // Quadratic drag coefficient
};

class ParticleSpring : public ParticleForceGenerator {
 public:
  ParticleSpring(const Particle* anchor, Float spring_length,
                 Float k = default_k_)
      : anchor_{anchor}, spring_length_{spring_length}, k_{k} {}

  void ApplyForce(Particle* particle, Float time_step) override {
    auto direction = particle->GetPosition() - anchor_->GetPosition();
    auto direction_length = glm::length(direction);
    if (direction_length == 0) return;
    auto length_difference = direction_length - spring_length_;
    direction /= direction_length;
    auto force = -k_ * length_difference * direction;
    particle->ApplyForce(force);
  }

 private:
  static constexpr Float default_k_{1};

  const Particle* anchor_;
  Float spring_length_;
  Float k_;
};

class ParticleFixedSpring : public ParticleForceGenerator {
 public:
  ParticleFixedSpring(const Vector3& anchor, Float spring_length,
                      Float k = default_k_)
      : anchor_{anchor}, spring_length_{spring_length}, k_{k} {}

  void ApplyForce(Particle* particle, Float time_step) override {
    auto direction = particle->GetPosition() - anchor_;
    auto direction_length = glm::length(direction);
    if (direction_length == 0) return;
    auto length_difference = direction_length - spring_length_;
    direction /= direction_length;
    auto force = -k_ * length_difference * direction;
    particle->ApplyForce(force);
  }

 private:
  static constexpr Float default_k_{1};

  Vector3 anchor_;
  Float spring_length_;
  Float k_;
};

class ParticleFixedRope : public ParticleForceGenerator {
 public:
  ParticleFixedRope(const Vector3& anchor, Float rope_length,
                    Float k = default_k_)
      : anchor_{anchor}, rope_length_{rope_length}, k_{k} {}

  void ApplyForce(Particle* particle, Float time_step) override {
    Float d = 0.5;
    auto gamma = 0.5f * std::sqrt(4 * k_ - d);
    auto c = (0.5f * d / gamma) * anchor_;
    auto scaled_time_step = gamma * time_step;
    auto position = (anchor_ * std::cos(scaled_time_step) +
                     c * std::sin(scaled_time_step)) *
                    std::exp(0.5f * d * time_step);
    auto acceleration = (position - anchor_) / (time_step * time_step);
    auto force = particle->GetMass() * acceleration;
    particle->ApplyForce(force);
  }

 private:
  static constexpr Float default_k_{1};

  Vector3 anchor_;
  Float rope_length_;
  Float k_;
};

}  // namespace phys

#endif  // PHYSICS_FORCE_GENERATOR_HPP_