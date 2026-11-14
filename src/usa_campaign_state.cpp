#include "usa_campaign_state.h"
#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"
#include <algorithm>
#include <sstream>

namespace
{
    using Faction = UsaFaction;
std::vector<Territory> buildContinentalUSA() {
    return {
        { "WA", "Washington",     Faction::SCAV_SYNDICATE, 150, "Sk-Rush",
          { { 115,115 },{ 195,115 },{ 195,175 },{ 125,175 },{ 110,145 } },{ 142,138 },{ "OR","ID" } },
        { "OR", "Oregon",         Faction::SCAV_SYNDICATE, 140, "Sk-Mountain",
          { { 110,175 },{ 195,175 },{ 195,245 },{ 100,245 } },{ 138,202 },{ "WA","ID","NV","CA" } },
        { "CA", "California",     Faction::THE_PROJECT,    260, "Sk-Startup",
          { { 100,245 },{ 155,245 },{ 155,305 },{ 205,380 },{ 185,425 },{ 135,395 },{ 105,315 } },{ 135,325 },{ "OR","NV","AZ" } },
        { "NV", "Nevada",         Faction::THE_PROJECT,    130, "Sk-SandCastles",
          { { 155,245 },{ 220,245 },{ 220,340 },{ 195,365 },{ 155,305 } },{ 176,285 },{ "OR","ID","UT","AZ","CA" } },
        { "ID", "Idaho",          Faction::SCAV_SYNDICATE, 120, "Sk-HighGround",
          { { 195,115 },{ 225,115 },{ 235,190 },{ 265,190 },{ 265,245 },{ 195,245 } },{ 215,195 },{ "WA","OR","NV","UT","WY","MT" } },
        { "MT", "Montana",        Faction::SCAV_SYNDICATE, 140, "Sk-HighGround",
          { { 225,115 },{ 365,115 },{ 365,190 },{ 235,190 } },{ 285,145 },{ "ID","WY","ND","SD" } },
        { "WY", "Wyoming",        Faction::NEW_PARADIGM,   120, "Sk-BeggarsKanyon",
          { { 265,190 },{ 365,190 },{ 365,260 },{ 265,260 } },{ 305,218 },{ "MT","ID","UT","CO","NE","SD" } },
        { "UT", "Utah",           Faction::NEW_PARADIGM,   135, "Sk-Valley",
          { { 220,245 },{ 280,245 },{ 280,335 },{ 220,335 } },{ 238,285 },{ "ID","WY","CO","AZ","NV" } },
        { "AZ", "Arizona",        Faction::NEW_PARADIGM,   165, "Sk-LittleEgypt",
          { { 205,335 },{ 280,335 },{ 280,425 },{ 205,425 },{ 185,380 } },{ 232,372 },{ "CA","NV","UT","NM" } },
        { "CO", "Colorado",       Faction::NEW_PARADIGM,   175, "Sk-GreatRift",
          { { 280,260 },{ 380,260 },{ 380,335 },{ 280,335 } },{ 318,290 },{ "WY","NE","KS","OK","NM","UT" } },
        { "NM", "New Mexico",     Faction::NEW_PARADIGM,   145, "Sk-LittleEgypt",
          { { 280,335 },{ 365,335 },{ 365,430 },{ 280,430 } },{ 312,375 },{ "AZ","CO","OK","TX" } },
        { "ND", "North Dakota",   Faction::THE_COLLECTIVE, 125, "Sk-RollingHills",
          { { 365,115 },{ 465,115 },{ 465,175 },{ 365,175 } },{ 405,138 },{ "MT","SD","MN" } },
        { "SD", "South Dakota",   Faction::THE_COLLECTIVE, 125, "Sk-RollingHills",
          { { 365,175 },{ 465,175 },{ 465,235 },{ 365,235 } },{ 405,198 },{ "ND","MT","WY","NE","IA","MN" } },
        { "NE", "Nebraska",       Faction::THE_COLLECTIVE, 140, "Sk-BeggarsKanyon",
          { { 365,235 },{ 475,235 },{ 485,290 },{ 380,290 },{ 380,260 },{ 365,260 } },{ 415,255 },{ "SD","WY","CO","KS","MO","IA" } },
        { "KS", "Kansas",         Faction::THE_COLLECTIVE, 150, "Sk-Clover",
          { { 380,290 },{ 495,290 },{ 495,350 },{ 380,350 } },{ 428,312 },{ "NE","CO","OK","MO" } },
        { "OK", "Oklahoma",       Faction::NEW_PARADIGM,   155, "Sk-BeggarsKanyon",
          { { 365,350 },{ 505,350 },{ 505,405 },{ 405,405 },{ 405,375 },{ 365,375 } },{ 435,370 },{ "KS","CO","NM","TX","AR","MO" } },
        { "TX", "Texas",          Faction::NEW_PARADIGM,   260, "Sk-GreatRift",
          { { 365,375 },{ 405,375 },{ 405,405 },{ 515,405 },{ 525,475 },{ 465,535 },{ 425,495 },{ 385,485 },{ 355,430 } },{ 440,445 },{ "NM","OK","AR","LA" } },
        { "MN", "Minnesota",      Faction::THE_COLLECTIVE, 165, "Sk-Valley",
          { { 465,115 },{ 545,115 },{ 535,215 },{ 465,215 } },{ 492,158 },{ "ND","SD","IA","WI" } },
        { "IA", "Iowa",           Faction::THE_COLLECTIVE, 150, "Sk-Clover",
          { { 465,215 },{ 545,215 },{ 555,275 },{ 475,275 } },{ 500,238 },{ "MN","SD","NE","MO","IL","WI" } },
        { "MO", "Missouri",       Faction::THE_COLLECTIVE, 170, "Sk-Ziggurat",
          { { 475,275 },{ 560,275 },{ 575,355 },{ 495,355 } },{ 515,308 },{ "IA","NE","KS","OK","AR","TN","KY","IL" } },
        { "AR", "Arkansas",       Faction::THE_COLLECTIVE, 140, "Sk-LittleEgypt",
          { { 495,355 },{ 570,355 },{ 565,420 },{ 505,420 } },{ 522,380 },{ "MO","OK","TX","LA","MS","TN" } },
        { "LA", "Louisiana",      Faction::THE_COLLECTIVE, 160, "Sk-SandCastles",
          { { 515,420 },{ 575,420 },{ 595,485 },{ 525,485 } },{ 542,448 },{ "AR","TX","MS" } },
        { "WI", "Wisconsin",      Faction::THE_COLLECTIVE, 165, "Sk-RollingHills",
          { { 535,145 },{ 605,155 },{ 600,225 },{ 545,225 } },{ 562,180 },{ "MN","IA","IL","MI" } },
        { "IL", "Illinois",       Faction::THE_COLLECTIVE, 210, "Sk-Manhattan",
          { { 545,225 },{ 600,225 },{ 605,315 },{ 565,315 },{ 555,275 } },{ 568,262 },{ "WI","IA","MO","KY","IN" } },
        { "MI", "Michigan",       Faction::NEXUS_REMNANT,  195, "Sk-Concrete",
          { { 605,145 },{ 670,155 },{ 675,230 },{ 615,230 } },{ 635,185 },{ "WI","IN","OH" } },
        { "IN", "Indiana",        Faction::NEXUS_REMNANT,  165, "Sk-Rush",
          { { 600,230 },{ 645,230 },{ 645,305 },{ 605,305 } },{ 615,260 },{ "MI","IL","KY","OH" } },
        { "OH", "Ohio",           Faction::NEXUS_REMNANT,  195, "Sk-Concrete",
          { { 645,230 },{ 705,230 },{ 705,295 },{ 645,295 } },{ 665,255 },{ "MI","IN","KY","WV","PA" } },
        { "KY", "Kentucky",       Faction::THE_COLLECTIVE, 155, "Sk-Mountain",
          { { 575,305 },{ 675,295 },{ 680,335 },{ 570,340 } },{ 622,312 },{ "IL","IN","OH","WV","VA","TN","MO" } },
        { "TN", "Tennessee",      Faction::THE_COLLECTIVE, 165, "Sk-Valley",
          { { 565,340 },{ 685,335 },{ 675,375 },{ 565,375 } },{ 615,348 },{ "KY","VA","NC","GA","AL","MS","AR","MO" } },
        { "MS", "Mississippi",    Faction::THE_COLLECTIVE, 140, "Sk-LittleEgypt",
          { { 565,375 },{ 615,375 },{ 615,455 },{ 575,455 } },{ 582,408 },{ "TN","AL","LA","AR" } },
        { "AL", "Alabama",        Faction::THE_COLLECTIVE, 155, "Sk-Ziggurat",
          { { 615,375 },{ 665,375 },{ 665,455 },{ 615,455 } },{ 630,408 },{ "TN","GA","FL","MS" } },
        { "FL", "Florida",        Faction::NEW_PARADIGM,   210, "Sk-SandCastles",
          { { 645,455 },{ 715,455 },{ 745,545 },{ 710,550 },{ 680,480 },{ 645,475 } },{ 695,495 },{ "AL","GA" } },
        { "GA", "Georgia",        Faction::NEXUS_REMNANT,  185, "Sk-HighGround",
          { { 665,375 },{ 725,375 },{ 735,455 },{ 665,455 } },{ 688,408 },{ "TN","NC","SC","FL","AL" } },
        { "SC", "South Carolina", Faction::NEXUS_REMNANT,  150, "Sk-SandCastles",
          { { 705,365 },{ 760,365 },{ 740,410 },{ 715,395 } },{ 725,378 },{ "NC","GA" } },
        { "NC", "North Carolina", Faction::NEXUS_REMNANT,  180, "Sk-HighGround",
          { { 685,335 },{ 785,335 },{ 765,370 },{ 680,365 } },{ 732,345 },{ "VA","TN","GA","SC" } },
        { "VA", "Virginia",       Faction::NEXUS_REMNANT,  185, "Sk-BeggarsKanyon",
          { { 695,290 },{ 780,295 },{ 785,335 },{ 685,335 } },{ 735,310 },{ "WV","MD","NC","TN","KY" } },
        { "WV", "West Virginia",  Faction::NEXUS_REMNANT,  135, "Sk-Mountain",
          { { 675,275 },{ 715,275 },{ 705,315 },{ 675,310 } },{ 685,288 },{ "OH","PA","MD","VA","KY" } },
        { "MD", "Maryland",       Faction::NEXUS_REMNANT,  160, "Sk-Concrete",
          { { 725,270 },{ 780,270 },{ 780,295 },{ 725,295 } },{ 745,276 },{ "PA","DE","VA","WV" } },
        { "DE", "Delaware",       Faction::NEXUS_REMNANT,  120, "Sk-Rush",
          { { 780,265 },{ 798,265 },{ 798,295 },{ 780,295 } },{ 782,274 },{ "PA","NJ","MD" } },
        { "PA", "Pennsylvania",   Faction::NEXUS_REMNANT,  210, "Sk-Concrete",
          { { 705,225 },{ 780,225 },{ 780,270 },{ 705,270 } },{ 732,240 },{ "NY","NJ","DE","MD","WV","OH" } },
        { "NJ", "New Jersey",     Faction::NEXUS_REMNANT,  175, "Sk-Startup",
          { { 780,225 },{ 802,225 },{ 802,265 },{ 780,265 } },{ 784,238 },{ "NY","PA","DE" } },
        { "NY", "New York",       Faction::NEXUS_REMNANT,  250, "Sk-Manhattan",
          { { 725,165 },{ 800,165 },{ 800,225 },{ 725,225 } },{ 755,188 },{ "VT","MA","CT","NJ","PA" } },
        { "CT", "Connecticut",    Faction::NEXUS_REMNANT,  150, "Sk-Rush",
          { { 800,205 },{ 830,205 },{ 830,225 },{ 800,225 } },{ 806,208 },{ "NY","MA","RI" } },
        { "RI", "Rhode Island",   Faction::NEXUS_REMNANT,  125, "Sk-Startup",
          { { 830,205 },{ 848,205 },{ 848,225 },{ 830,225 } },{ 832,208 },{ "CT","MA" } },
        { "MA", "Massachusetts",  Faction::NEXUS_REMNANT,  175, "Sk-Manhattan",
          { { 800,185 },{ 850,185 },{ 850,205 },{ 800,205 } },{ 815,188 },{ "VT","NH","RI","CT","NY" } },
        { "VT", "Vermont",        Faction::NEXUS_REMNANT,  125, "Sk-Mountain",
          { { 795,140 },{ 818,140 },{ 818,185 },{ 795,185 } },{ 799,155 },{ "NH","MA","NY" } },
        { "NH", "New Hampshire",  Faction::NEXUS_REMNANT,  130, "Sk-Mountain",
          { { 818,140 },{ 840,140 },{ 840,185 },{ 818,185 } },{ 822,155 },{ "ME","MA","VT" } },
        { "ME", "Maine",          Faction::NEXUS_REMNANT,  145, "Sk-RollingHills",
          { { 840,95 },{ 885,115 },{ 865,175 },{ 840,175 } },{ 852,130 },{ "NH" } }
    };
}
    void saveStateBasePhysfs(const Territory& t)
    {
        PHYSFS_mkdir("usa_campaign");
        PHYSFS_mkdir("usa_campaign/bases");
        std::string filePath = "usa_campaign/bases/" + t.code + ".base";
        std::ostringstream out;
        for (const auto& e : t.savedEntities)
        {
            std::string safeName = e.name.empty() ? e.id : e.name;
            std::replace(safeName.begin(), safeName.end(), ' ', '_');
            if (e.kind == "STRUCT") out << "STRUCT " << e.id << " " << e.x << " " << e.y << " " << safeName << "\n";
            else if (e.kind == "DROID") out << "DROID " << e.body << " " << e.prop << " " << e.weapon << " " << e.x << " " << e.y << " " << safeName << "\n";
        }
        std::string str = out.str();
        PHYSFS_File* f = PHYSFS_openWrite(filePath.c_str());
        if (f != nullptr) { PHYSFS_writeBytes(f, str.data(), str.size()); PHYSFS_close(f); }
    }

