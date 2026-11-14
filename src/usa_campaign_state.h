#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class UsaFaction
{
    THE_PROJECT = 0, // Player (Blue)
    NEW_PARADIGM,    // Southwest/Rockies (Amber/Orange)
    THE_COLLECTIVE,  // Midwest/Plains (Red)
    NEXUS_REMNANT,   // Northeast/Rust Belt (Purple)
    SCAV_SYNDICATE   // Northwest/Outlands (Olive/Teal)
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

struct UsaCampaignState
{
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

    void initDefault();
    bool saveToPhysfs() const;
    bool loadFromPhysfs();
};

UsaCampaignState& getUsaCampaignState();
UsaColor getUsaFactionColor(UsaFaction f, bool isSelected = false);
std::string getUsaFactionName(UsaFaction f);
bool usaPointInPolygon(const UsaVec2& p, const std::vector<UsaVec2>& poly);
