// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CParticleState.h"
#include "CParticlePackage.h"
#include "ox/algo/CRand.h"
#include "ox/core/CMath.h"
#include "ox/video/IParticleEngineCallback.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include <math.h>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

CParticleState::CParticleState(ox::video::IParticlePackage* package, ox::video::ISpritePackage* spritePackage,
    SParticleTypeInfo* info)
    : IParticleState(package, spritePackage), Info(info), Animation(0), BounceCount(0), Age(0),
      Rotation(0), LastPosition(-1000.0f, -1000.0f), PulseTimer(0), PulseTimeLeft(0), PulseInterval(0),
      NextPulseParticle(0), PulseCount(0), SoundPlayed(false)
{
    if (!SpritePackage || !Info)
        return;

    if (info->AnimationNames.size() > 1)
        Animation = SpritePackage->addNewAnimationState(
            Info->AnimationNames[ox::algo::CRand::rand() % Info->AnimationNames.size()]);
    else if (!info->AnimationNames.empty())
        Animation = spritePackage->addNewAnimationState(info->AnimationNames[0]);

    Color = Info->StartColor;
    Scale = Info->StartScale;

    if (Info->MaxSpeed > 0)
    {
        int speed = Info->MinSpeed;
        if (Info->MaxSpeed > speed)
            speed += ox::algo::CRand::rand() % (Info->MaxSpeed - Info->MinSpeed);

        // The original takes the range the wrong way round, so the modulo goes negative.
        int direction = Info->MinDirection;
        if (Info->MaxDirection > direction)
            direction += ox::algo::CRand::rand() % (Info->MinDirection - Info->MaxDirection);

        int elevation = Info->MinElevation;
        if (Info->MaxElevation > elevation)
            elevation += ox::algo::CRand::rand() % (Info->MinElevation - Info->MaxElevation);

        Speed.set((float)speed, 0, 0);
        Speed.rotateXZBy(elevation);
        Speed.rotateXYBy(direction);
    }

    LifeTime = (float)Info->MinLifeTime;
    if (Info->MaxLifeTime > 0 && Info->MaxLifeTime > Info->MinLifeTime)
        LifeTime += ox::algo::CRand::rand() % (Info->MaxLifeTime - Info->MinLifeTime);
    LifeTime *= 0.001f;

    MoveX.Function = &Info->MoveXFunction;
    MoveY.Function = &Info->MoveYFunction;
    MoveZ.Function = &Info->MoveZFunction;
    SpeedChange.Function = &Info->SpeedFunction;
}

CParticleState::~CParticleState()
{
}

void CParticleState::remove()
{
    if (Animation)
        Animation->remove();
    if (Package)
        Package->removeParticleState(this);
}

const char* CParticleState::getParticleName()
{
    if (Info)
        return Info->Name.c_str();
    return "";
}

const char* CParticleState::getAnimationName()
{
    if (Info && !Info->AnimationNames.empty())
        return Info->AnimationNames[0].c_str();
    return "";
}

const ox::video::SColor& CParticleState::getCurrentColor()
{
    return Color;
}

float CParticleState::getCurrentScale()
{
    return Scale;
}