    void loadStateBasePhysfs(Territory& t)
    {
        t.savedEntities.clear();
        std::string filePath = "usa_campaign/bases/" + t.code + ".base";
        if (!PHYSFS_exists(filePath.c_str())) return;
        PHYSFS_File* f = PHYSFS_openRead(filePath.c_str());
        if (f == nullptr) return;
        PHYSFS_sint64 len = PHYSFS_fileLength(f);
        if (len <= 0) { PHYSFS_close(f); return; }
        std::string content(static_cast<size_t>(len), '\0');
        PHYSFS_readBytes(f, content.data(), static_cast<PHYSFS_uint64>(len));
        PHYSFS_close(f);

        std::istringstream in(content);
        std::string kind;
        while (in >> kind)
        {
            SavedEntity e; e.kind = kind;
            if (kind == "STRUCT" && (in >> e.id >> e.x >> e.y >> e.name)) { std::replace(e.name.begin(), e.name.end(), '_', ' '); t.savedEntities.push_back(e); }
            else if (kind == "DROID" && (in >> e.body >> e.prop >> e.weapon >> e.x >> e.y >> e.name)) { std::replace(e.name.begin(), e.name.end(), '_', ' '); t.savedEntities.push_back(e); }
        }
    }
}
UsaCampaignState& getUsaCampaignState()
{
    static UsaCampaignState instance;
    static bool initialized = false;
    if (!initialized) { initialized = true; instance.initDefault(); instance.loadFromPhysfs(); }
    return instance;
}

