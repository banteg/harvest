// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The subset of NVIDIA's Cg runtime (<Cg/cg.h> and <Cg/cgGL.h>, Cg 2.x) that daisy uses. The build
// container has no Cg toolkit, so the declarations are written out here with the toolkit's names,
// types and enumerator values.

#ifndef DAISY_VIDEO_CGAPI_H
#define DAISY_VIDEO_CGAPI_H

typedef struct _CGcontext* CGcontext;
typedef struct _CGprogram* CGprogram;
typedef struct _CGparameter* CGparameter;

typedef int CGbool;

enum CGenum
{
    CG_SOURCE = 4112,
    CG_OBJECT = 4113,
    CG_PROGRAM = 4125
};

enum CGprofile
{
    CG_PROFILE_UNKNOWN = 6145
};

enum CGerror
{
    CG_NO_ERROR = 0
};

enum CGGLenum
{
    CG_GL_VERTEX = 8,
    CG_GL_FRAGMENT = 9
};

extern "C" {

CGcontext cgCreateContext();
CGerror cgGetError();
const char* cgGetErrorString(CGerror error);

CGprogram cgCreateProgram(CGcontext context, CGenum programType, const char* program, CGprofile profile,
    const char* entry, const char** args);
void cgDestroyProgram(CGprogram program);
CGprofile cgGetProgramProfile(CGprogram program);

CGparameter cgGetNamedParameter(CGprogram program, const char* name);
CGparameter cgGetFirstParameter(CGprogram program, CGenum nameSpace);
CGparameter cgGetNextParameter(CGparameter current);
const char* cgGetParameterName(CGparameter parameter);
int cgGetParameterRows(CGparameter parameter);
int cgGetParameterColumns(CGparameter parameter);
void cgSetParameterValuefc(CGparameter parameter, int count, const float* values);

CGprofile cgGLGetLatestProfile(CGGLenum profileType);
void cgGLSetOptimalOptions(CGprofile profile);
void cgGLLoadProgram(CGprogram program);
void cgGLBindProgram(CGprogram program);
void cgGLUnbindProgram(CGprofile profile);
void cgGLEnableProfile(CGprofile profile);
void cgGLDisableProfile(CGprofile profile);

} // end extern "C"

#endif
