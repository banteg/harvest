// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CStaticMeshOBJ.cpp and fast_atof.h (license:
// third_party/irrlicht-0.7/include/irrlicht.h).
//
// daisy's loader (Mac 0x11a434, Linux 0x57d1b0) is Irrlicht 0.7's, and so is this one, with the
// conventions docs/port/menu-scene.md lists: positions and normals copied as they are (no axis
// flip), V negated, white vertices, every face corner its own vertex, faces fanned as (0, 1, n-1)
// then (1, n-2-k, n-1-k). Numbers go through Irrlicht's fast_atof, which rounds differently from
// strtod. Unlike the original, the file buffer is zero-terminated and face tokens are not copied
// into fixed-size buffers.

#include "scene/CStaticMeshOBJ.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "daisy/os.h"
#include "ox/event/ILogger.h"
#include "ox/io/IReadFile.h"

namespace daisy {
namespace scene {

namespace {

const float fast_atof_table[] = { 0.f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f, 0.0000001f, 0.00000001f,
    0.000000001f, 0.0000000001f, 0.00000000001f, 0.000000000001f, 0.0000000000001f, 0.00000000000001f,
    0.000000000000001f };

//! Irrlicht 0.7's fast_atof: integer part and fraction digits through strtol, scaled by the table.
float fast_atof(const char* c)
{
    bool inv = false;
    char* t;
    float f;

    if (*c == '-')
    {
        c++;
        inv = true;
    }

    f = (float)strtol(c, &t, 10);
    c = t;

    if (*c == '.')
    {
        c++;
        float pl = (float)strtol(c, &t, 10);
        pl *= fast_atof_table[t - c];
        f += pl;
        c = t;

        if (*c == 'e')
        {
            ++c;
            float exp = (float)strtol(c, &t, 10);
            f *= (float)pow(10.0f, exp);
        }
    }

    if (inv)
        f *= -1.0f;

    return f;
}

bool isWordEnd(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

//! Skips whitespace.
const char* getFirstWord(const char* buf)
{
    while (*buf && isWordEnd(*buf))
        ++buf;
    return buf;
}

//! The start of the word after this one, or null at the end of the text.
const char* getNextWord(const char* word)
{
    if (!word)
        return 0;

    int i = 0;
    while (word[i] && !isWordEnd(word[i]))
        ++i;

    const char* next = getFirstWord(&word[i]);
    return next == word ? 0 : next;
}

//! The text from p up to the first character in stops (or the end).
std::string copyUntil(const char* p, const char* stops)
{
    return std::string(p, strcspn(p, stops));
}

const int MAX_FACE_POINT_COUNT = 40;

} // end anonymous namespace

bool CStaticMeshOBJ::loadFile(ox::io::IReadFile* file)
{
    int filesize = file->getSize();
    if (!filesize)
        return false;

    std::string text(filesize, '\0');
    file->read(&text[0], filesize);
    const char* buf = text.c_str();

    SMeshBuffer* meshbuffer = new SMeshBuffer();
    std::vector<vector3df> vertexBuffer;
    std::vector<ox::core::CVector2d<float> > textureCoordBuffer;
    std::vector<vector3df> normalsBuffer;

    const char* line = buf;
    while (line)
    {
        std::string word = copyUntil(line, " \n\r\t");

        if (word == "v")
        {
            const char* p1 = getNextWord(line);
            const char* p2 = getNextWord(p1);
            const char* p3 = getNextWord(p2);
            vertexBuffer.push_back(vector3df(fast_atof(p1), fast_atof(p2), fast_atof(p3)));
        }
        else if (word.size() >= 2 && word[0] == 'v' && word[1] == 't')
        {
            const char* p1 = getNextWord(line);
            const char* p2 = getNextWord(p1);
            textureCoordBuffer.push_back(ox::core::CVector2d<float>(fast_atof(p1), fast_atof(p2)));
        }
        else if (word.size() >= 2 && word[0] == 'v' && word[1] == 'n')
        {
            const char* p1 = getNextWord(line);
            const char* p2 = getNextWord(p1);
            const char* p3 = getNextWord(p2);
            normalsBuffer.push_back(vector3df(fast_atof(p1), fast_atof(p2), fast_atof(p3)));
        }
        else if (word == "f")
        {
            // The rest of the line; each corner is v, v/vt or v/vt/vn with 1-based indices, and
            // a missing or out of range index gives zeros.
            std::string faceLine = copyUntil(line, "\n");

            int facePoints[MAX_FACE_POINT_COUNT][3];
            for (int k = 0; k < MAX_FACE_POINT_COUNT; ++k)
                facePoints[k][0] = facePoints[k][1] = facePoints[k][2] = -1;
            int facePointCount = 0;

            const char* face = getNextWord(faceLine.c_str());
            while (face && face[0])
            {
                std::string faceBuf = copyUntil(face, " ");
                int len = (int)faceBuf.size();
                int idx = 0;
                for (int z = 0; idx < len; ++z)
                {
                    if (z < 3)
                        facePoints[facePointCount][z] = atoi(&faceBuf[idx]);
                    for (++idx; idx < len && faceBuf[idx] && faceBuf[idx - 1] != '/'; ++idx)
                        ;
                }

                ++facePointCount;
                if (facePointCount >= MAX_FACE_POINT_COUNT)
                {
                    meshbuffer->drop();
                    os::Printer::log("Face with more than 40 face points found s32 file. Not loading.",
                        file->getFileName(), ox::event::ELL_ERROR);
                    return false;
                }

                face = getNextWord(face);
            }

            int currentVertexCount = (int)meshbuffer->Vertices.size();
            for (int i = 0; i < facePointCount; ++i)
            {
                ox::video::S3DVertex v;
                v.Color = ox::video::SColor(255, 255, 255, 255);

                int n = facePoints[i][2] - 1;
                if (n >= 0 && n < (int)normalsBuffer.size())
                    v.Normal = normalsBuffer[n];
                else
                    v.Normal.set(0, 0, 0);

                int p = facePoints[i][0] - 1;
                if (p >= 0 && p < (int)vertexBuffer.size())
                    v.Pos = vertexBuffer[p];
                else
                    v.Pos.set(0, 0, 0);

                int t = facePoints[i][1] - 1;
                if (t >= 0 && t < (int)textureCoordBuffer.size())
                {
                    v.TCoords.X = textureCoordBuffer[t].X;
                    v.TCoords.Y = -textureCoordBuffer[t].Y;
                }
                else
                    v.TCoords.set(0, 0);

                meshbuffer->Vertices.push_back(v);
            }

            meshbuffer->Indices.push_back((unsigned short)(0 + currentVertexCount));
            meshbuffer->Indices.push_back((unsigned short)(1 + currentVertexCount));
            meshbuffer->Indices.push_back((unsigned short)((facePointCount - 1) + currentVertexCount));
            for (int jk = 0; jk < facePointCount - 3; ++jk)
            {
                meshbuffer->Indices.push_back((unsigned short)(1 + currentVertexCount));
                meshbuffer->Indices.push_back((unsigned short)((facePointCount - 2 - jk) + currentVertexCount));
                meshbuffer->Indices.push_back((unsigned short)((facePointCount - 1 - jk) + currentVertexCount));
            }
        }
        else if (!word.empty() && (word[0] == '#' || word[0] == 'u' || word[0] == 'g'))
        {
            // Comments, usemtl and groups: skip to the end of the line.
            line += strcspn(line, "\n\r");
        }

        line = getNextWord(line);
    }

    meshbuffer->recalculateBoundingBox();
    Mesh.addMeshBuffer(meshbuffer);
    Mesh.recalculateBoundingBox();
    meshbuffer->drop();

    return true;
}

int CStaticMeshOBJ::getFrameCount()
{
    return 1;
}

ox::scene::IMesh* CStaticMeshOBJ::getMesh(int frame, int detailLevel, int startFrameLoop, int endFrameLoop)
{
    return &Mesh;
}

const aabbox3df& CStaticMeshOBJ::getBoundingBox() const
{
    return Mesh.getBoundingBox();
}

} // end namespace scene
} // end namespace daisy
