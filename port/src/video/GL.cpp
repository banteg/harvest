#include "video/GL.h"
#include "daisy/os.h"
#include <SDL3/SDL_video.h>
#include <string.h>

namespace port {
namespace gl {

#define PORT_GL_DEFINE(ret, name, params) ret(PORT_GL_APIENTRY* name) params = 0;
PORT_GL_FUNCTIONS(PORT_GL_DEFINE)
#undef PORT_GL_DEFINE

bool load()
{
    bool complete = true;

#define PORT_GL_LOAD(ret, name, params) \
    name = (ret(PORT_GL_APIENTRY*) params)SDL_GL_GetProcAddress(#name); \
    if (!name) \
    { \
        daisy::os::Printer::log("OpenGL entry point missing", #name, ox::event::ELL_ERROR); \
        complete = false; \
    }
    PORT_GL_FUNCTIONS(PORT_GL_LOAD)
#undef PORT_GL_LOAD

    return complete;
}

bool isES()
{
    const char* version = (const char*)glGetString(GL_VERSION);
    return version && strncmp(version, "OpenGL ES", 9) == 0;
}

} // end namespace gl
} // end namespace port
