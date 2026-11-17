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
          { {105,105},{195,105},{195,165},{115,165},{100,135} }, {145,132}, { "OR","ID" } },
        { "OR", "Oregon",         Faction::SCAV_SYNDICATE, 140, "Sk-Mountain",
          { {115,165},{195,165},{195,235},{150,235},{95,235} }, {140,196}, { "WA","ID","NV","CA" } },
        { "CA", "California",     Faction::THE_PROJECT,    260, "Sk-Startup",
          { {95,235},{150,235},{150,300},{195,375},{180,415},{130,390},{100,310} }, {128,322}, { "OR","NV","AZ" } },
        { "NV", "Nevada",         Faction::THE_PROJECT,    130, "Sk-SandCastles",
          { {150,235},{195,235},{215,235},{215,335},{195,375},{150,300} }, {176,282}, { "OR","ID","UT","AZ","CA" } },
        { "ID", "Idaho",          Faction::SCAV_SYNDICATE, 120, "Sk-HighGround",
          { {195,105},{225,105},{235,180},{265,180},{265,235},{215,235},{195,235},{195,165} }, {218,195}, { "WA","OR","NV","UT","WY","MT" } },
        { "MT", "Montana",        Faction::SCAV_SYNDICATE, 140, "Sk-HighGround",
          { {225,105},{365,105},{365,165},{365,180},{265,180},{235,180} }, {290,138}, { "ID","WY","ND","SD" } },
        { "WY", "Wyoming",        Faction::NEW_PARADIGM,   120, "Sk-BeggarsKanyon",
          { {265,180},{365,180},{365,225},{365,250},{280,250},{265,250},{265,235} }, {308,210}, { "MT","ID","UT","CO","NE","SD" } },
        { "UT", "Utah",           Faction::NEW_PARADIGM,   135, "Sk-Valley",
          { {215,235},{265,235},{265,250},{280,250},{280,335},{215,335} }, {240,285}, { "ID","WY","CO","AZ","NV" } },
        { "AZ", "Arizona",        Faction::NEW_PARADIGM,   165, "Sk-LittleEgypt",
          { {215,335},{280,335},{280,425},{215,425},{180,415},{195,375} }, {234,375}, { "CA","NV","UT","NM" } },
        { "CO", "Colorado",       Faction::NEW_PARADIGM,   175, "Sk-GreatRift",
          { {280,250},{365,250},{380,250},{380,280},{380,335},{365,335},{280,335} }, {322,288}, { "WY","NE","KS","OK","NM","UT" } },
        { "NM", "New Mexico",     Faction::NEW_PARADIGM,   145, "Sk-LittleEgypt",
          { {280,335},{365,335},{365,365},{365,425},{280,425} }, {315,375}, { "AZ","CO","OK","TX" } },
        { "ND", "North Dakota",   Faction::THE_COLLECTIVE, 125, "Sk-RollingHills",
          { {365,105},{465,105},{465,165},{365,165} }, {408,130}, { "MT","SD","MN" } },
        { "SD", "South Dakota",   Faction::THE_COLLECTIVE, 125, "Sk-RollingHills",
          { {365,165},{465,165},{465,205},{465,225},{365,225},{365,180} }, {408,190}, { "ND","MT","WY","NE","IA","MN" } },
        { "NE", "Nebraska",       Faction::THE_COLLECTIVE, 140, "Sk-BeggarsKanyon",
          { {365,225},{465,225},{475,225},{485,270},{485,280},{380,280},{380,250},{365,250} }, {418,248}, { "SD","WY","CO","KS","MO","IA" } },
        { "KS", "Kansas",         Faction::THE_COLLECTIVE, 150, "Sk-Clover",
          { {380,280},{485,280},{495,280},{495,335},{380,335} }, {430,305}, { "NE","CO","OK","MO" } },
        { "OK", "Oklahoma",       Faction::NEW_PARADIGM,   155, "Sk-BeggarsKanyon",
          { {365,335},{380,335},{495,335},{505,335},{505,345},{505,400},{405,400},{405,365},{365,365} }, {438,365}, { "KS","CO","NM","TX","AR","MO" } },
        { "TX", "Texas",          Faction::NEW_PARADIGM,   260, "Sk-GreatRift",
          { {365,365},{405,365},{405,400},{505,400},{515,400},{515,415},{520,470},{520,480},{465,530},{425,490},{385,480},{355,425},{365,425} }, {442,442}, { "NM","OK","AR","LA" } },
        { "MN", "Minnesota",      Faction::THE_COLLECTIVE, 165, "Sk-Valley",
          { {465,105},{545,105},{545,135},{535,205},{465,205},{465,165} }, {495,150}, { "ND","SD","IA","WI" } },
        { "IA", "Iowa",           Faction::THE_COLLECTIVE, 150, "Sk-Clover",
          { {465,205},{535,205},{545,215},{555,270},{485,270},{475,225},{465,225} }, {502,232}, { "MN","SD","NE","MO","IL","WI" } },
        { "MO", "Missouri",       Faction::THE_COLLECTIVE, 170, "Sk-Ziggurat",
          { {485,270},{555,270},{560,270},{568,305},{572,330},{575,345},{505,345},{505,335},{495,335},{495,280},{485,280} }, {522,302}, { "IA","NE","KS","OK","AR","TN","KY","IL" } },
        { "AR", "Arkansas",       Faction::THE_COLLECTIVE, 140, "Sk-LittleEgypt",
          { {505,345},{575,345},{568,370},{575,415},{515,415},{515,400},{505,400} }, {532,375}, { "MO","OK","TX","LA","MS","TN" } },
        { "LA", "Louisiana",      Faction::THE_COLLECTIVE, 160, "Sk-SandCastles",
          { {515,415},{575,415},{575,450},{595,480},{520,480},{520,470} }, {544,444}, { "AR","TX","MS" } },
        { "WI", "Wisconsin",      Faction::THE_COLLECTIVE, 165, "Sk-RollingHills",
          { {545,135},{605,145},{600,215},{545,215},{535,205} }, {566,172}, { "MN","IA","IL","MI" } },
        { "IL", "Illinois",       Faction::THE_COLLECTIVE, 210, "Sk-Manhattan",
          { {545,215},{600,215},{600,220},{605,295},{568,305},{560,270},{555,270} }, {570,255}, { "WI","IA","MO","KY","IN" } },
        { "MI", "Michigan",       Faction::NEXUS_REMNANT,  195, "Sk-Concrete",
          { {605,145},{670,150},{675,220},{645,220},{600,220},{600,215} }, {638,178}, { "WI","IN","OH" } },
        { "IN", "Indiana",        Faction::NEXUS_REMNANT,  165, "Sk-Rush",
          { {600,220},{645,220},{645,285},{645,295},{605,295} }, {616,252}, { "MI","IL","KY","OH" } },
        { "OH", "Ohio",           Faction::NEXUS_REMNANT,  195, "Sk-Concrete",
          { {645,220},{675,220},{705,215},{705,265},{675,270},{675,285},{645,285} }, {666,248}, { "MI","IN","KY","WV","PA" } },
        { "KY", "Kentucky",       Faction::THE_COLLECTIVE, 155, "Sk-Mountain",
          { {568,305},{605,295},{645,295},{645,285},{675,285},{680,310},{685,330},{572,330} }, {625,308}, { "IL","IN","OH","WV","VA","TN","MO" } },
        { "TN", "Tennessee",      Faction::THE_COLLECTIVE, 165, "Sk-Valley",
          { {572,330},{685,330},{675,370},{665,370},{615,370},{568,370},{575,345} }, {618,345}, { "KY","VA","NC","GA","AL","MS","AR","MO" } },
        { "MS", "Mississippi",    Faction::THE_COLLECTIVE, 140, "Sk-LittleEgypt",
          { {568,370},{615,370},{615,450},{575,450},{575,415} }, {584,405}, { "TN","AL","LA","AR" } },
        { "AL", "Alabama",        Faction::THE_COLLECTIVE, 155, "Sk-Ziggurat",
          { {615,370},{665,370},{665,450},{645,450},{615,450} }, {632,405}, { "TN","GA","FL","MS" } },
        { "FL", "Florida",        Faction::NEW_PARADIGM,   210, "Sk-SandCastles",
          { {645,450},{665,450},{735,450},{750,540},{715,545},{685,475},{645,470} }, {702,492}, { "AL","GA" } },
        { "GA", "Georgia",        Faction::NEXUS_REMNANT,  185, "Sk-HighGround",
          { {665,370},{675,370},{715,370},{735,415},{735,450},{665,450} }, {690,405}, { "TN","NC","SC","FL","AL" } },
        { "SC", "South Carolina", Faction::NEXUS_REMNANT,  150, "Sk-SandCastles",
          { {715,370},{760,365},{735,415} }, {726,378}, { "NC","GA" } },
        { "NC", "North Carolina", Faction::NEXUS_REMNANT,  180, "Sk-HighGround",
          { {685,330},{785,330},{760,365},{715,370},{675,370} }, {734,344}, { "VA","TN","GA","SC" } },
        { "VA", "Virginia",       Faction::NEXUS_REMNANT,  185, "Sk-BeggarsKanyon",
          { {680,310},{695,285},{710,285},{778,290},{785,330},{685,330} }, {736,306}, { "WV","MD","NC","TN","KY" } },
        { "WV", "West Virginia",  Faction::NEXUS_REMNANT,  135, "Sk-Mountain",
          { {675,270},{705,265},{710,265},{710,285},{695,285},{680,310},{675,285} }, {684,284}, { "OH","PA","MD","VA","KY" } },
        { "MD", "Maryland",       Faction::NEXUS_REMNANT,  160, "Sk-Concrete",
          { {710,265},{778,265},{778,290},{710,285} }, {742,272}, { "PA","DE","VA","WV" } },
        { "DE", "Delaware",       Faction::NEXUS_REMNANT,  120, "Sk-Rush",
          { {778,260},{796,260},{796,290},{778,290},{778,265} }, {781,270}, { "PA","NJ","MD" } },
        { "PA", "Pennsylvania",   Faction::NEXUS_REMNANT,  210, "Sk-Concrete",
          { {705,215},{720,215},{778,215},{778,260},{778,265},{710,265},{705,265} }, {736,236}, { "NY","NJ","DE","MD","WV","OH" } },
        { "NJ", "New Jersey",     Faction::NEXUS_REMNANT,  175, "Sk-Manhattan",
          { {778,215},{795,215},{800,215},{800,260},{796,260},{778,260} }, {783,234}, { "NY","PA","DE" } },
        { "NY", "New York",       Faction::NEXUS_REMNANT,  240, "Sk-Manhattan",
          { {720,165},{788,165},{788,175},{792,175},{792,195},{795,195},{795,215},{778,215},{720,215} }, {754,186}, { "VT","MA","CT","NJ","PA" } },
        { "CT", "Connecticut",    Faction::NEXUS_REMNANT,  140, "Sk-Concrete",
          { {795,195},{820,195},{820,215},{800,215},{795,215} }, {801,201}, { "NY","MA","RI" } },
        { "RI", "Rhode Island",   Faction::NEXUS_REMNANT,  110, "Sk-Rush",
          { {820,195},{836,195},{836,215},{820,215} }, {822,201}, { "CT","MA" } },
        { "MA", "Massachusetts",  Faction::NEXUS_REMNANT,  175, "Sk-Concrete",
          { {792,175},{808,175},{828,175},{838,175},{838,195},{836,195},{820,195},{795,195},{792,195} }, {810,181}, { "NY","VT","NH","RI","CT" } },
        { "VT", "Vermont",        Faction::NEXUS_REMNANT,  120, "Sk-Mountain",
          { {788,135},{808,135},{808,175},{792,175},{788,175} }, {792,150}, { "NY","NH","MA" } },
        { "NH", "New Hampshire",  Faction::NEXUS_REMNANT,  125, "Sk-Mountain",
          { {808,135},{828,135},{828,170},{828,175},{808,175} }, {812,150}, { "VT","ME","MA" } },
        { "ME", "Maine",          Faction::NEXUS_REMNANT,  145, "Sk-HighGround",
          { {828,100},{872,115},{855,170},{828,170},{828,135} }, {844,132}, { "NH" } }
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
    int lift = isSelected ? 22 : 0;
    switch (f)
    {
    case UsaFaction::THE_PROJECT:
    {
        int idx = std::max(0, std::min(7, getUsaCampaignState().playerColorIndex));
        static const uint8_t pCols[8][3] = { {44,84,136}, {32,108,96}, {136,46,52}, {92,56,128}, {148,122,42}, {102,112,126}, {52,58,124}, {36,96,118} };
        return { static_cast<uint8_t>(std::min(255, pCols[idx][0] + lift)), static_cast<uint8_t>(std::min(255, pCols[idx][1] + lift)), static_cast<uint8_t>(std::min(255, pCols[idx][2] + lift)), 255 };
    }
    case UsaFaction::NEW_PARADIGM:     return { static_cast<uint8_t>(148 + lift), static_cast<uint8_t>(94 + lift),  static_cast<uint8_t>(38 + lift),  255 }; // Neo Paradigm Amber
    case UsaFaction::SCAV_PACIFIC:     return { static_cast<uint8_t>(48 + lift),  static_cast<uint8_t>(88 + lift),  static_cast<uint8_t>(74 + lift),  255 }; // Pacific Pine-Teal
    case UsaFaction::SCAV_ROCKIES:     return { static_cast<uint8_t>(88 + lift),  static_cast<uint8_t>(76 + lift),  static_cast<uint8_t>(62 + lift),  255 }; // Rockies Granite
    case UsaFaction::SCAV_DESERT:      return { static_cast<uint8_t>(112 + lift), static_cast<uint8_t>(74 + lift),  static_cast<uint8_t>(52 + lift),  255 }; // Desert Terracotta
    case UsaFaction::SCAV_BADLANDS:    return { static_cast<uint8_t>(92 + lift),  static_cast<uint8_t>(92 + lift),  static_cast<uint8_t>(54 + lift),  255 }; // Badlands Ochre-Olive
    case UsaFaction::SCAV_HEARTLAND:   return { static_cast<uint8_t>(62 + lift),  static_cast<uint8_t>(94 + lift),  static_cast<uint8_t>(56 + lift),  255 }; // Heartland Moss
    case UsaFaction::SCAV_LAKES:       return { static_cast<uint8_t>(52 + lift),  static_cast<uint8_t>(84 + lift),  static_cast<uint8_t>(94 + lift),  255 }; // Lakes Slate-Teal
    case UsaFaction::SCAV_OHIO:        return { static_cast<uint8_t>(84 + lift),  static_cast<uint8_t>(80 + lift),  static_cast<uint8_t>(64 + lift),  255 }; // Ohio Khaki-Olive
    case UsaFaction::SCAV_GULF:        return { static_cast<uint8_t>(46 + lift),  static_cast<uint8_t>(86 + lift),  static_cast<uint8_t>(64 + lift),  255 }; // Gulf Bayou-Green
    case UsaFaction::SCAV_PIEDMONT:    return { static_cast<uint8_t>(96 + lift),  static_cast<uint8_t>(66 + lift),  static_cast<uint8_t>(62 + lift),  255 }; // Piedmont Clay
    case UsaFaction::SCAV_ATLANTIC:    return { static_cast<uint8_t>(68 + lift),  static_cast<uint8_t>(72 + lift),  static_cast<uint8_t>(96 + lift),  255 }; // Atlantic Iron-Slate
    case UsaFaction::SCAV_NEW_ENGLAND: return { static_cast<uint8_t>(58 + lift),  static_cast<uint8_t>(88 + lift),  static_cast<uint8_t>(82 + lift),  255 }; // New England Sea-Sage
    case UsaFaction::SCAV_NORTHWOODS:  return { static_cast<uint8_t>(66 + lift),  static_cast<uint8_t>(82 + lift),  static_cast<uint8_t>(58 + lift),  255 }; // Northwoods Forest
    default:                           return { static_cast<uint8_t>(62 + lift),  static_cast<uint8_t>(84 + lift),  static_cast<uint8_t>(72 + lift),  255 };
    }
}

