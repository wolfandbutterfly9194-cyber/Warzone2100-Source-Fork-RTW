#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class UsaFaction
{
    THE_PROJECT = 0,
    NEW_PARADIGM,
    THE_COLLECTIVE,
    NEXUS_REMNANT,
    SCAV_SYNDICATE,
    SCAV_PACIFIC,
    SCAV_ROCKIES,
    SCAV_DESERT,
    SCAV_BADLANDS,
    SCAV_HEARTLAND,
    SCAV_LAKES,
    SCAV_OHIO,
    SCAV_GULF,
    SCAV_PIEDMONT,
    SCAV_ATLANTIC,
    SCAV_NEW_ENGLAND,
    SCAV_NORTHWOODS
};

struct UsaVec2 { float x = 0.0f; float y = 0.0f; };
struct UsaColor { uint8_t r = 255; uint8_t g = 255; uint8_t b = 255; uint8_t a = 255; };

struct SavedEntity
{
    std::string kind;
    std::string id;
    std::string name;
    int x = 0;
    int y = 0;
    std::string body;
    std::string prop;
    std::string weapon;
};

struct Territory
{
    std::string code;
    std::string name;
    UsaFaction owner = UsaFaction::THE_PROJECT;
    int powerIncome = 100;
    std::string wzMap;
    std::vector<UsaVec2> polygon;
    UsaVec2 labelPos;
    std::vector<std::string> neighbors;
    std::vector<SavedEntity> savedEntities;
};
struct TechUpgrade { std::string id; std::string label; int cost = 0; bool unlocked = false; std::string wzResearchId; };
struct ComponentDef { std::string id; std::string name; int cost = 0; int hpBonus = 0; int firepower = 0; std::string roleHint; };
struct DesignedBlueprint { std::string name; std::string bodyId; std::string propId; std::string weaponId; int unitCost = 0; int count = 0; };

struct GeneralPartOption
{
    std::string id;
    std::string name;
    int statBonus = 10;
};

struct GeneralUpgradeNode
{
    std::string id;
    std::string label;
    int level = 0;
    int maxLevel = 5;
    int baseCost = 200;
};

struct UsaGeneralVariant
{
    std::string name;
    std::string bodyId;
    std::string propId;
    std::string weapon1Id;
    std::string weapon2Id;
};

struct UsaGeneralState
{
    std::string name = "Supreme Commander (Dragon HQ)";
    std::string bodyId = "";
    std::string propId = "";
    std::string weapon1Id = "";
    std::string weapon2Id = "";
    std::vector<GeneralPartOption> bodyOptions;
    std::vector<GeneralPartOption> propOptions;
    std::vector<GeneralPartOption> weap1Options;
    std::vector<GeneralPartOption> weap2Options;
    std::vector<GeneralUpgradeNode> upgrades;
    std::vector<UsaGeneralVariant> variants;
};

struct UsaCampaignState
{
    std::string playerFactionName = "The Project";
    int playerColorIndex = 0;
    bool campaignConfigured = false;
    int turn = 1;
    int power = 1200;
    int selectedIndex = 2; // CA
    int activeTargetIndex = -1;
    bool activeSessionIsBaseMode = false;
    bool activeSessionIsDefense = false;

    std::vector<Territory> states;
    std::vector<TechUpgrade> techs;
    std::vector<ComponentDef> bodies;
    std::vector<ComponentDef> props;
    std::vector<ComponentDef> weapons;
    std::vector<DesignedBlueprint> squad;

    UsaGeneralState general;

    void initDefault();
    bool saveToPhysfs() const;
    bool loadFromPhysfs();
};

UsaCampaignState& getUsaCampaignState();
UsaColor getUsaFactionColor(UsaFaction f, bool isSelected = false);
std::string getUsaFactionName(UsaFaction f);
bool usaPointInPolygon(const UsaVec2& p, const std::vector<UsaVec2>& poly);
