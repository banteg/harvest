// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "CScatterShader.h"
#include "ox/core/CMatrix4.h"
#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/video/IGPUProgrammingServices.h"
#include "ox/video/IMaterialRendererServices.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace gfx {

//! The planet and atmosphere radii of the scattering model, in planet node units.
static const float INNER_RADIUS = 10.0f;
static const float OUTER_RADIUS = 10.25f;
//! Altitude, as a fraction of the atmosphere thickness, at which the average density is found.
static const float RAYLEIGH_SCALE_DEPTH = 0.25f;

//! The transformation slots of IVideoDriver::getTransform; Irrlicht's E_TRANSFORMATION_STATE order.
static const ox::video::E_TRANSFORMATION_STATE TRANSFORM_VIEW = (ox::video::E_TRANSFORMATION_STATE)0;
static const ox::video::E_TRANSFORMATION_STATE TRANSFORM_WORLD = (ox::video::E_TRANSFORMATION_STATE)1;
static const ox::video::E_TRANSFORMATION_STATE TRANSFORM_PROJECTION = (ox::video::E_TRANSFORMATION_STATE)2;

CScatterShader::CScatterShader(ox::video::IVideoDriver* driver, ox::scene::ICameraSceneNode* camera,
    ox::scene::IAnimatedMeshSceneNode* node)
    : Driver(driver), Camera(camera), Node(node)
{
    Type = ESST_NONE;
    Scattering = true;
}

CScatterShader::~CScatterShader()
{
}

void CScatterShader::initAtmo()
{
    int material = ((ox::video::IGPUProgrammingServices*)Driver->getGPUProgrammingServices())->addCgShaderMaterialFromFiles(
        "$GAME_RESOURCES$/harvestClientData/gfx/shaders/scatterAtmoCG.vsh", "main",
        "$GAME_RESOURCES$/harvestClientData/gfx/shaders/scatterAtmoCG.psh", "main", this,
        ox::video::EMT_TRANSPARENT_ADD_COLOR, false, 0);
    if (material != -1)
    {
        Node->getMaterial(0).MaterialType = (ox::video::E_MATERIAL_TYPE)material;
        Node->getMaterial(0).BilinearFilter = true;
        Node->setVisible(true);
        Type = ESST_ATMOSPHERE;
    }
}

void CScatterShader::initGround(bool scattering)
{
    Scattering = scattering;
    int material;
    if (scattering)
        material = ((ox::video::IGPUProgrammingServices*)Driver->getGPUProgrammingServices())->addCgShaderMaterialFromFiles(
            "$GAME_RESOURCES$/harvestClientData/gfx/shaders/scatterGroundCG.vsh", "main",
            "$GAME_RESOURCES$/harvestClientData/gfx/shaders/scatterGroundCG.psh", "main", this,
            ox::video::EMT_SOLID_2_LAYER, false, 0);
    else
        material = ((ox::video::IGPUProgrammingServices*)Driver->getGPUProgrammingServices())->addCgShaderMaterialFromFiles(
            "$GAME_RESOURCES$/harvestClientData/gfx/shaders/scatterGroundSCG.vsh", "main",
            "$GAME_RESOURCES$/harvestClientData/gfx/shaders/scatterGroundSCG.psh", "main", this,
            ox::video::EMT_SOLID_2_LAYER, false, 0);
    if (material != -1)
    {
        Node->getMaterial(0).MaterialType = (ox::video::E_MATERIAL_TYPE)material;
        Type = ESST_GROUND;
    }
}

void CScatterShader::OnSetConstants(ox::video::IMaterialRendererServices* services, int userData)
{
    ox::core::CVector3d<float> cameraPos = Camera->getAbsolutePosition() - Node->getAbsolutePosition();
    ox::core::CVector3d<float> lightPos = (-Node->getAbsolutePosition()).normalize();
    // 1 / wavelength^4 for red, green and blue light of 0.650, 0.570 and 0.475 micrometres.
    float invWavelength[3] = { 5.60204601f, 9.47328377f, 19.6438046f };
    // Computed but unused; GCC keeps its sqrt for errno.
    float cameraHeight = cameraPos.getLength();
    float cameraHeight2 = cameraPos.getLengthSQ();
    float innerRadius = INNER_RADIUS;
    float outerRadius = OUTER_RADIUS;
    float outerRadius2 = OUTER_RADIUS * OUTER_RADIUS;
    float scale = 1.0f / (OUTER_RADIUS - INNER_RADIUS);
    float scaleOverScaleDepth = scale / RAYLEIGH_SCALE_DEPTH;

    ox::core::CMatrix4 translation;
    translation.makeIdentity();
    translation.setTranslation(-Node->getAbsolutePosition());
    ox::core::CMatrix4 matRot = translation * Node->getAbsoluteTransformation();

    ox::core::CMatrix4 matViewProjection;
    matViewProjection = Driver->getTransform(TRANSFORM_PROJECTION);
    matViewProjection *= Driver->getTransform(TRANSFORM_VIEW);
    matViewProjection *= Driver->getTransform(TRANSFORM_WORLD);

    ox::core::CMatrix4 matWorldInverseTranspose;
    if (Type == ESST_GROUND)
    {
        ox::core::CMatrix4 inverse;
        matRot.getInverse(inverse);
        matWorldInverseTranspose = inverse.getTransposed();
    }

    if (Scattering)
    {
        services->setVertexShaderConstant("v3CameraPos", &cameraPos.X, 3);
        services->setVertexShaderConstant("v3LightPos", &lightPos.X, 3);
        services->setVertexShaderConstant("v3InvWavelength", invWavelength, 3);
        services->setVertexShaderConstant("fCameraHeight2", &cameraHeight2, 1);
        services->setVertexShaderConstant("fInnerRadius", &innerRadius, 1);
        services->setVertexShaderConstant("fOuterRadius", &outerRadius, 1);
        services->setVertexShaderConstant("fOuterRadius2", &outerRadius2, 1);
        services->setVertexShaderConstant("fScale", &scale, 1);
        services->setVertexShaderConstant("fScaleOverScaleDepth", &scaleOverScaleDepth, 1);
    }
    services->setVertexShaderConstant("matRot", matRot.M, 16);
    services->setVertexShaderConstant("matViewProjection", matViewProjection.M, 16);
    if (Type == ESST_GROUND)
    {
        services->setVertexShaderConstant("matWorldInverseTranspose", matWorldInverseTranspose.M, 16);
        services->setPixelShaderConstant("v3CameraPos", &cameraPos.X, 3);
    }
    services->setPixelShaderConstant("v3LightPos", &lightPos.X, 3);
}

} // end namespace gfx
} // end namespace harvest
