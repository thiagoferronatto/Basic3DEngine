#ifndef PHYSICS_GRAPHICAL_PARTICLE_HPP_
#define PHYSICS_GRAPHICAL_PARTICLE_HPP_

#include "actor.hpp"
#include "particle.hpp"
#include "triangle_mesh.hpp"

class GraphicalParticle : public Actor {
 public:
  GraphicalParticle(phys::Particle& particle)
      : Actor{"tmp", particle_mesh_}, particle_{particle} {
    _name = std::string{"particle_"} + std::to_string(current_particle_id_++);
    // Make particles show up as red spheres
    material.Kd = {1, 0, 0};
    material.Ns = 0;
    this->scale(0.1);
  }

  void Update() { setPosition(particle_.GetPosition()); }

 protected:
 private:
  static inline TriangleMesh* particle_mesh_{
      new TriangleMesh{TriangleMeshData::fromObj("assets/icosphere.obj")}};

  static inline size_t current_particle_id_{};

  phys::Particle& particle_;
};

#endif  // PHYSICS_GRAPHICAL_PARTICLE_HPP_