std::string getUsaFactionName(UsaFaction f)
{
    switch (f)
    {
    case UsaFaction::THE_PROJECT:      return getUsaCampaignState().playerFactionName.empty() ? "The Project" : getUsaCampaignState().playerFactionName;
    case UsaFaction::NEW_PARADIGM:     return "Neo Paradigm";
    case UsaFaction::SCAV_PACIFIC:     return "Pacific Scavs";
    case UsaFaction::SCAV_ROCKIES:     return "Rockies Scavs";
    case UsaFaction::SCAV_DESERT:      return "Desert Scavs";
    case UsaFaction::SCAV_BADLANDS:    return "Badlands Scavs";
    case UsaFaction::SCAV_HEARTLAND:   return "Heartland Scavs";
    case UsaFaction::SCAV_LAKES:       return "Lakes Scavs";
    case UsaFaction::SCAV_OHIO:        return "Ohio Scavs";
    case UsaFaction::SCAV_GULF:        return "Gulf Scavs";
    case UsaFaction::SCAV_PIEDMONT:    return "Piedmont Scavs";
    case UsaFaction::SCAV_ATLANTIC:    return "Atlantic Scavs";
    case UsaFaction::SCAV_NEW_ENGLAND: return "New England Scavs";
    case UsaFaction::SCAV_NORTHWOODS:  return "Northwoods Scavs";
    default:                           return "Scav Warlords";
    }
}

