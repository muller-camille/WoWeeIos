#pragma once

#include <glm/glm.hpp>

#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

namespace wowee {
namespace rendering {

/// Where a world position falls in a water surface's own grid.
///
/// A surface is an origin and two step vectors, one per grid axis, so a lake
/// lying at an angle inside a WMO is described the same way as a chunk of open
/// sea. Everything that asks a question about the water under a point starts
/// by projecting that point onto those two axes: the height under the player,
/// the nearest surface, the liquid type, and whether the water belongs to a
/// building. water_renderer.cpp did it four times.
///
/// Answers nothing when the point is outside the surface, and when the surface
/// is degenerate. A step vector of zero length would divide by zero and put
/// every point in the world inside that surface, which is worth a check rather
/// than a NaN: a surface built from a malformed chunk would otherwise claim
/// the whole map and the player would swim through the air over it.
///
/// The bounds are inclusive at both ends, because a grid of `width` cells has
/// `width + 1` corners and the far edge belongs to the surface.
///
/// The result is in grid units, not cells: 2.5 is halfway along the third
/// column, which is what the callers interpolate between corners with.
inline std::optional<glm::vec2> surfaceGridPosition(
        const glm::vec3& origin, const glm::vec3& stepX, const glm::vec3& stepY,
        uint8_t width, uint8_t height, float worldX, float worldY) {
    const glm::vec2 relative(worldX - origin.x, worldY - origin.y);
    const glm::vec2 axisX(stepX.x, stepX.y);
    const glm::vec2 axisY(stepY.x, stepY.y);

    const float lengthSqX = glm::dot(axisX, axisX);
    const float lengthSqY = glm::dot(axisY, axisY);
    if (lengthSqX < 1e-6f || lengthSqY < 1e-6f) return std::nullopt;

    const glm::vec2 grid(glm::dot(relative, axisX) / lengthSqX,
                         glm::dot(relative, axisY) / lengthSqY);
    if (grid.x < 0.0f || grid.x > static_cast<float>(width) ||
        grid.y < 0.0f || grid.y > static_cast<float>(height)) {
        return std::nullopt;
    }
    return grid;
}


/// The water height at a grid position inside a surface, interpolated between
/// the four corners of the cell it lands in.
///
/// A surface's heights are a (width + 1) by (height + 1) grid of corners, so a
/// grid position of exactly `width` is the far edge rather than a cell start.
/// That case is folded back onto the last cell with a fraction of one, which
/// is what keeps two neighbouring surfaces meeting at the same height instead
/// of leaving a seam the player falls through.
///
/// Answers nothing when the position is behind the grid or the height array is
/// too short for the cell, which happens for a surface whose heights failed to
/// load: better no answer than a height read from whatever follows.
inline std::optional<float> sampleGridHeight(const std::vector<float>& heights,
                                             uint8_t width, uint8_t height,
                                             float gx, float gy) {
    // Rejected before the cast, because casting -0.5 to int gives 0 and the
    // interpolation would then extrapolate below the grid with a negative
    // fraction. Every caller reaches here through surfaceGridPosition, which
    // has already refused anything behind the surface, so this only makes the
    // contract match what the callers rely on.
    if (gx < 0.0f || gy < 0.0f) return std::nullopt;

    int ix = static_cast<int>(gx);
    int iy = static_cast<int>(gy);
    float fx = gx - static_cast<float>(ix);
    float fy = gy - static_cast<float>(iy);

    if (ix >= static_cast<int>(width)) { ix = static_cast<int>(width) - 1; fx = 1.0f; }
    if (iy >= static_cast<int>(height)) { iy = static_cast<int>(height) - 1; fy = 1.0f; }
    if (ix < 0 || iy < 0) return std::nullopt;

    const int gridWidth = static_cast<int>(width) + 1;
    const int idx00 = iy * gridWidth + ix;
    const int idx10 = idx00 + 1;
    const int idx01 = idx00 + gridWidth;
    const int idx11 = idx01 + 1;
    if (idx11 >= static_cast<int>(heights.size())) return std::nullopt;

    const float h00 = heights[static_cast<size_t>(idx00)];
    const float h10 = heights[static_cast<size_t>(idx10)];
    const float h01 = heights[static_cast<size_t>(idx01)];
    const float h11 = heights[static_cast<size_t>(idx11)];
    return h00 * (1 - fx) * (1 - fy) + h10 * fx * (1 - fy) +
           h01 * (1 - fx) * fy + h11 * fx * fy;
}

/// The corner heights a liquid layer is drawn at, with the ones that cannot
/// be real replaced rather than the layer's.
///
/// MH2O stores a height for every corner of a layer's rect, including the
/// corners no existing cell touches, and those are often left at zero. The
/// renderer used to see one such corner, call the whole array insane and lay
/// the layer flat at its minimum. That is harmless on a flat sheet and wrong
/// on a layer spanning two levels: in Shadowglen a pond steps down 1.4 yards
/// inside one chunk, and flattened to the lower level its upper cells went
/// under the pond bed, a dry strip straight across the water.
///
/// A corner is taken when it is finite and within eight yards of the layer's
/// own range; otherwise it borrows the mean of its valid neighbours along the
/// grid, and failing those the minimum. Every corner of an existing cell is
/// real in the data seen so far, so the borrowing only decides where unused
/// corners sit, and keeps them near their neighbours if one is ever shared.
inline std::vector<float> liquidCornerHeights(const std::vector<float>& heights,
                                              int width, int height,
                                              float minHeight, float maxHeight) {
    const int gridW = width + 1;
    const int gridH = height + 1;
    const size_t count = static_cast<size_t>(gridW) * static_cast<size_t>(gridH);
    std::vector<float> out(count, minHeight);
    if (heights.size() != count) return out;

    const auto valid = [&](float h) {
        return std::isfinite(h) && std::abs(h) <= 50000.0f &&
               h >= minHeight - 8.0f && h <= maxHeight + 8.0f;
    };
    for (int y = 0; y < gridH; ++y) {
        for (int x = 0; x < gridW; ++x) {
            const size_t i = static_cast<size_t>(y) * gridW + x;
            if (valid(heights[i])) { out[i] = heights[i]; continue; }
            float sum = 0.0f;
            int n = 0;
            const int nx[4] = {x - 1, x + 1, x, x};
            const int ny[4] = {y, y, y - 1, y + 1};
            for (int k = 0; k < 4; ++k) {
                if (nx[k] < 0 || nx[k] >= gridW || ny[k] < 0 || ny[k] >= gridH) continue;
                const float h = heights[static_cast<size_t>(ny[k]) * gridW + nx[k]];
                if (valid(h)) { sum += h; ++n; }
            }
            if (n > 0) out[i] = sum / static_cast<float>(n);
        }
    }
    return out;
}

}  // namespace rendering
}  // namespace wowee
