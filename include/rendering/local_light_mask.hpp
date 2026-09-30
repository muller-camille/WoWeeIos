#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace wowee {
namespace rendering {

/// Which of the frame's local lights can reach a draw, as a bit per light.
///
/// Every lit pixel used to walk the frame's whole list - on a phone or tablet
/// the nearest sixteen - and each light it walked cost its distance test even
/// where the lamp was a hundred yards off. A draw only needs the lamps whose
/// reach touches it, and nearly always that is none or a few, so the list is
/// narrowed once per draw on the CPU and the shader skips the rest before
/// reading them.
///
/// The test is sphere against sphere: the light's position and radius, the
/// draw's bounds as a centre and a radius. It may pass a light that misses
/// every pixel of the draw; it never drops one that reaches any.
///
/// Thirty-two bits for thirty-two lights. A list longer than that (the
/// desktop's sixty-four) answers every bit set, and the shaders test only the
/// first thirty-two against the mask - so past that nothing is skipped, which
/// is what they did before.
inline uint32_t localLightMask(const glm::vec4* posRadius, uint32_t count,
                               const glm::vec3& center, float radius) {
    if (count > 32) return 0xFFFFFFFFu;
    uint32_t mask = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const float reach = posRadius[i].w + radius;
        if (posRadius[i].w <= 0.0f) continue;
        const glm::vec3 d = glm::vec3(posRadius[i]) - center;
        if (glm::dot(d, d) < reach * reach) mask |= 1u << i;
    }
    return mask;
}

/// The frame's lights as the renderer gathered them, for a sub-renderer to
/// narrow per draw. Points into the per-frame data; valid for the frame.
struct LocalLightList {
    const glm::vec4* posRadius = nullptr;
    uint32_t count = 0;

    [[nodiscard]] uint32_t maskFor(const glm::vec3& center, float radius) const {
        return posRadius ? localLightMask(posRadius, count, center, radius) : 0xFFFFFFFFu;
    }
};

}  // namespace rendering
}  // namespace wowee