bool isUsaScavFaction(UsaFaction f) { return static_cast<int>(f) >= static_cast<int>(UsaFaction::SCAV_SYNDICATE); }

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
    playerFactionName = "The Project"; playerColorIndex = 0; campaignConfigured = false; turn = 1; power = 1200; selectedIndex = 2; activeTargetIndex = -1;
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
    for (auto& st : states) { const auto& c = st.code; if (c == "CA") st.owner = UsaFaction::THE_PROJECT; else if (c == "TX") st.owner = UsaFaction::NEW_PARADIGM; else if (c == "WA" || c == "OR" || c == "NV" || c == "ID") st.owner = UsaFaction::SCAV_PACIFIC; else if (c == "MT" || c == "WY" || c == "UT" || c == "CO") st.owner = UsaFaction::SCAV_ROCKIES; else if (c == "AZ" || c == "NM" || c == "OK" || c == "KS") st.owner = UsaFaction::SCAV_DESERT; else if (c == "ND" || c == "SD" || c == "NE" || c == "MN") st.owner = UsaFaction::SCAV_BADLANDS; else if (c == "IA" || c == "MO" || c == "AR" || c == "LA") st.owner = UsaFaction::SCAV_HEARTLAND; else if (c == "WI" || c == "IL" || c == "MI" || c == "IN") st.owner = UsaFaction::SCAV_LAKES; else if (c == "OH" || c == "KY" || c == "TN" || c == "WV") st.owner = UsaFaction::SCAV_OHIO; else if (c == "MS" || c == "AL" || c == "GA" || c == "FL") st.owner = UsaFaction::SCAV_GULF; else if (c == "SC" || c == "NC" || c == "VA" || c == "MD") st.owner = UsaFaction::SCAV_PIEDMONT; else if (c == "PA" || c == "NY" || c == "NJ" || c == "DE") st.owner = UsaFaction::SCAV_ATLANTIC; else if (c == "CT" || c == "RI" || c == "MA" || c == "VT") st.owner = UsaFaction::SCAV_NEW_ENGLAND; else st.owner = UsaFaction::SCAV_NORTHWOODS; }
}
bool UsaCampaignState::saveToPhysfs() const
{
    PHYSFS_mkdir("usa_campaign");
    std::ostringstream out;
    std::string safeFName = playerFactionName; for (char& c : safeFName) if (c == ' ') c = '_';
    out << "PLAYER_FACTION " << safeFName << " " << playerColorIndex << "\n";
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
    out << "GENERAL_ACTIVE " << (general.bodyId.empty() ? "NONE" : general.bodyId) << " " << (general.propId.empty() ? "NONE" : general.propId) << " " << (general.weapon1Id.empty() ? "NONE" : general.weapon1Id) << " " << (general.weapon2Id.empty() ? "NONE" : general.weapon2Id) << "\n";
    for (const auto& gv : general.variants)
    {
        std::string safeName = gv.name; for (char& c : safeName) if (c == ' ') c = '_';
        out << "GENERAL_VARIANT " << safeName << " " << gv.bodyId << " " << gv.propId << " " << gv.weapon1Id << " " << gv.weapon2Id << "\n";
    }
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

    squad.clear(); general.variants.clear();
    std::istringstream in(content);
    std::string tag;
    while (in >> tag)
    {
        if (tag == "PLAYER_FACTION") { std::string fn; int ci = 0; in >> fn >> ci; for (char& c : fn) if (c == '_') c = ' '; playerFactionName = fn; playerColorIndex = std::max(0, std::min(7, ci)); campaignConfigured = true; }
        else if (tag == "TURN") { in >> turn; campaignConfigured = true; }
        else if (tag == "POWER") in >> power;
        else if (tag == "GENERAL_ACTIVE")
        {
            std::string b, p, w1, w2; in >> b >> p >> w1 >> w2;
            general.bodyId = (b == "NONE") ? "" : b; general.propId = (p == "NONE") ? "" : p;
            general.weapon1Id = (w1 == "NONE") ? "" : w1; general.weapon2Id = (w2 == "NONE") ? "" : w2;
        }
        else if (tag == "GENERAL_VARIANT")
        {
            std::string nm, b, p, w1, w2; in >> nm >> b >> p >> w1 >> w2;
            for (char& c : nm) if (c == '_') c = ' ';
            general.variants.push_back({ nm, b, p, w1, w2 });
        }

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
