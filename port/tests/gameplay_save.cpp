// From port/: zig build test-gameplay_save. Uses only the tiny Lua bootstrap in tests/fixtures.
#include "daisy/video/Null/CSpritePackage.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/entity/CPerimeterBomb.h"
#include "harvest/entity/CSparkEntity.h"
#include "harvest/game/CLuaManager.h"
#include "ox/IOxDevice.h"
#include "ox/io/CMemReadFile.h"
#include "ox/io/CMemWriteFile.h"
#include "ox/io/IFileSystem.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

using namespace harvest;
using namespace ox::io;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "Gameplay save check failed: %s\n", #c); return false; } } while (0)

static bool roundTrip(ox::IOxDevice* device, bool saveValue)
{
    CMemWriteFile saved;
    {
        game::CLuaManager lua(device, 0);
        lua.initLuaByScriptList();
        CHECK(lua.getCompilerErrors().empty() && !lua.isRunningMods());
        if (saveValue)
        {
            lua_pushinteger(lua.L, 47);
            lua_setglobal(lua.L, "checkpoint");
        }
        CHECK(lua.writeLuaStates(&saved));
    }
    const int entityStart = saved.getPos();

    entity::CPerimeterBombExplosion bomb(520.0f, 415.0f);
    bomb.setFuse(0.75f);
    bomb.killEntity();
    bomb.writeEntity(&saved);
    entity::CSparkEntity spark(510.0f, 300.0f, -1, 123);
    spark.writeEntity(&saved);

    CMemReadFile input(saved.getData(), saved.getSize(), false);
    game::CLuaManager restored(device, 0);
    CHECK(restored.initLuaBySaveFile(&input, 30));
    CHECK(input.getPos() == entityStart);
    lua_getglobal(restored.L, "checkpoint");
    CHECK(saveValue ? lua_tointeger(restored.L, -1) == 47 : lua_isnil(restored.L, -1));
    lua_pop(restored.L, 1);

    CMemWriteFile reserialized;
    entity::CEntity* first = entity::CEntity::readNextEntity(&input, 30);
    CHECK(first && first->getEntityType() == 12 && first->getId() == bomb.getId() && first->isKilled());
    first->writeEntity(&reserialized);
    first->notifyRemoved();
    delete first;
    entity::CEntity* next = entity::CEntity::readNextEntity(&input, 30);
    CHECK(next && next->getEntityType() == 2 && next->getId() == spark.getId());
    next->writeEntity(&reserialized);
    delete next;

    CHECK(input.getRemainingSize() == 0);
    CHECK(reserialized.getSize() == saved.getSize() - entityStart);
    CHECK(std::memcmp(reserialized.getData(), saved.getData() + entityStart, reserialized.getSize()) == 0);
    bomb.notifyRemoved();
    return true;
}

int main()
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    ox::IOxDevice* device = ox::createDevice(ox::video::EDT_NULL, 0, L"0.7");
    device->getFileSystem()->addDirectoryAlias("$GAME_RESOURCES$", "tests/fixtures/gameplay_save");
    device->getFileSystem()->addDirectoryAlias("$HARVEST_USERDATA$", "tests/fixtures/gameplay_save/harvestClientData");
    entity::CEntityManager entities;
    entity::gp_entityManager = &entities;
    daisy::video::CSpritePackage sprites(0, false);
    entity::CEntity::gp_spritePackage = &sprites;

    bool passed = roundTrip(device, false) && roundTrip(device, true);
    device->drop();
    SDL_Quit();
    return passed ? 0 : 1;
}
