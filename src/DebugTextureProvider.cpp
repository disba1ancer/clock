#include "DebugTextureProvider.h"

namespace {
    std::uint32_t cFToI(const float color[4]) {
        return std::uint32_t(color[0] * 255.f) +
            (std::uint32_t(color[1] * 255.f) << 8) +
            (std::uint32_t(color[2] * 255.f) << 16) +
            (std::uint32_t(color[3] * 255.f) << 24);
    }
}

namespace clock_widget {

const float DebugTextureProvider::DefColor1[4] = {1.f, 0.f, 1.f, 1.f};
const float DebugTextureProvider::DefColor2[4] = {.0f, .0f, .0f, 1.f};

DebugTextureProvider::DebugTextureProvider(
    int width, int height, const float color1[4], const float color2[4]
) :
    width(width),
    height(height),
    colors{cFToI(color1), cFToI(color2)}
{}

void DebugTextureProvider::LoadParameters(TextureParameters* parameters)
{
    using TDP = ITextureDataProvider;
    parameters->width = width;
    parameters->height = height;
    parameters->depth = 1;
    parameters->format = TDP::RGBA8sRGB;
    parameters->lodCount = 1;
}

void DebugTextureProvider::LoadData(void* recvBuffer, unsigned lod)
{
    auto buf = static_cast<std::uint32_t*>(recvBuffer);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x, ++buf) {
            *buf = colors[((y ^ x) >> 3) & 1];
        }
    }
}

unsigned DebugTextureProvider::GetVersion()
{
    return 1;
}

} // namespace clock_widget
