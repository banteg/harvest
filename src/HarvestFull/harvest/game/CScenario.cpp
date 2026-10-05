// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Campaign layout, starting entities and scheduler are reconstructed; byte matching is partial.

#include <iostream>
#include "CWorld.h"
#include "CScenario.h"
#include "harvest/settings/CSystemConfig.h"
#include "harvest/entity/CDropshipEntity.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CMineralsEntity.h"

namespace harvest {
namespace game {

CScenario::CScenario() : Time(0), Dropship(0)
{
    Objectives[0] = settings::gp_systemConfig->getLocalizedText(L"scenario:objective1");
    Objectives[1] = settings::gp_systemConfig->getLocalizedText(L"scenario:objective2");
    for (int i = 0; i < 3; ++i) CharacterNames[i] = L"#2";
    CharacterNames[0].append(settings::gp_systemConfig->getLocalizedText(L"characters:info"));
    CharacterNames[1].append(settings::gp_systemConfig->getLocalizedText(L"characters:pilot"));
    CharacterNames[2].append(settings::gp_systemConfig->getLocalizedText(L"characters:training"));
}

CScenario::~CScenario() { clearEvents(); }

bool CHarvestEventCondition::comparisonTest(int value, int threshold, int comparison)
{
    switch (comparison)
    {
    case 0: return value == threshold;
    case 1: return value > threshold;
    case 2: return value < threshold;
    case 3: return value >= threshold;
    case 4: return value <= threshold;
    }
    return false;
}

void CScenario::clearEvents()
{
    // The native method deletes events but leaves the list nodes for their member destructors.
    for (std::list<CHarvestEvent*>::iterator it = Events.begin(); it != Events.end(); ++it) delete *it;
    for (std::list<CHarvestEvent*>::iterator it = ConditionalEvents.begin(); it != ConditionalEvents.end(); ++it)
        delete *it;
}

void CScenario::addEvent(CHarvestEvent* event) { Events.push_back(event); }

void CScenario::update(float frameDelta)
{
    if (!Events.empty()) Time += frameDelta;
    while (!Events.empty() && Time >= Events.front()->Delay)
    {
        Time = 0;
        Events.front()->runEvent();
        delete Events.front();
        Events.pop_front();
    }
    for (std::list<CHarvestEvent*>::iterator it = ConditionalEvents.begin(); it != ConditionalEvents.end();)
    {
        if (static_cast<CConditionalHarvestEvent*>(*it)->shouldRunEvent())
        {
            (*it)->runEvent();
            delete *it;
            it = ConditionalEvents.erase(it);
        }
        else ++it;
    }
}

void CScenario::skipToNextEvent()
{
    if (!Events.empty()) Time = Events.front()->Delay;
    else if (Dropship && Dropship->getDropshipState() == 1)
    {
        const std::list<ox::entity::COxEntity*>& aliens = entity::gp_entityManager->getEntityList(1);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = aliens.begin(); it != aliens.end(); ++it)
            (*it)->killEntity();
    }
}
int CScenario::getScenarioPlanet() const { return 0; }
int CScenario::getStartingCredits() const { return 0; }
int CScenario::getDoodadSeed() const { return 1234; }
const ox::core::CString<wchar_t>& CScenario::getObjective(int index) { return Objectives[index]; }

int CScenario::getNumObjectives() { return 2; }

void CScenario::applyInitialExpansions(CWorld* world)
{
    world->expandWorldFromCurrent(0, false);
    world->expandWorldFromCurrent(0, false);
    world->expandWorldFromCurrent(3, false);
    world->expandWorldFromCurrent(1, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(2, false);
}

void CScenario::addStartingEntities()
{
    StartingCollectorId = entity::gp_entityManager->addBuilding(0, 590.0f, 540.0f)->getId();
    entity::gp_entityManager->addBuilding(0, 380.0f, 599.0f);
    entity::gp_entityManager->addBuilding(0, 463.0f, 394.0f);
    entity::gp_entityManager->addBuilding(1, 480.0f, 480.0f);
    entity::gp_entityManager->addBuilding(1, 570.0f, 460.0f);
    entity::gp_entityManager->addBuilding(1, 698.0f, 510.0f);
    entity::gp_entityManager->addBuilding(1, 731.0f, 645.0f);
    entity::gp_entityManager->addBuilding(1, 629.0f, 737.0f);
    entity::CEntity* firstLink = entity::gp_entityManager->addBuilding(1, 357.0f, 714.0f);
    entity::gp_entityManager->addBuilding(1, 492.0f, 764.0f)->handleSelectionDraggedToEntity(firstLink);
    entity::gp_entityManager->addBuilding(1, 290.0f, 597.0f);
    entity::gp_entityManager->addBuilding(1, 346.0f, 475.0f);
    entity::gp_entityManager->addBuilding(7, 787.0f, 404.0f);
    entity::gp_entityManager->addBuilding(7, 581.0f, 326.0f);
    entity::gp_entityManager->addBuilding(8, 510.0f, 366.0f);
    entity::gp_entityManager->addBuilding(7, 382.0f, 334.0f);
    entity::gp_entityManager->addBuilding(7, 282.0f, 354.0f);
    entity::gp_entityManager->addBuilding(7, 162.0f, 628.0f);
    entity::gp_entityManager->addBuilding(7, 303.0f, 840.0f);
    entity::gp_entityManager->addBuilding(7, 517.0f, 896.0f);
    entity::gp_entityManager->addBuilding(7, 736.0f, 828.0f);
    entity::CEntity* linkTower = entity::gp_entityManager->addBuilding(7, 863.0f, 668.0f);
    entity::gp_entityManager->addBuilding(4, 807.0f, 546.0f);
    entity::gp_entityManager->addBuilding(4, 840.0f, 597.0f);
    entity::gp_entityManager->addBuilding(4, 412.0f, 770.0f);
    entity::gp_entityManager->addBuilding(4, 313.0f, 771.0f);
    entity::gp_entityManager->addBuilding(4, 265.0f, 483.0f);
    entity::gp_entityManager->addBuilding(1, 336.0f, 331.0f);
    entity::gp_entityManager->addBuilding(1, 358.0f, 196.0f);
    entity::gp_entityManager->addBuilding(1, 356.0f, 62.0f);
    entity::gp_entityManager->addBuilding(1, 417.0f, -61.0f);
    entity::gp_entityManager->addBuilding(1, 483.0f, -186.0f);
    entity::gp_entityManager->addBuilding(1, 446.0f, -317.0f);
    int firstDefenseId = entity::gp_entityManager->addBuilding(4, 506.0f, -319.0f)->getId();
    int secondDefenseId = entity::gp_entityManager->addBuilding(4, 466.0f, -404.0f)->getId();
    int thirdDefenseId = entity::gp_entityManager->addBuilding(4, 586.0f, -188.0f)->getId();
    int fourthDefenseId = entity::gp_entityManager->addBuilding(4, 406.0f, -226.0f)->getId();
    entity::gp_entityManager->addBuilding(7, 792.0f, 714.0f)->handleSelectionDraggedToEntity(linkTower);
    entity::gp_entityManager->addBuilding(7, 758.0f, 546.0f)->handleSelectionDraggedToEntity(linkTower);
    entity::gp_entityManager->addMinerals(1, 862.0f, 539.0f);
    entity::gp_entityManager->addMinerals(1, 1092.0f, 900.0f);
    entity::gp_entityManager->addMinerals(1, 1170.0f, 864.0f);
    entity::gp_entityManager->addMinerals(1, 1210.0f, 944.0f);
    entity::gp_entityManager->addMinerals(0, 1123.0f, 970.0f);
    entity::gp_entityManager->addMinerals(1, 1131.0f, 853.0f);
    entity::gp_entityManager->addMinerals(1, 1154.0f, 885.0f);
    entity::gp_entityManager->addMinerals(1, 722.0f, 1293.0f);
    entity::gp_entityManager->addMinerals(2, 593.0f, 1378.0f);
    entity::gp_entityManager->addMinerals(1, 128.0f, 1027.0f);
    entity::gp_entityManager->addMinerals(1, 45.0f, 1091.0f);
    entity::gp_entityManager->addMinerals(1, 155.0f, 1125.0f);
    TrackedMineralId = entity::gp_entityManager->addMinerals(0, -183.0f, 556.0f)->getId();
    entity::gp_entityManager->addMinerals(1, -145.0f, 515.0f);
    entity::gp_entityManager->addMinerals(1, -207.0f, 474.0f);
    entity::gp_entityManager->addMinerals(1, 12.0f, 122.0f);
    entity::gp_entityManager->addMinerals(2, 134.0f, 67.0f);
    entity::gp_entityManager->addMinerals(0, -27.0f, 75.0f);
    entity::gp_entityManager->addMinerals(1, -91.0f, 114.0f);
    entity::gp_entityManager->addMinerals(1, -337.0f, 949.0f);
    entity::gp_entityManager->addMinerals(1, -209.0f, 1438.0f);
    entity::gp_entityManager->addMinerals(0, -337.0f, 1353.0f);
    entity::gp_entityManager->addMinerals(1, -240.0f, 1323.0f);
    entity::gp_entityManager->addMinerals(1, 1337.0f, 553.0f);
    entity::gp_entityManager->addMinerals(1, 1405.0f, 338.0f);
    entity::gp_entityManager->addMinerals(0, 1109.0f, 163.0f);
    entity::gp_entityManager->addMinerals(1, 1178.0f, 153.0f);
    entity::gp_entityManager->addMinerals(1, 1118.0f, 105.0f);
    entity::gp_entityManager->addMinerals(1, 620.0f, 50.0f);
    entity::gp_entityManager->addMinerals(0, 686.0f, 88.0f);
    entity::gp_entityManager->addMinerals(2, 554.0f, 165.0f);
    entity::gp_entityManager->addMinerals(1, 681.0f, 134.0f);
    entity::gp_entityManager->addMinerals(0, 802.0f, 127.0f);
    entity::gp_entityManager->addMinerals(1, -389.0f, 184.0f);
    entity::gp_entityManager->addMinerals(1, 1275.0f, 1259.0f);
    entity::gp_entityManager->addMinerals(1, 756.0f, 1045.0f);
    entity::gp_entityManager->addMinerals(1, 155.0f, 1362.0f);
    entity::gp_entityManager->addMinerals(0, 39.0f, 1914.0f);
    entity::gp_entityManager->addMinerals(1, -140.0f, 1957.0f);
    entity::gp_entityManager->addMinerals(1, -73.0f, 1855.0f);
    entity::gp_entityManager->addMinerals(1, 1167.0f, 2001.0f);
    entity::gp_entityManager->addMinerals(0, 1144.0f, 2168.0f);
    entity::gp_entityManager->addMinerals(1, 1310.0f, 2178.0f);
    entity::gp_entityManager->addMinerals(0, 1104.0f, 2021.0f);
    entity::gp_entityManager->addMinerals(1, 504.0f, 2339.0f);
    entity::gp_entityManager->addMinerals(1, 492.0f, 2351.0f);
    entity::gp_entityManager->addMinerals(1, 639.0f, 2274.0f);
    entity::gp_entityManager->addMinerals(0, 1252.0f, 2799.0f);
    entity::gp_entityManager->addMinerals(2, 1288.0f, 2907.0f);
    entity::gp_entityManager->addMinerals(1, 1103.0f, 2889.0f);
    entity::gp_entityManager->addMinerals(0, 19.0f, 3314.0f);
    entity::gp_entityManager->addMinerals(1, 21.0f, 3353.0f);
    entity::gp_entityManager->addMinerals(1, 32.0f, 3428.0f);
    entity::gp_entityManager->addMinerals(1, -164.0f, 3350.0f);
    entity::gp_entityManager->addMinerals(0, -93.0f, 2528.0f);
    entity::gp_entityManager->addMinerals(1, 39.0f, 2624.0f);
    entity::gp_entityManager->addMinerals(0, -23.0f, 2619.0f);
    entity::gp_entityManager->addMinerals(0, 730.0f, 3087.0f);
    entity::gp_entityManager->addMinerals(1, 706.0f, 3019.0f);
    entity::gp_entityManager->addMinerals(1, 713.0f, 2920.0f);
    entity::gp_entityManager->addMinerals(1, 999.0f, 2731.0f);
    entity::gp_entityManager->addMinerals(1, 1222.0f, 3235.0f);
    entity::gp_entityManager->addMinerals(0, 1302.0f, 3336.0f);
    entity::gp_entityManager->addMinerals(1, 1067.0f, 3478.0f);
    entity::gp_entityManager->addMinerals(1, 949.0f, 3498.0f);
    entity::gp_entityManager->addMinerals(1, 1169.0f, 3317.0f);
    entity::gp_entityManager->addMinerals(0, 426.0f, 3602.0f);
    entity::gp_entityManager->addMinerals(2, 319.0f, 3769.0f);
    entity::gp_entityManager->addMinerals(0, 423.0f, 3844.0f);
    entity::gp_entityManager->addMinerals(1, 423.0f, 3745.0f);
    entity::gp_entityManager->addMinerals(0, 366.0f, 3658.0f);
    entity::gp_entityManager->addMinerals(1, 390.0f, 2898.0f);
    entity::gp_entityManager->addMinerals(1, 245.0f, 2960.0f);
    entity::gp_entityManager->addMinerals(1, 812.0f, 1780.0f);
    entity::gp_entityManager->addMinerals(0, 665.0f, 1672.0f);
    entity::gp_entityManager->addMinerals(1, 821.0f, 1741.0f);
    entity::gp_entityManager->addMinerals(1, 692.0f, 1783.0f);
    entity::gp_entityManager->addMinerals(1, 776.0f, 1682.0f);
    entity::gp_entityManager->addMinerals(1, 1029.0f, 1242.0f);
    entity::gp_entityManager->addMinerals(0, 987.0f, 1310.0f);
    entity::gp_entityManager->addMinerals(1, -394.0f, 2239.0f);
    entity::gp_entityManager->addMinerals(1, -466.0f, 2274.0f);
    entity::gp_entityManager->addMinerals(0, -402.0f, 2301.0f);
    entity::gp_entityManager->addMinerals(1, -324.0f, 2978.0f);
    entity::gp_entityManager->addMinerals(1, 431.0f, 2802.0f);
    entity::gp_entityManager->addMinerals(0, -292.0f, 3872.0f);
    entity::gp_entityManager->addMinerals(1, -139.0f, 3782.0f);
    entity::gp_entityManager->addMinerals(0, 949.0f, 3874.0f);
    entity::gp_entityManager->addMinerals(1, 1052.0f, 3933.0f);
    entity::gp_entityManager->addMinerals(2, 1184.0f, 3980.0f);
    entity::gp_entityManager->addMinerals(0, 1095.0f, 3863.0f);
    entity::gp_entityManager->addMinerals(1, 1039.0f, 3856.0f);
    entity::gp_entityManager->addMinerals(1, 990.0f, 3929.0f);
    entity::gp_entityManager->addMinerals(1, 1121.0f, 3980.0f);
    entity::gp_entityManager->addMinerals(0, 985.0f, 3531.0f);
    entity::gp_entityManager->addMinerals(1, 925.0f, 2316.0f);
    entity::gp_entityManager->addMinerals(1, 955.0f, 2422.0f);
    entity::gp_entityManager->addMinerals(1, 1349.0f, 1478.0f);
    entity::gp_entityManager->addMinerals(0, 1434.0f, 1572.0f);
    entity::gp_entityManager->addMinerals(1, 1172.0f, 1670.0f);
    entity::gp_entityManager->addMinerals(0, 144.0f, 2233.0f);
    entity::gp_entityManager->addMinerals(0, 389.0f, 1842.0f);
    entity::gp_entityManager->addMinerals(1, 448.0f, 1113.0f);
    entity::gp_entityManager->addMinerals(0, 740.0f, 2608.0f);
    entity::gp_entityManager->addMinerals(1, 751.0f, 3745.0f);
    entity::gp_entityManager->addMinerals(1, 1159.0f, 387.0f);
    entity::gp_entityManager->addMinerals(2, -319.0f, -300.0f);
    entity::gp_entityManager->addMinerals(1, -273.0f, -328.0f);
    entity::gp_entityManager->addMinerals(1, -299.0f, -393.0f);
    entity::gp_entityManager->addMinerals(0, -295.0f, -418.0f);
    entity::gp_entityManager->addMinerals(1, 110.0f, -407.0f);
    entity::gp_entityManager->addMinerals(0, 174.0f, -491.0f);
    entity::gp_entityManager->addMinerals(1, 159.0f, -517.0f);
    entity::gp_entityManager->addMinerals(1, 311.0f, -435.0f);
    entity::gp_entityManager->addMinerals(0, 870.0f, -967.0f);
    entity::gp_entityManager->addMinerals(2, 944.0f, -943.0f);
    entity::gp_entityManager->addMinerals(1, 974.0f, -909.0f);
    entity::gp_entityManager->addMinerals(1, 861.0f, -877.0f);
    entity::gp_entityManager->addMinerals(0, 1022.0f, -455.0f);
    entity::gp_entityManager->addMinerals(1, 984.0f, -328.0f);
    entity::gp_entityManager->addMinerals(0, 874.0f, -422.0f);
    entity::gp_entityManager->addMinerals(1, 896.0f, -584.0f);
    entity::gp_entityManager->addMinerals(0, 675.0f, -349.0f);
    entity::gp_entityManager->addMinerals(1, 658.0f, -347.0f);
    entity::gp_entityManager->addMinerals(0, 819.0f, -296.0f);
    entity::gp_entityManager->addMinerals(1, 1461.0f, -744.0f);
    entity::gp_entityManager->addMinerals(1, 1418.0f, -885.0f);
    entity::gp_entityManager->addMinerals(1, 1266.0f, -862.0f);
    entity::gp_entityManager->addMinerals(0, 1291.0f, -681.0f);
    entity::gp_entityManager->addMinerals(1, 1395.0f, -807.0f);
    entity::gp_entityManager->addMinerals(1, 1349.0f, -767.0f);
    entity::gp_entityManager->addMinerals(2, 437.0f, -930.0f);
    entity::gp_entityManager->addMinerals(1, 246.0f, -919.0f);
    entity::gp_entityManager->addMinerals(1, 342.0f, -979.0f);
    entity::gp_entityManager->addMinerals(2, 357.0f, -916.0f);
    entity::gp_entityManager->addMinerals(1, -146.0f, -878.0f);
    entity::gp_entityManager->addMinerals(0, -356.0f, -901.0f);
    entity::gp_entityManager->addMinerals(2, -335.0f, -797.0f);
    entity::gp_entityManager->addMinerals(0, 608.0f, -616.0f);
    entity::gp_entityManager->addMinerals(1, 1305.0f, -83.0f);
    entity::gp_entityManager->addMinerals(0, 1360.0f, -42.0f);
    entity::gp_entityManager->addMinerals(1, 1354.0f, -119.0f);
    entity::gp_entityManager->addMinerals(1, 1034.0f, -24.0f);
    entity::gp_entityManager->addMinerals(0, 31.0f, -69.0f);
    entity::gp_entityManager->addMinerals(2, 269.0f, -180.0f);
    entity::gp_entityManager->addMinerals(0, 904.0f, 606.0f);
    entity::gp_entityManager->addMinerals(0, 912.0f, 577.0f);
    entity::gp_entityManager->addMinerals(0, 788.0f, 581.0f);
    entity::CAlienEntity* alien = entity::gp_entityManager->addAlien(0, -214.0f, -426.0f);
    alien->Target.Id = firstDefenseId;
    alien->Target.Entity = 0;
    alien = entity::gp_entityManager->addAlien(0, -222.0f, -387.0f);
    alien->Target.Id = secondDefenseId;
    alien->Target.Entity = 0;
    entity::gp_entityManager->addAlien(0, -259.0f, -371.0f);
    alien = entity::gp_entityManager->addAlien(0, 1075.0f, -401.0f);
    alien->Target.Id = fourthDefenseId;
    alien->Target.Entity = 0;
    entity::gp_entityManager->addAlien(0, 1098.0f, -376.0f);
    alien = entity::gp_entityManager->addAlien(0, 1034.0f, -424.0f);
    alien->Target.Id = firstDefenseId;
    alien->Target.Entity = 0;
    alien = entity::gp_entityManager->addAlien(0, 231.0f, -426.0f);
    alien->Target.Id = thirdDefenseId;
    alien->Target.Entity = 0;
    alien = entity::gp_entityManager->addAlien(0, 148.0f, -369.0f);
    alien->Target.Id = fourthDefenseId;
    alien->Target.Entity = 0;
    alien = entity::gp_entityManager->addAlien(0, 63.0f, -391.0f);
    alien->Target.Id = firstDefenseId;
    alien->Target.Entity = 0;
    TrackedAlienId = entity::gp_entityManager->addAlien(0, 564.0f, -438.0f)->getId();
    alien = entity::gp_entityManager->addAlien(0, -59.0f, -295.0f);
    alien->Target.Id = secondDefenseId;
    alien->Target.Entity = 0;
    alien = entity::gp_entityManager->addAlien(0, -26.0f, -332.0f);
    alien->Target.Id = thirdDefenseId;
    alien->Target.Entity = 0;
    entity::gp_entityManager->addAlien(0, 1313.0f, -391.0f);
    Dropship = new entity::CDropshipEntity(512, 9700, false);
    entity::gp_entityManager->appendEntity(Dropship, 4);
}

bool CMineralsCondition::testCondition()
{
    return comparisonTest(gp_mineralAmount->getValue(), Value, Comparison);
}

bool CAlienCountCondition::testCondition()
{
    return comparisonTest(entity::gp_entityManager->getNumAliens(), Value, Comparison);
}

static const char* const CHARACTER_PORTRAITS[] = {
    "PortraitCommunications", "PortraitDropShip", "PortraitBase"
};

gui::CDialogueItemInfo* CScenario::getCharacterDialog(const wchar_t* key, int character, char* sound, float time)
{
    ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(key);
    return new gui::CDialogueItemInfo(CharacterNames[character].c_str(),
        text.c_str(), CHARACTER_PORTRAITS[character], sound, time, false);
}

CAddInfoLineEvent* CScenario::getCharacterInfoLine(float delay, ox::event::IEventReceiver* receiver,
                                                const wchar_t* key, int character, const char* sound)
{
    ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(key);
    return new CAddInfoLineEvent(delay, receiver, CharacterNames[character].c_str(),
        text.c_str(), CHARACTER_PORTRAITS[character], sound);
}

void CScenario::createScenarioEvents(ox::event::IEventReceiver* receiver)
{
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 29;
    event.UserEvent.UserData2 = Dropship->getId();
    event.UserEvent.UserData3 = 4;
    receiver->OnEvent(event);
    addEvent(new CAddDialogueEvent(0, receiver, getCharacterDialog(L"scenario:pilot1", 1, "pilot1-2.ogg", 19)));
    addEvent(new CAddDialogueEvent(19, receiver, getCharacterDialog(L"scenario:pilot2", 1, "", 10)));
    addEvent(new CAddDialogueEvent(10, receiver, getCharacterDialog(L"scenario:officer1", 2, "officer1.ogg", 7)));
    addEvent(new CAddDialogueEvent(7, receiver, getCharacterDialog(L"scenario:pilot3", 1, "pilot3.ogg", 10)));
    addEvent(new CPostEventEvent(9, receiver, 39, 0, 0));
    addEvent(new CDropshipStateEvent(1, Dropship, 1));
    gui::CDialogueItemInfo* credit = new gui::CDialogueItemInfo(L"",
        L"Jens Bergensten\n   - Programming & Game Design", "", "", 7, true);
    addEvent(new CAddDialogueEvent(13, receiver, credit));
    credit = new gui::CDialogueItemInfo(L"",
        L"Daniel Brynolf\n   - 2D/3D Artwork & Game Design", "", "", 7, false);
    addEvent(new CAddDialogueEvent(5.5f, receiver, credit));
    credit = new gui::CDialogueItemInfo(L"",
        L"Pontus Hammarberg\n   - 2d/3D Artwork & Cinematics", "", "", 7, true);
    addEvent(new CAddDialogueEvent(5.5f, receiver, credit));
    credit = new gui::CDialogueItemInfo(L"",
        L"Alexander Persson\n   - Platform Programming", "", "", 7, false);
    addEvent(new CAddDialogueEvent(5.5f, receiver, credit));
    credit = new gui::CDialogueItemInfo(L"",
        L"Jonas Johnsson\n   - Programming", "", "", 7, true);
    addEvent(new CAddDialogueEvent(5.5f, receiver, credit));
    credit = new gui::CDialogueItemInfo(L"",
        L"Steve Olofsson\n   - Music", "", "", 7, false);
    addEvent(new CAddDialogueEvent(5.5f, receiver, credit));
}

void CScenario::notifyDropshipKilledAllAliens(ox::event::IEventReceiver* receiver)
{
    addEvent(new CAddDialogueEvent(0, receiver, getCharacterDialog(L"scenario:pilotDone1", 1, "piloteDone1.ogg", 5)));
    addEvent(new CAddDialogueEvent(5, receiver, getCharacterDialog(L"scenario:officerDone1", 2, "officerDone1.ogg", 9)));
    addEvent(new CAddDialogueEvent(9, receiver, getCharacterDialog(L"scenario:pilotDone2", 1, "pilotDone2.ogg", 4)));
    addEvent(new CAddDialogueEvent(4, receiver, getCharacterDialog(L"scenario:officerDone2", 2, "officerDone2.ogg", 7)));
    addEvent(new CAddDialogueEvent(7, receiver, getCharacterDialog(L"scenario:pilotDone3", 1, "pilotDone3.ogg", 4)));
}

void CScenario::notifyDropshipLanded(ox::event::IEventReceiver* receiver)
{
    addEvent(new CAddDialogueEvent(0, receiver, getCharacterDialog(L"scenario:pilotLanded1", 1, "pilotLanded1.ogg", 5)));
    addEvent(new CAddDialogueEvent(5, receiver, getCharacterDialog(L"scenario:officerLanded1", 2, "officerLanded1.ogg", 18)));
    addEvent(new CAddDialogueEvent(18, receiver, getCharacterDialog(L"scenario:pilotLanded2", 1, "pilotLanded2.ogg", 7)));
    addEvent(new CAddDialogueEvent(7, receiver, getCharacterDialog(L"scenario:officerLanded2", 2, "officerLanded2.ogg", 6)));
    addEvent(new CTrackEntityEvent(6, receiver, -1, -1));
    addEvent(new CDropshipStateEvent(0, Dropship, 5));
    addEvent(new CPostEventEvent(0, receiver, 24, -1, -1));
    addEvent(new CPostEventEvent(0, receiver, 34, -1, -1));
    addEvent(getCharacterInfoLine(7, receiver, L"scenario:officerTraining1", 2, "officerTraining1.ogg"));
    addEvent(new CTrackEntityEvent(6, receiver, StartingCollectorId, 0));
    addEvent(new CPostEventEvent(0, receiver, 38, 0, 0));
    addEvent(new CTrackEntityEvent(1, receiver, -1, -1));
    addEvent(getCharacterInfoLine(9, receiver, L"scenario:officerTraining2", 2, "officerTraining2.ogg"));
    addEvent(new CPostEventEvent(0, receiver, 38, 1, 0));
    addEvent(getCharacterInfoLine(10, receiver, L"scenario:officerTraining3", 2, "officerTraining3.ogg"));
    addEvent(new CPostEventEvent(0, receiver, 38, 4, 0));
    addEvent(getCharacterInfoLine(10, receiver, L"scenario:officerTraining4", 2, "officerTraining4.ogg"));
    addEvent(getCharacterInfoLine(45, receiver, L"scenario:officerTraining5", 2, "officerTraining5.ogg"));
}

void CScenario::notifyDropshipGoingToSpace(ox::event::IEventReceiver* receiver)
{
    CConditionalHarvestEvent* halfway = new CConditionalHarvestEvent(this);
    halfway->Events.push_back(getCharacterInfoLine(0, receiver, L"scenario:officerHalf", 2, "officerHalf.ogg"));
    CHarvestEventCondition* condition = new CMineralsCondition(100, 3);
    halfway->Conditions.push_back(condition);
    ConditionalEvents.push_back(halfway);

    CConditionalHarvestEvent* reinforcements = new CConditionalHarvestEvent(this);
    reinforcements->Events.push_back(getCharacterInfoLine(0, receiver, L"scenario:officerAliens", 2, "officerAliens.ogg"));
    entity::CAlienEntity* leader = new entity::CAlienEntity(1700, 500, 0);
    reinforcements->Events.push_back(new CSpawnEntityEvent(0, leader, 1));
    reinforcements->Events.push_back(new CSpawnEntityEvent(0, new entity::CAlienEntity(1725, 510, 0), 1));
    reinforcements->Events.push_back(new CSpawnEntityEvent(0, new entity::CAlienEntity(1675, 520, 0), 1));
    reinforcements->Events.push_back(new CSpawnEntityEvent(0, new entity::CAlienEntity(1600, 530, 0), 1));
    reinforcements->Events.push_back(new CSpawnEntityEvent(0, new entity::CAlienEntity(1780, 540, 0), 1));
    reinforcements->Events.push_back(new CSpawnEntityEvent(0, new entity::CAlienEntity(1810, 550, 0), 1));
    reinforcements->Events.push_back(new CSpawnEntityEvent(0, new entity::CAlienEntity(1725, 560, 0), 1));
    reinforcements->Events.push_back(new CTrackEntityEvent(1, receiver, leader->getId(), 1));
    reinforcements->Events.push_back(new CPostEventEvent(0, receiver, 38, 6, 1));
    reinforcements->Events.push_back(new CTrackEntityEvent(9, receiver, StartingCollectorId, 0));
    reinforcements->Events.push_back(new CTrackEntityEvent(1, receiver, -1, -1));
    condition = new CMineralsCondition(150, 3);
    reinforcements->Conditions.push_back(condition);
    ConditionalEvents.push_back(reinforcements);

    CConditionalHarvestEvent* completed = new CConditionalHarvestEvent(this);
    completed->Events.push_back(getCharacterInfoLine(3, receiver, L"scenario:infoCompleted", 0, "scenario_infoCompleted.ogg"));
    completed->Events.push_back(new CPostEventEvent(0, receiver, 26, 0, 0));
    condition = new CMineralsCondition(200, 3);
    completed->Conditions.push_back(condition);
    condition = new CAlienCountCondition(0, 0);
    completed->Conditions.push_back(condition);
    ConditionalEvents.push_back(completed);
}

} // end namespace game
} // end namespace harvest
