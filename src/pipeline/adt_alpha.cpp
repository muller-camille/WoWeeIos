#include "pipeline/adt_alpha.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace wowee {
namespace pipeline {

namespace {

/// The last row and column of a four-bit map are not data.
///
/// A chunk's alpha map is stored 64 wide and only 63 of those are painted: the
/// file's last row and column carry whatever was left there, and the client
/// fills them from the row and column before. Without it every chunk ends in a
/// strip of something else, and since the strip is on two of the four sides,
/// the boundary between two chunks is a hard line - which is the ground
/// textures not quite lining up, in a grid across the whole world.
///
/// Only the four-bit form. The eight-bit and compressed maps are what a map
/// with "big alpha" carries, and those are painted to the edge.
void fixLastRowAndColumn(std::vector<uint8_t>& alpha) {
    if (alpha.size() < ALPHA_MAP_SIZE) return;
    constexpr size_t kLast = ALPHA_MAP_DIM - 1;
    for (size_t i = 0; i < ALPHA_MAP_DIM; ++i) {
        alpha[kLast * ALPHA_MAP_DIM + i] = alpha[(kLast - 1) * ALPHA_MAP_DIM + i];
    }
    // After the row, so the corner takes the value the row above it just did.
    for (size_t i = 0; i < ALPHA_MAP_DIM; ++i) {
        alpha[i * ALPHA_MAP_DIM + kLast] = alpha[i * ALPHA_MAP_DIM + kLast - 1];
    }
}

}  // namespace

bool decodeLayerAlpha(const MapChunk& chunk, size_t layerIdx,
                      std::vector<uint8_t>& outAlpha, uint8_t unsetFill) {
    outAlpha.assign(ALPHA_MAP_SIZE, unsetFill);

    if (layerIdx >= chunk.layers.size()) return false;
    const auto& layer = chunk.layers[layerIdx];
    if (!layer.useAlpha() || layer.offsetMCAL >= chunk.alphaMap.size()) return false;

    const size_t offset = layer.offsetMCAL;

    // How much of MCAL belongs to this layer, taken from where the next layer
    // with an alpha map starts rather than from what is left in the blob. The
    // difference decides between the 4-bit and 8-bit forms below, so reading
    // "everything remaining" would misidentify every layer but the last.
    size_t layerSize = chunk.alphaMap.size() - offset;
    for (size_t j = layerIdx + 1; j < chunk.layers.size(); ++j) {
        if (chunk.layers[j].useAlpha()) {
            layerSize = chunk.layers[j].offsetMCAL - offset;
            break;
        }
    }

    if (layer.compressedAlpha()) {
        size_t readPos = offset;
        size_t writePos = 0;
        while (writePos < ALPHA_MAP_SIZE && readPos < chunk.alphaMap.size()) {
            const uint8_t cmd = chunk.alphaMap[readPos++];
            const bool fill = (cmd & ALPHA_FILL_FLAG) != 0;
            const int count = (cmd & ALPHA_COUNT_MASK) + 1;

            if (fill) {
                if (readPos >= chunk.alphaMap.size()) break;
                const uint8_t val = chunk.alphaMap[readPos++];
                for (int i = 0; i < count && writePos < ALPHA_MAP_SIZE; ++i) {
                    outAlpha[writePos++] = val;
                }
            } else {
                for (int i = 0;
                     i < count && writePos < ALPHA_MAP_SIZE && readPos < chunk.alphaMap.size();
                     ++i) {
                    outAlpha[writePos++] = chunk.alphaMap[readPos++];
                }
            }
        }
        return true;
    }

    if (layerSize >= ALPHA_MAP_SIZE) {
        std::copy(chunk.alphaMap.begin() + static_cast<std::ptrdiff_t>(offset),
                  chunk.alphaMap.begin() + static_cast<std::ptrdiff_t>(offset + ALPHA_MAP_SIZE),
                  outAlpha.begin());
        return true;
    }

    if (layerSize >= ALPHA_MAP_PACKED &&
        offset + ALPHA_MAP_PACKED <= chunk.alphaMap.size()) {
        // 4 bits per texel: low nibble first, scaled 0-15 to 0-255 by 17.
        for (size_t i = 0; i < ALPHA_MAP_PACKED; ++i) {
            const uint8_t v = chunk.alphaMap[offset + i];
            outAlpha[i * 2] = static_cast<uint8_t>((v & 0x0F) * 17);
            outAlpha[i * 2 + 1] = static_cast<uint8_t>((v >> 4) * 17);
        }
        fixLastRowAndColumn(outAlpha);
        return true;
    }

    return false;
}

size_t alphaTexelIndex(float u, float v) {
    const auto clamp01 = [](float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); };
    constexpr float kPainted = static_cast<float>(ALPHA_MAP_DIM - 1);
    const auto x = static_cast<size_t>(clamp01(u) * kPainted);
    const auto y = static_cast<size_t>(clamp01(v) * kPainted);
    return y * ALPHA_MAP_DIM + x;
}