bool CParticleState::update(float frameDelta, ox::core::CVector3d<float>& position)
{
    if (!Info)
        return false;

    while (frameDelta > 0.0f)
    {
        if (!SoundPlayed && Package && Package->getCallbackEngine())
        {
            if (!Info->SoundNames.empty())
                Package->getCallbackEngine()->playParticleSound(
                    Info->SoundNames[ox::algo::CRand::rand() % Info->SoundNames.size()].c_str(), position);
            SoundPlayed = true;
        }

        Age += frameDelta;
        if (LifeTime > 0.0f)
        {
            if (Age >= LifeTime)
            {
                createOnDieParticle(position);
                return false;
            }

            float t = Age / LifeTime;
            Scale = (Info->EndScale - Info->StartScale) * t + Info->StartScale;
            const ox::video::SColor& start = Info->StartColor;
            const ox::video::SColor& end = Info->EndColor;
            Color = ox::video::SColor(
                (int)(start.getAlpha() + (end.getAlpha() - start.getAlpha()) * t),
                (int)(start.getRed() + (end.getRed() - start.getRed()) * t),
                (int)(start.getGreen() + (end.getGreen() - start.getGreen()) * t),
                (int)(start.getBlue() + (end.getBlue() - start.getBlue()) * t));
        }

        if (Animation && Animation->update(frameDelta) && Info->DieWithAnimation)
        {
            createOnDieParticle(position);
            return false;
        }

        ox::core::CVector3d<float> direction = Speed;
        if (Info->NormalizeSpeed)
            direction.normalize();

        updateParticleFunction(SpeedChange, frameDelta);
        Speed += direction * (SpeedChange.Value * frameDelta);

        updateParticleFunction(MoveX, frameDelta);
        updateParticleFunction(MoveY, frameDelta);
        updateParticleFunction(MoveZ, frameDelta);
        Speed.X += MoveX.Value * frameDelta;
        Speed.Y += MoveY.Value * frameDelta;
        Speed.Z += MoveZ.Value * frameDelta;

        if (Info->RotationSpeed > 0.0f && !Info->RotateToDirection)
            Rotation += Info->RotationSpeed * frameDelta * 0.0174532905f;

        float step = frameDelta;
        frameDelta -= step;
        position += Speed * step;

        PulseTimer -= step;
        if (PulseTimer <= 0.0f && PulseTimeLeft <= 0.0f)
        {
            if (!Info->PulseParticles.empty())
            {
                if (Info->MaxPulses > 0 && PulseCount >= Info->MaxPulses)
                {
                    createOnDieParticle(position);
                    return false;
                }

                if (PulseCount > 0 || !Info->SkipFirstPulse)
                {
                    int first = Info->MinPulseParticles;
                    int count = Info->MaxPulseParticles;
                    if (first > 0)
                    {
                        createPulseParticle(position);
                        --count;
                        --first;
                    }

                    if (count > 0)
                    {
                        int particles = first;
                        if (first < count)
                            particles = ox::algo::CRand::rand() % (count - first) + first;

                        if (particles > 0)
                        {
                            if (Info->MaxPulseDuration > 0 && Info->MaxPulseDuration > Info->MinPulseDuration)
                            {
                                int duration = ox::algo::CRand::rand() %
                                    (Info->MaxPulseDuration - Info->MinPulseDuration) + Info->MinPulseDuration;
                                PulseTimeLeft = duration * 0.001f;
                                PulseInterval = NextPulseParticle = duration / particles * 0.001f;
                            }
                            else
                            {
                                for (int i = 0; i < particles; ++i)
                                    createPulseParticle(position);
                            }
                        }
                    }
                }

                int delay = Info->MinPulseDelay;
                int extra = 0;
                if (delay > 0 && delay < Info->MaxPulseDelay)
                    extra = ox::algo::CRand::rand() % (Info->MaxPulseDelay - delay);
                PulseTimer += (delay + extra) * 0.001f;
            }
            else
                PulseTimer += 1000000.0f;
            ++PulseCount;
        }
        else if (PulseTimeLeft > 0.0f)
        {
            PulseTimeLeft -= step;
            NextPulseParticle -= step;
            if (NextPulseParticle <= 0.0f)
            {
                NextPulseParticle += PulseInterval;
                createPulseParticle(position);
            }
        }
    }
    return true;
}

