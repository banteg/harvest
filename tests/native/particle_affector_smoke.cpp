#include "daisy/scene/CParticleFadeOutAffector.h"
#include "ox/scene/IParticleAffector.h"
#include "ox/scene/SParticle.h"
#include <cstdio>
#include <cstring>
#include <typeinfo>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "Particle affector check failed: %s\n", #c); return 1; } } while (0)

int main()
{
    using ox::scene::SParticle;
    // The original emitter/particle-system interface accepts the ox base and particle type.
    ox::scene::IParticleAffector* affector = new daisy::scene::CParticleFadeOutAffector(
        ox::video::SColor(0xff000000), 100);
    CHECK(dynamic_cast<daisy::scene::CParticleFadeOutAffector*>(affector) != 0);
    CHECK(std::strcmp(typeid(ox::scene::IParticleAffector).name(), "N2ox5scene17IParticleAffectorE") == 0);
    CHECK(sizeof(SParticle) == 52);
    SParticle p;
    p.endTime = 1000;
    p.startColor = ox::video::SColor(0xff804020);
    p.color = ox::video::SColor(0x12345678);
    affector->affect(900, &p, 1);
    CHECK(p.color.color == 0x12345678); // The boundary itself does not rewrite the color.
    affector->affect(950, &p, 1);
    CHECK((unsigned int)p.color.color == 0xff402010);
    affector->affect(1000, &p, 1);
    CHECK((unsigned int)p.color.color == 0xff000000);
    p.color = ox::video::SColor(0x12345678);
    affector->affect(1001, &p, 1);
    CHECK(p.color.color == 0x12345678); // Preserve the original unsigned elapsed-time wrap.
    affector->affect(1000, 0, 0);
    CHECK(affector->drop());

    affector = new daisy::scene::CParticleFadeOutAffector(ox::video::SColor(0xff000000), 0);
    affector->affect(999, &p, 1);
    CHECK(p.color.color == 0x12345678); // Zero fade duration becomes one millisecond.
    affector->affect(1000, &p, 1);
    CHECK((unsigned int)p.color.color == 0xff000000);
    CHECK(affector->drop());
    std::puts("Particle affector smoke passed: ox interface, RTTI, fade boundaries and unsigned wrap");
    return 0;
}