float sampleAlpha(const std::vector<uint8_t>& alpha, float u, float v) {
    if (alpha.size() < ALPHA_MAP_SIZE) return 0.0f;
    const auto clamp01 = [](float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); };

    const float fx = clamp01(u) * static_cast<float>(ALPHA_MAP_DIM - 1);
    const float fy = clamp01(v) * static_cast<float>(ALPHA_MAP_DIM - 1);
    const auto x0 = static_cast<size_t>(fx);
    const auto y0 = static_cast<size_t>(fy);
    const size_t x1 = std::min(x0 + 1, ALPHA_MAP_DIM - 1);
    const size_t y1 = std::min(y0 + 1, ALPHA_MAP_DIM - 1);
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);

    const float a00 = static_cast<float>(alpha[y0 * ALPHA_MAP_DIM + x0]);
    const float a10 = static_cast<float>(alpha[y0 * ALPHA_MAP_DIM + x1]);
    const float a01 = static_cast<float>(alpha[y1 * ALPHA_MAP_DIM + x0]);
    const float a11 = static_cast<float>(alpha[y1 * ALPHA_MAP_DIM + x1]);

    const float top = a00 + (a10 - a00) * tx;
    const float bottom = a01 + (a11 - a01) * tx;
    return (top + (bottom - top) * ty) / 255.0f;
}

std::vector<uint8_t> featherAlphaEdges(const std::vector<uint8_t>& alpha) {
    if (alpha.size() < ALPHA_MAP_SIZE) return alpha;
    std::vector<uint8_t> out(alpha.begin(), alpha.begin() + ALPHA_MAP_SIZE);

    constexpr int kDim = static_cast<int>(ALPHA_MAP_DIM);
    const auto texel = [&](int x, int y) {
        x = std::clamp(x, 0, kDim - 1);
        y = std::clamp(y, 0, kDim - 1);
        return static_cast<float>(alpha[static_cast<size_t>(y * kDim + x)]);
    };
    // GLSL's smoothstep, from one texel to eight: the band and the ramp the
    // shader had.
    const auto smoothstep = [](float edge0, float edge1, float x) {
        const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };
    constexpr float kInner = 1.0f / static_cast<float>(ALPHA_MAP_DIM);
    constexpr float kOuter = 8.0f / static_cast<float>(ALPHA_MAP_DIM);

    for (int y = 0; y < kDim; ++y) {
        for (int x = 0; x < kDim; ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(kDim);
            const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(kDim);
            const float border = std::min(std::min(u, 1.0f - u), std::min(v, 1.0f - v));
            const float weight = 1.0f - smoothstep(kInner, kOuter, border);
            if (weight < 0.001f) continue;
            // Four bilinear taps half a texel off, as the shader took them,
            // are this at a texel's centre: 1-2-1 each way.
            float tent = 0.0f;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    tent += static_cast<float>((2 - std::abs(dx)) * (2 - std::abs(dy))) *
                            texel(x + dx, y + dy);
                }
            }
            tent /= 16.0f;
            const float center = texel(x, y);
            const float mixed = center + (tent - center) * weight;
            out[static_cast<size_t>(y * kDim + x)] =
                static_cast<uint8_t>(std::clamp(std::lround(mixed), 0L, 255L));
        }
    }
    return out;
}

} // namespace pipeline
} // namespace wowee