void CParticleState::createOnDieParticle(ox::core::CVector3d<float>& position)
{
    if (!Package || !Package->getCallbackEngine() || Info->OnDieParticles.empty())
        return;

    ox::video::IParticleEngineCallback* callback = Package->getCallbackEngine();
    const char* marker = callback->getOnDieMarkerAtPos(position);

    // Without a marker the names before the first '@' name are used, with one the names after the
    // matching '@' name up to the next one.
    bool active = marker == 0;
    bool hasMarkers = false;
    int created = 0;
    for (unsigned int i = 0; i < Info->OnDieParticles.size(); ++i)
    {
        const ox::core::CString<char>& name = Info->OnDieParticles[i];
        if (name.size() > 0 && name[0] == '@')
        {
            hasMarkers = true;
            if (active)
                break;
            if (name.equals_ignore_case(ox::core::CString<char>(marker)))
                active = true;
            continue;
        }

        if (active)
        {
            if (Package->spriteFulfillsImportance(name))
            {
                CParticleState* state = (CParticleState*)Package->addNewParticleState(name);
                state->applyParentModifiers(this);
                callback->addParticleEntity(state, position);
            }
            ++created;
        }
    }

    if (created == 0 && !hasMarkers)
    {
        for (unsigned int i = 0; i < Info->OnDieParticles.size(); ++i)
        {
            const ox::core::CString<char>& name = Info->OnDieParticles[i];
            if (Package->spriteFulfillsImportance(name))
            {
                CParticleState* state = (CParticleState*)Package->addNewParticleState(name);
                state->applyParentModifiers(this);
                callback->addParticleEntity(state, position);
            }
        }
    }
}

void CParticleState::updateParticleFunction(SParticleFunctionInstance& instance, float frameDelta)
{
    const SParticleFunction* function = instance.Function;
    switch (function->Type)
    {
    case EPFT_CONSTANT:
        instance.Value = 1.0f;
        break;
    case EPFT_LINEAR:
        instance.Time += frameDelta;
        instance.Value = instance.Time * function->Frequency + function->Phase;
        break;
    case EPFT_SINE:
        instance.Time += frameDelta;
        instance.Value = sinf(instance.Time * function->Frequency + function->Phase);
        break;
    case EPFT_TRIANGLE:
    {
        instance.Time += frameDelta;
        float x = instance.Time * function->Frequency + function->Phase;
        x -= (float)((int)x & ~1);
        if (x < 0.5f)
            instance.Value = x * 2;
        else if (x < 1.5f)
            instance.Value = (1.0f - x) * 2;
        else
            instance.Value = (x - 2.0f) * 2;
        break;
    }
    case EPFT_SQUARE:
    {
        instance.Time += frameDelta;
        float x = instance.Time * function->Frequency + function->Phase;
        if (x - (float)(int)x < 0.5f)
            instance.Value = 1.0f;
        else
            instance.Value = -1.0f;
        break;
    }
    case EPFT_STEP:
    {
        float previous = instance.Time * function->Frequency + function->Phase;
        instance.Time += frameDelta;
        int current = (int)(instance.Time * function->Frequency + function->Phase);
        if (current != (int)previous || previous == 0.0f)
        {
            if (current & 1)
                instance.Value = -1.0f / frameDelta;
            else
                instance.Value = 1.0f / frameDelta;
        }
        else
            instance.Value = 0.0f;
        break;
    }
    case EPFT_EXP:
        instance.Time += frameDelta;
        instance.Value = (float)exp(instance.Time * function->Frequency + function->Phase);
        break;
    case EPFT_INVERSE_EXP:
        instance.Time += frameDelta;
        instance.Value = (float)(1.0 / exp(instance.Time * function->Frequency + function->Phase));
        break;
    }

    switch (instance.Function->Clamp)
    {
    case EPFC_ABSOLUTE:
        if (instance.Value < 0.0f)
            instance.Value = -instance.Value;
        break;
    case EPFC_POSITIVE:
        if (instance.Value < 0.0f)
            instance.Value = 0.0f;
        break;
    }
    instance.Value *= instance.Function->Amplitude;
}

