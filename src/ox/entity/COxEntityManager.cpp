// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <algorithm>
#include "COxEntityManager.h"
#include "COxEntity.h"
#include "ITestEntityFunction.h"
#include "ITestBestEntityFunction.h"
#include "../algo/SPointerSortFunctor.h"

namespace ox {
namespace entity {

COxEntityManager::COxEntityManager(int layers)
    : NumLayers(layers), UpdateCounter(0), Updating(false)
{
    EntityLists = new std::list<COxEntity*>[layers];
    PendingEntities = new std::list<COxEntity*>[layers];
#ifdef HARVEST_PORT
    // cleared: updateReference reads the flags before the first update sets them (the original's
    // garbage only re-located the entity); UBSan traps on a bool that is neither 0 nor 1
    ListChanged = new bool[layers]();
#else
    ListChanged = new bool[layers];
#endif
}

COxEntityManager::~COxEntityManager()
{
    for (int layer = 0; layer < NumLayers; ++layer)
    {
        for (std::list<COxEntity*>::iterator it = EntityLists[layer].begin(); it != EntityLists[layer].end(); ++it)
            delete *it;
        for (std::list<COxEntity*>::iterator it = PendingEntities[layer].begin(); it != PendingEntities[layer].end(); ++it)
            delete *it;
    }
    delete[] EntityLists;
    delete[] PendingEntities;
    delete[] ListChanged;
}

void COxEntityManager::updateAllEntities(float frameDelta, const core::CRect<float>& visibleArea)
{
    Updating = true;
    RenderList.clear();
    ++UpdateCounter;
    for (int layer = 0; layer < NumLayers; ++layer)
    {
        bool changed = false;
        std::list<COxEntity*>::iterator it = EntityLists[layer].begin();
        while (it != EntityLists[layer].end())
        {
            if ((*it)->update(frameDelta) == 1 || (*it)->isKilled())
            {
                (*it)->notifyRemoved();
                delete *it;
                it = EntityLists[layer].erase(it);
                ListChanged[layer] = true;
                changed = true;
            }
            else
            {
                if ((*it)->addToRenderList(visibleArea)) RenderList.push_back(*it);
                ++it;
            }
        }
        ListChanged[layer] = changed;
        it = PendingEntities[layer].begin();
        for (; it != PendingEntities[layer].end(); ++it)
            EntityLists[layer].push_back(*it);
        // Native code neither resets nor advances the pending iterator in this second pass.
        for (; it != PendingEntities[layer].end(); )
            if ((*it)->update(.0001f) == 0 && (*it)->addToRenderList(visibleArea))
                RenderList.push_back(*it);
        PendingEntities[layer].clear();
    }
    std::sort(RenderList.begin(), RenderList.end(), algo::SPointerSortFunctor<COxEntity*>());
    Updating = false;
}

const TArray<COxEntity*>& COxEntityManager::getRenderList() { return RenderList; }

void COxEntityManager::appendEntity(COxEntity* entity, int layer)
{
    if (Updating) PendingEntities[layer].push_back(entity);
    else EntityLists[layer].push_back(entity);
}

const std::list<COxEntity*>& COxEntityManager::getEntityList(int layer) const
{
    return EntityLists[layer];
}

COxEntity* COxEntityManager::locateEntity(int id, int layer)
{
    for (std::list<COxEntity*>::iterator it = EntityLists[layer].begin(); it != EntityLists[layer].end(); ++it)
        if ((*it)->getId() == id) return (*it)->isKilled() ? 0 : *it;
    return 0;
}

COxEntity* COxEntityManager::findFirstEntity(int layer, ITestEntityFunction* test)
{
    for (std::list<COxEntity*>::iterator it = EntityLists[layer].begin(); it != EntityLists[layer].end(); ++it)
        if (test->testEntity(*it)) return *it;
    return 0;
}

COxEntity* COxEntityManager::findBestEntity(int layer, ITestBestEntityFunction* test)
{
    COxEntity* best = 0;
    for (std::list<COxEntity*>::iterator it = EntityLists[layer].begin(); it != EntityLists[layer].end(); ++it)
    {
        int result = test->testEntity(*it, best);
        if (result == 1) best = *it;
        else if (result == 2) return *it;
    }
    return best;
}

void COxEntityManager::findAllEntities(TArray<COxEntity*>& result, int layer, ITestEntityFunction* test)
{
    for (std::list<COxEntity*>::iterator it = EntityLists[layer].begin(); it != EntityLists[layer].end(); ++it)
        if (test->testEntity(*it)) result.push_back(*it);
}

void COxEntityManager::updateReference(SEntityReference& reference, int layer, bool locate)
{
    if (reference.Entity)
    {
        // Both native builds deliberately fault through this virtual call on a stale reference.
        if (reference.UpdateCounter && UpdateCounter > reference.UpdateCounter + 1)
            ((COxEntity*)0)->setPosition(3.0f, 4.0f, 5.0f);
        reference.UpdateCounter = UpdateCounter;
        if (ListChanged[layer])
        {
            reference.Entity = locateEntity(reference.Id, layer);
            if (!reference.Entity) reference.Id = -1;
        }
        else if (reference.Entity->isKilled())
        {
            reference.Entity = 0;
            reference.Id = -1;
        }
    }
    else if (locate)
    {
        reference.Entity = locateEntity(reference.Id, layer);
        reference.UpdateCounter = UpdateCounter;
    }
}

int COxEntityManager::getUpdateCounter() const { return UpdateCounter; }

} // end namespace entity
} // end namespace ox
