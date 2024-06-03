#ifndef PHYSICS_COMMON_HPP_
#define PHYSICS_COMMON_HPP_

#include "custom_assert.hpp"
#include "glm/glm.hpp"

namespace phys {

// If double precision is required, change this one line
using Float = float;

using Vector3 = glm::tvec3<Float>;

}  // namespace phys

#endif  // PHYSICS_COMMON_HPP_