UsaColor getUsaFactionColor(UsaFaction f, bool isSelected)
{
    UsaColor c{40, 125, 225, 255};
    switch (f)
    {
    case UsaFaction::THE_PROJECT:    c = UsaColor{40, 125, 225, 255};  break;
    case UsaFaction::NEW_PARADIGM:   c = UsaColor{220, 125, 35, 255};  break;
    case UsaFaction::THE_COLLECTIVE: c = UsaColor{200, 48, 48, 255};   break;
    case UsaFaction::NEXUS_REMNANT:  c = UsaColor{145, 55, 195, 255};  break;
    case UsaFaction::SCAV_SYNDICATE: c = UsaColor{55, 145, 110, 255};  break;
    }
    if (isSelected)
    {
        c.r = static_cast<uint8_t>(std::min(255, static_cast<int>(c.r) + 55));
        c.g = static_cast<uint8_t>(std::min(255, static_cast<int>(c.g) + 55));
        c.b = static_cast<uint8_t>(std::min(255, static_cast<int>(c.b) + 55));
    }
    return c;
}

std::string getUsaFactionName(UsaFaction f)
{
    switch (f)
    {
    case UsaFaction::THE_PROJECT:    return "The Project (YOU)";
    case UsaFaction::NEW_PARADIGM:   return "New Paradigm";
    case UsaFaction::THE_COLLECTIVE: return "The Collective";
    case UsaFaction::NEXUS_REMNANT:  return "NEXUS Remnant";
    case UsaFaction::SCAV_SYNDICATE: return "Western Scav Syndicate";
    }
    return "Unknown";
}

