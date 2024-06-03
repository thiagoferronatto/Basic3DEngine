#ifndef PHYSICS_PARTICLE_HPP_
#define PHYSICS_PARTICLE_HPP_

#include "common.hpp"

namespace phys {

class Particle {
 public:
  Particle() = default;

  explicit Particle(Float mass) : mass_{mass} {}

  void SetMass(Float mass) {
    assert(mass > 0);
    inverse_mass_ = 1 / (mass_ = mass);
  }

  void SetInverseMass(Float inverse_mass) {
    assert(inverse_mass > 0);
    mass_ = 1 / (inverse_mass_ = inverse_mass);
  }

  void SetPosition(const Vector3& position) { position_ = position; }

  void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

  Float GetMass() const { return mass_; }

  Float GetInverseMass() const { return inverse_mass_; }

  const Vector3& GetPosition() const { return position_; }

  const Vector3& GetVelocity() const { return velocity_; }

  void Integrate(Float dt) {
    assert(dt >= 0);
    position_ += velocity_ * dt;
    acceleration_ = accumulated_force_ * inverse_mass_;
    velocity_ += acceleration_ * dt;
    velocity_ *= std::pow(damping_, dt);
  }

  void ApplyForce(const Vector3& force) { accumulated_force_ += force; }

  void ClearForceAccumulator() { accumulated_force_ = {}; }

 protected:
  Vector3 position_{};
  Vector3 velocity_{};
  Vector3 acceleration_{};
  Vector3 accumulated_force_{};
  Float inverse_mass_{1};
  Float mass_{1};
  Float damping_{0.9};
};

}  // namespace phys

#endif  // PHYSICS_PARTICLE_HPP_