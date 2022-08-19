#ifndef CLOCK_WIDGET_DEBUGTEXTUREPROVIDER_H
#define CLOCK_WIDGET_DEBUGTEXTUREPROVIDER_H

#include "ITextureDataProvider.h"
#include <cstdint>

namespace clock_widget {

class DebugTextureProvider : public dse::scn::ITextureDataProvider
{
public:
    static const float DefColor1[4];// = {1.f, 0.f, 1.f, 1.f};
    static const float DefColor2[4];// = {.0f, .0f, .0f, 1.f};
    DebugTextureProvider(int width, int height, const float color1[4] = DefColor1, const float color2[4] = DefColor2);

    // ITextureDataProvider interface
public:
    virtual void LoadParameters(TextureParameters* parameters) override;
    virtual void LoadData(void* recvBuffer, unsigned lod) override;
    virtual unsigned GetVersion() override;
private:
    int width, height;
    std::uint32_t colors[2];
};

} // namespace clock_widget

#endif // CLOCK_WIDGET_DEBUGTEXTUREPROVIDER_H
