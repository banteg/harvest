// The OpenGL entry points the renderer uses: the common subset of OpenGL ES 3.0 (WebGL 2) and
// OpenGL 3.3 core. They are loaded at run time with SDL_GL_GetProcAddress from the context the
// device made current, so no system GL header or library is needed on any target.

#ifndef PORT_VIDEO_GL_H
#define PORT_VIDEO_GL_H

#include <stddef.h>

#if defined(_WIN32) && !defined(_WIN64)
#define PORT_GL_APIENTRY __stdcall
#else
#define PORT_GL_APIENTRY
#endif

namespace port {
namespace gl {

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef float GLfloat;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

const GLenum GL_FALSE = 0;
const GLenum GL_TRUE = 1;
const GLenum GL_NO_ERROR = 0;

// primitives
const GLenum GL_LINES = 0x0001;
const GLenum GL_TRIANGLES = 0x0004;
const GLenum GL_TRIANGLE_FAN = 0x0006;

// capabilities
const GLenum GL_CULL_FACE = 0x0B44;
const GLenum GL_DEPTH_TEST = 0x0B71;
const GLenum GL_BLEND = 0x0BE2;
const GLenum GL_SCISSOR_TEST = 0x0C11;

// face winding
const GLenum GL_CW = 0x0900;
const GLenum GL_CCW = 0x0901;

// blend factors
const GLenum GL_ZERO = 0;
const GLenum GL_ONE = 1;
const GLenum GL_SRC_COLOR = 0x0300;
const GLenum GL_ONE_MINUS_SRC_COLOR = 0x0301;
const GLenum GL_SRC_ALPHA = 0x0302;
const GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;

// clear bits
const GLbitfield GL_DEPTH_BUFFER_BIT = 0x00000100;
const GLbitfield GL_STENCIL_BUFFER_BIT = 0x00000400;
const GLbitfield GL_COLOR_BUFFER_BIT = 0x00004000;

// queries
const GLenum GL_VIEWPORT = 0x0BA2;
const GLenum GL_TEXTURE_BINDING_2D = 0x8069;
const GLenum GL_VENDOR = 0x1F00;
const GLenum GL_RENDERER = 0x1F01;
const GLenum GL_VERSION = 0x1F02;
const GLenum GL_SHADING_LANGUAGE_VERSION = 0x8B8C;

// pixel data
const GLenum GL_UNSIGNED_BYTE = 0x1401;
const GLenum GL_UNSIGNED_SHORT = 0x1403;
const GLenum GL_FLOAT = 0x1406;
const GLenum GL_RGBA = 0x1908;
const GLenum GL_RGBA8 = 0x8058;
const GLenum GL_PACK_ALIGNMENT = 0x0D05;
const GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;

// textures
const GLenum GL_TEXTURE_2D = 0x0DE1;
const GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
const GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
const GLenum GL_TEXTURE_WRAP_S = 0x2802;
const GLenum GL_TEXTURE_WRAP_T = 0x2803;
const GLenum GL_NEAREST = 0x2600;
const GLenum GL_LINEAR = 0x2601;
const GLenum GL_LINEAR_MIPMAP_NEAREST = 0x2701;
const GLenum GL_REPEAT = 0x2901;
const GLenum GL_MIRRORED_REPEAT = 0x8370;
const GLenum GL_TEXTURE0 = 0x84C0;

// buffers
const GLenum GL_ARRAY_BUFFER = 0x8892;
const GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
const GLenum GL_STREAM_DRAW = 0x88E0;
const GLenum GL_STATIC_DRAW = 0x88E4;

// shaders
const GLenum GL_FRAGMENT_SHADER = 0x8B30;
const GLenum GL_VERTEX_SHADER = 0x8B31;
const GLenum GL_COMPILE_STATUS = 0x8B81;
const GLenum GL_LINK_STATUS = 0x8B82;
const GLenum GL_INFO_LOG_LENGTH = 0x8B84;

//! Every entry point: X(return type, name, parameter list).
#define PORT_GL_FUNCTIONS(X) \
    X(void, glActiveTexture, (GLenum texture)) \
    X(void, glAttachShader, (GLuint program, GLuint shader)) \
    X(void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar* name)) \
    X(void, glBindBuffer, (GLenum target, GLuint buffer)) \
    X(void, glBindTexture, (GLenum target, GLuint texture)) \
    X(void, glBindVertexArray, (GLuint array)) \
    X(void, glBlendFunc, (GLenum sfactor, GLenum dfactor)) \
    X(void, glBufferData, (GLenum target, GLsizeiptr size, const void* data, GLenum usage)) \
    X(void, glClear, (GLbitfield mask)) \
    X(void, glClearColor, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    X(void, glCompileShader, (GLuint shader)) \
    X(GLuint, glCreateProgram, (void)) \
    X(GLuint, glCreateShader, (GLenum type)) \
    X(void, glDeleteBuffers, (GLsizei n, const GLuint* buffers)) \
    X(void, glDeleteProgram, (GLuint program)) \
    X(void, glDeleteShader, (GLuint shader)) \
    X(void, glDeleteTextures, (GLsizei n, const GLuint* textures)) \
    X(void, glDeleteVertexArrays, (GLsizei n, const GLuint* arrays)) \
    X(void, glDepthMask, (GLboolean flag)) \
    X(void, glDisable, (GLenum cap)) \
    X(void, glDisableVertexAttribArray, (GLuint index)) \
    X(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count)) \
    X(void, glDrawElements, (GLenum mode, GLsizei count, GLenum type, const void* indices)) \
    X(void, glEnable, (GLenum cap)) \
    X(void, glEnableVertexAttribArray, (GLuint index)) \
    X(void, glFrontFace, (GLenum mode)) \
    X(void, glGenBuffers, (GLsizei n, GLuint* buffers)) \
    X(void, glGenTextures, (GLsizei n, GLuint* textures)) \
    X(void, glGenVertexArrays, (GLsizei n, GLuint* arrays)) \
    X(void, glGenerateMipmap, (GLenum target)) \
    X(GLenum, glGetError, (void)) \
    X(void, glGetIntegerv, (GLenum pname, GLint* data)) \
    X(void, glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)) \
    X(void, glGetProgramiv, (GLuint program, GLenum pname, GLint* params)) \
    X(void, glGetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)) \
    X(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint* params)) \
    X(const GLubyte*, glGetString, (GLenum name)) \
    X(GLint, glGetUniformLocation, (GLuint program, const GLchar* name)) \
    X(void, glLinkProgram, (GLuint program)) \
    X(void, glPixelStorei, (GLenum pname, GLint param)) \
    X(void, glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, \
        void* pixels)) \
    X(void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    X(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)) \
    X(void, glTexImage2D, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, \
        GLint border, GLenum format, GLenum type, const void* pixels)) \
    X(void, glTexParameteri, (GLenum target, GLenum pname, GLint param)) \
    X(void, glUniform1f, (GLint location, GLfloat v0)) \
    X(void, glUniform1fv, (GLint location, GLsizei count, const GLfloat* value)) \
    X(void, glUniform1i, (GLint location, GLint v0)) \
    X(void, glUniform1iv, (GLint location, GLsizei count, const GLint* value)) \
    X(void, glUniform2fv, (GLint location, GLsizei count, const GLfloat* value)) \
    X(void, glUniform3fv, (GLint location, GLsizei count, const GLfloat* value)) \
    X(void, glUniform4fv, (GLint location, GLsizei count, const GLfloat* value)) \
    X(void, glUniformMatrix3fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
    X(void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
    X(void, glUseProgram, (GLuint program)) \
    X(void, glVertexAttrib4f, (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)) \
    X(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, \
        const void* pointer)) \
    X(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height))

#define PORT_GL_DECLARE(ret, name, params) extern ret(PORT_GL_APIENTRY* name) params;
PORT_GL_FUNCTIONS(PORT_GL_DECLARE)
#undef PORT_GL_DECLARE

//! Loads every entry point from the current context; false (with the missing name logged) if one
//! is missing.
bool load();

//! True when the current context is OpenGL ES (or WebGL), false for desktop OpenGL core.
bool isES();

} // end namespace gl
} // end namespace port

#endif
