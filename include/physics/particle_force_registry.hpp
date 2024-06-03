#ifndef PHYSICS_PARTICLE_FORCE_REGISTRY_HPP_
#define PHYSICS_PARTICLE_FORCE_REGISTRY_HPP_

#include <utility>
#include <vector>

#include "physics/particle_force_generator.hpp"

namespace phys {

class ParticleForceRegistry {
 public:
  void Register(Particle* particle, ParticleForceGenerator* force_generator) {
    registrations_.push_back({particle, force_generator});
  }

  void Unregister(Particle* particle, ParticleForceGenerator* force_generator) {
    auto new_end = std::remove(registrations_.begin(), registrations_.end(),
                               Registration{particle, force_generator});
    registrations_.erase(new_end, registrations_.end());
  }

  void Clear() { registrations_.clear(); }

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