bool usaPointInPolygon(const UsaVec2& p, const std::vector<UsaVec2>& poly)
{
    bool inside = false;
    size_t n = poly.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++)
    {
        if (((poly[i].y > p.y) != (poly[j].y > p.y)) &&
            (p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / (poly[j].y - poly[i].y + 0.0001f) + poly[i].x))
        {
            inside = !inside;
        }
    }
    return inside;
}

void UsaCampaignState::initDefault()
{
    turn = 1; power = 1200; selectedIndex = 2; activeTargetIndex = -1;
    activeSessionIsBaseMode = false; activeSessionIsDefense = false;
    states = buildContinentalUSA();
    techs = {
        { "MG_DMG",  "Twin Machinegun Ammo (+25% Kinetic)", 350, false, "R-Wpn-MG-Damage02" },
        { "ARMOR",   "Composite Alloys (+30% Kinetic Armor)", 450, false, "R-Vehicle-Metals02" },
        { "ENGINE",  "Turbocharged Engines (+20% Speed)",     400, false, "R-Vehicle-Engine02" },
        { "CANNON",  "Medium Cannon Unlock (Heavy Armor)",   600, false, "R-Wpn-Cannon2Mk1" }
    };
    bodies = { { "Body1REC", "Viper (Light)", 60, 100, 0, "Fast Recon" }, { "Body5REC", "Cobra (Medium)", 130, 220, 0, "Balanced Assault" }, { "Body11ABT", "Python (Heavy)", 240, 400, 0, "Heavy Breakthrough" } };
    props  = { { "wheeled01", "Wheels", 30, 20, 0, "High Road Speed" }, { "HalfTrack", "Half-Tracks", 60, 65, 0, "All-Terrain" }, { "tracked01", "Heavy Tracks", 110, 140, 0, "Max Armor" } };
    weapons = { { "MG1Mk1", "Machinegun", 40, 0, 25, "Anti-Infantry" }, { "Cannon1Mk1", "Light Cannon", 95, 0, 65, "Anti-Vehicle" }, { "Rocket-Pod", "Mini-Rocket Pod", 120, 0, 85, "Burst Barrage" }, { "Mortar1Mk1", "Mortar Turret", 110, 0, 75, "Indirect Artillery" } };
    squad = { { "Cobra Half-Track Cannon", "Body5REC", "HalfTrack", "Cannon1Mk1", 285, 2 } };
}
bool UsaCampaignState::saveToPhysfs() const
{
    PHYSFS_mkdir("usa_campaign");
    std::ostringstream out;
    out << "TURN " << turn << "\nPOWER " << power << "\n";
    for (const auto& s : states)
    {
        out << "STATE " << s.code << " " << static_cast<int>(s.owner) << "\n";
        if (!s.savedEntities.empty()) saveStateBasePhysfs(s);
    }
    for (const auto& t : techs) out << "TECH " << t.id << " " << (t.unlocked ? 1 : 0) << "\n";
    for (const auto& bp : squad)
    {
        std::string safeName = bp.name;
        std::replace(safeName.begin(), safeName.end(), ' ', '_');
        out << "BLUEPRINT " << safeName << " " << bp.bodyId << " " << bp.propId << " " << bp.weaponId << " " << bp.unitCost << " " << bp.count << "\n";
    }
    std::string str = out.str();
    PHYSFS_File* f = PHYSFS_openWrite("usa_campaign/usa_campaign_save.txt");
    if (f == nullptr) return false;
    PHYSFS_writeBytes(f, str.data(), str.size());
    PHYSFS_close(f);
    return true;
}