void CParticleState::createPulseParticle(ox::core::CVector3d<float> position)
{
    if (!Package || !Package->getCallbackEngine() || Info->PulseParticles.empty())
        return;

    int index = ox::algo::CRand::rand() % (int)Info->PulseParticles.size();
    if (!Package->spriteFulfillsImportance(Info->PulseParticles[index]))
        return;

    CParticleState* state = (CParticleState*)Package->addNewParticleState(Info->PulseParticles[index]);
    position += Info->MinPulseOffset;
    if (Info->MaxPulseOffset.X > Info->MinPulseOffset.X)
        position.X += ox::algo::CRand::rand() % 1000 * 0.001f * (Info->MaxPulseOffset.X - Info->MinPulseOffset.X);
    if (Info->MaxPulseOffset.Y > Info->MinPulseOffset.Y)
        position.Y += ox::algo::CRand::rand() % 1000 * 0.001f * (Info->MaxPulseOffset.Y - Info->MinPulseOffset.Y);
    if (Info->MaxPulseOffset.Z > Info->MinPulseOffset.Z)
        position.Z += ox::algo::CRand::rand() % 1000 * 0.001f * (Info->MaxPulseOffset.Z - Info->MinPulseOffset.Z);

    state->applyParentModifiers(this);
    Package->getCallbackEngine()->addParticleEntity(state, position);
}

void CParticleState::addToSpeed(const ox::core::CVector3d<float>& speed)
{
    Speed += speed;
}

void CParticleState::setCurrentSpeed(const ox::core::CVector3d<float>& speed)
{
    Speed = speed;
}

void CParticleState::applyParentModifiers(CParticleState* parent)
{
    if (Info && Info->ParentSpeedModifier != 0.0f)
    {
        ox::core::CVector3d<float> inherited = parent->Speed;
        inherited *= Info->ParentSpeedModifier;
        Speed += inherited;
    }
}

void CParticleState::render2D(const ox::core::CPosition2d<float>& position, float scale)
{
    if (Info->RotateToDirection)
    {
        if (LastPosition.X > -1000.0f)
        {
            Rotation = ox::core::CMath::getAngleIY(LastPosition, position);
            LastPosition = position;
        }
        else
        {
            LastPosition = position;
            return;
        }
    }

    if (!Animation)
        return;

    if (!Info->MirrorByDirection)
        Animation->drawRotated(position, Rotation, scale * Scale, Color);
    else if (Speed.X < 0.0f)
        Animation->drawMirrored(position, scale * Scale, Color);
    else
        Animation->drawRotated(position, 0.0f, scale * Scale, Color);
}

void CParticleState::render3D(const ox::core::CVector3d<float>& position)
{
    if (Animation)
        Animation->draw3d(position, 0.0f, Scale, false, Color);
}

void CParticleState::render2DShadow(const ox::core::CPosition2d<float>& position, float scale, float alpha)
{
    if (Animation && Info->CastsShadow)
        Animation->drawRotated(position, Rotation, Scale * scale,
            ox::video::SColor((int)(Color.getAlpha() * alpha), 0, 0, 0));
}

const ox::core::CVector3d<float>& CParticleState::getCurrentSpeed()
{
    return Speed;
}

bool CParticleState::isGroundSprite()
{
    if (Info)
        return Info->GroundSprite;
    return false;
}

int CParticleState::getParticleImportance() const
{
    if (Info)
        return Info->Importance;
    return 0;
}

float CParticleState::getWindModifier() const
{
    if (Info)
        return Info->WindModifier;
    return 1.0f;
}

bool CParticleState::notifyBounce(ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& normal)
{
    float along = normal.dotProduct(Speed);
    Speed = (Speed - normal * along) * 2.0f - Speed;
    Speed *= Info->Bounciness;

    ++BounceCount;
    if (BounceCount > Info->MaxBounces && Info->MaxBounces >= 0)
    {
        createOnDieParticle(position);
        return false;
    }
    return true;
}

} // end namespace video
} // end namespace daisy
