#pragma once

#include "core/bridge.hpp"

#include <imgui.h>
#include <cmath>

namespace lumin::project {

// Right-handed camera basis built from Minecraft's yaw/pitch convention:
// yaw 0 looks toward +Z, +yaw rotates toward -X, -pitch looks up.
struct Basis {
    double fx = 0, fy = 0, fz = 1;
    double rx = 1, ry = 0, rz = 0;
    double ux = 0, uy = 1, uz = 0;
};

inline Basis fromCamera(const GameState& s) {
    const double yaw = s.camYaw * (3.14159265358979323846 / 180.0);
    const double pitch = s.camPitch * (3.14159265358979323846 / 180.0);

    const double cp = std::cos(pitch);
    const double sp = std::sin(pitch);
    const double cy = std::cos(yaw);
    const double sy = std::sin(yaw);

    Basis b;
    b.fx = -sy * cp;
    b.fy = -sp;
    b.fz = cy * cp;

    b.rx = cy;
    b.ry = 0.0;
    b.rz = sy;

    // up = forward x right
    b.ux = b.fy * b.rz - b.fz * b.ry;
    b.uy = b.fz * b.rx - b.fx * b.rz;
    b.uz = b.fx * b.ry - b.fy * b.rx;
    return b;
}

// Projects a world point into viewport pixels. Returns false when the point is
// behind the camera or outside the viewport.
inline bool toScreen(const GameState& s, const Basis& b, double x, double y, double z,
                     const ImVec2& viewport, ImVec2& out) {
    if (viewport.x <= 1.0f || viewport.y <= 1.0f) return false;
    if (s.fov <= 1.0) return false;

    const double dx = x - s.camX;
    const double dy = y - s.camY;
    const double dz = z - s.camZ;

    const double cx = dx * b.rx + dy * b.ry + dz * b.rz;
    const double cy = dx * b.ux + dy * b.uy + dz * b.uz;
    const double cz = dx * b.fx + dy * b.fy + dz * b.fz;

    if (cz <= 0.05) return false;

    const double tanHalf = std::tan(s.fov * (3.14159265358979323846 / 180.0) * 0.5);
    const double aspect = static_cast<double>(viewport.x) / static_cast<double>(viewport.y);

    const double ndcX = cx / (cz * tanHalf * aspect);
    const double ndcY = cy / (cz * tanHalf);

    if (std::fabs(ndcX) > 2.0 || std::fabs(ndcY) > 2.0) return false;

    out.x = static_cast<float>((ndcX * 0.5 + 0.5) * viewport.x);
    out.y = static_cast<float>((0.5 - ndcY * 0.5) * viewport.y);
    return true;
}

} // namespace lumin::project