bool UsaCampaignState::loadFromPhysfs()
{
    if (!PHYSFS_exists("usa_campaign/usa_campaign_save.txt")) return false;
    PHYSFS_File* f = PHYSFS_openRead("usa_campaign/usa_campaign_save.txt");
    if (f == nullptr) return false;
    PHYSFS_sint64 len = PHYSFS_fileLength(f);
    if (len <= 0) { PHYSFS_close(f); return false; }
    std::string content(static_cast<size_t>(len), '\0');
    PHYSFS_readBytes(f, content.data(), static_cast<PHYSFS_uint64>(len));
    PHYSFS_close(f);

    squad.clear();
    std::istringstream in(content);
    std::string tag;
    while (in >> tag)
    {
        if (tag == "TURN") in >> turn;
        else if (tag == "POWER") in >> power;
        else if (tag == "STATE")
        {
            std::string code; int ownerInt = 0;
            if (in >> code >> ownerInt)
            {
                for (auto& s : states)
                {
                    if (s.code == code) { s.owner = static_cast<UsaFaction>(ownerInt); loadStateBasePhysfs(s); break; }
                }
            }
        }
        else if (tag == "TECH")
        {
            std::string id; int unl = 0;
            if (in >> id >> unl) { for (auto& t : techs) if (t.id == id) { t.unlocked = (unl != 0); break; } }
        }
        else if (tag == "BLUEPRINT")
        {
            DesignedBlueprint bp;
            if (in >> bp.name >> bp.bodyId >> bp.propId >> bp.weaponId >> bp.unitCost >> bp.count)
            {
                std::replace(bp.name.begin(), bp.name.end(), '_', ' ');
                squad.push_back(bp);
            }
        }
    }
    return true;
}
