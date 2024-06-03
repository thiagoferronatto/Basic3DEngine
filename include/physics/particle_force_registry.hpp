#ifndef PHYSICS_PARTICLE_FORCE_REGISTRY_HPP_
#define PHYSICS_PARTICLE_FORCE_REGISTRY_HPP_

#include <utility>
#include <vector>

#include "physics/particle_force_generator.hpp"

namespace phys {

/// A registry that keeps track of what particles suffer effects from which
/// force generator. This is useful for forces that are constantly applied to
/// particles in a system, like gravity or drag.
class ParticleForceRegistry {
 public:
  /// Registers a particle-generator pair. Registering these means that the
  /// particle will be affected by the force generated by the force generator
  /// when ApplyForces is called.
  void Register(Particle* particle, ParticleForceGenerator* force_generator) {
    registrations_.push_back({particle, force_generator});
  }

  /// Undoes the registration of the particle-generator pair. Unregistering
  /// means that the particle will no longer be affected by the force generated
  /// by the force generator when ApplyForces is called.
  void Unregister(Particle* particle, ParticleForceGenerator* force_generator) {
    auto new_end = std::remove(registrations_.begin(), registrations_.end(),
                               Registration{particle, force_generator});
    registrations_.erase(new_end, registrations_.end());
  }

  /// Undoes all particle-generator pair registrations.
  void Clear() { registrations_.clear(); }

  /// Applies all forces generated by all force generators to all particles
  /// registered alongside them.
  void ApplyForces(Float time_step) const {
    for (auto& [particle, force_generator] : registrations_)
      force_generator->ApplyForce(particle, time_step);
  }

 private:
  using Registration = std::pair<Particle*, ParticleForceGenerator*>;

  std::vector<Registration> registrations_;
};

}  // namespace phys

#endif  // PHYSICS_PARTICLE_FORCE_REGISTRY_HPP_