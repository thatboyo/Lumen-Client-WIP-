#include "logo.hpp"

#include "core/log.hpp"
#include "logo_data.hpp"

#include <GL/gl.h>

#include <vector>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace lumin::logo {
namespace {

ImTextureID g_texture = 0;
bool g_attempted = false;

// Expands the run-length data into a straight-alpha RGBA buffer. Kept apart from
// the upload so a failed GL call cannot leave the texture half written.
std::vector<unsigned char> expand() {
    const size_t bytes = static_cast<size_t>(kWidth) * static_cast<size_t>(kHeight) * 4;
    std::vector<unsigned char> pixels(bytes, 0);

    size_t offset = 0;
    for (int run = 0; run < kRunCount && offset < bytes; ++run) {
        const unsigned int packed = kRuns[run][1];
        unsigned int length = kRuns[run][0];
        while (length-- > 0 && offset + 4 <= bytes) {
            pixels[offset + 0] = static_cast<unsigned char>(packed & 0xFF);
            pixels[offset + 1] = static_cast<unsigned char>((packed >> 8) & 0xFF);
            pixels[offset + 2] = static_cast<unsigned char>((packed >> 16) & 0xFF);
            pixels[offset + 3] = static_cast<unsigned char>((packed >> 24) & 0xFF);
            offset += 4;
        }
    }

    if (offset != bytes) {
        VLOG_WARN("logo data ended early (%zu of %zu bytes)", offset, bytes);
    }
    return pixels;
}

ImTextureID upload() {
    const std::vector<unsigned char> pixels = expand();

    GLuint id = 0;
    glGenTextures(1, &id);
    if (id == 0) {
        VLOG_WARN("glGenTextures failed for the logo");
        return 0;
    }

    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);

    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kWidth, kHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels.data());

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
    return static_cast<ImTextureID>(id);
}

} // namespace

ImTextureID texture() {
    if (g_attempted) return g_texture;

    // One attempt is enough: a second try without a context would fail the same
    // way, and the menu simply draws without the mark.
    g_attempted = true;
    g_texture = upload();
    if (g_texture) {
        VLOG_INFO("logo uploaded (%dx%d)", kWidth, kHeight);
    }
    return g_texture;
}

bool ready() {
    return texture() != 0;
}

float widthForHeight(float height) {
    return height * static_cast<float>(kWidth) / static_cast<float>(kHeight);
}

void draw(ImDrawList* draw, const ImVec2& pos, float height, ImU32 tint) {
    const ImTextureID tex = texture();
    if (!tex) return;

    const float width = widthForHeight(height);
    draw->AddImage(tex, pos, ImVec2(pos.x + width, pos.y + height), ImVec2(0, 0), ImVec2(1, 1),
                   tint);
}

} // namespace lumin::logo
