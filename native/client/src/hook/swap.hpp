#pragma once

namespace lumin::hook {

// Hooks opengl32!wglSwapBuffers. Retries internally until OpenGL is loaded, so
// it is safe to call as soon as the overlay thread starts.
bool install();

void uninstall();

} // namespace lumin::hook
