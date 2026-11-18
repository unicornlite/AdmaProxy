#include "Player.hpp"
#include "../Packets/Packets.hpp"
#include "../Network/Handler.hpp"
#include "../Network/Network.hpp"
#include "../Items/Items.hpp"
#include "../Network/NET_MESSAGE_GAME_PACKET/PACKET_SEND_MAP_DATA.hpp"

#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <queue>

PlayerState Player;

// Batch 2 item ids, verified against items.dat.
constexpr int ITEM_WIGGLY_WORM = 2914;
constexpr int ITEM_HAND_DRILL = 5522;
constexpr int ITEM_DETONATOR = 5524;
constexpr int ITEM_ICE_CRUST = 5602;
constexpr int ITEM_URANIUM = 4658;

constexpr int SURG_SPONGE = 1258;
constexpr int SURG_SCALPEL = 1260;
constexpr int SURG_ANESTHETIC = 1262;
constexpr int SURG_ANTISEPTIC = 1264;
constexpr int SURG_ANTIBIOTICS = 1266;
constexpr int SURG_SPLINT = 1268;
constexpr int SURG_STITCHES = 1270;
constexpr int SURG_GLOVES = 1296;
constexpr int SURG_PINS = 4308;
constexpr int SURG_TRANSFUSION = 4310;
constexpr int SURG_DEFIB = 4312;
constexpr int SURG_ULTRASOUND = 4316;
constexpr int SURG_LABKIT = 4318;

// ESP marker blocks: Yellow Block for drops, Aqua Block for players.
constexpr uint16_t ESP_DROP_BLOCK = 174;
constexpr uint16_t ESP_PLAYER_BLOCK = 178;

// Water flag bit in the tile flags (GTProxy TileFlag::Water).
constexpr uint16_t TILE_FLAG_WATER = 1 << 10;

// Long punch reach in tiles, measured from the player.
constexpr int LONG_PUNCH_REACH = 10;

// Noclip nudge per outgoing STATE packet while pushing against a wall.
constexpr float NOCLIP_STEP = 8.0f;

// Batch 3: Chandelier (verified id 340) breaks drop Gems (item 112).
constexpr int ITEM_CHANDELIER = 340;
constexpr int ITEM_GEMS = 112;

// Blackjack report window: gems land within a couple of seconds of the break.
constexpr int BJ_WINDOW_MS = 2500;
constexpr size_t BJ_HISTORY_MAX = 10;

inline uint64_t EspKey(int x, int y) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y);
}

inline std::string EspDirection(float dx, float dy) {
    if (std::abs(dx) >= std::abs(dy)) return dx >= 0.0f ? "right" : "left";
    return dy >= 0.0f ? "below" : "above";
}


void PlayerState::Reset() {
    userID = -1;

    tankIDName.clear();
    player_age = 0;
    klv.clear();
    meta.clear();
    rid.clear();
    mac.clear();
    wk.clear();

    netID = -1;
    country.clear();
    world.clear();
    lastWorld.clear();
    saveWorld.clear();
    worldPlayers.clear();

    dropMode = false;
    trashMode = false;
    wrenchMode = false;
    autoChange = false;
    hideClothes = false;
    antiMod = false;
    antiGravity = false;
    hoverY = -1.0f;
    unlimitedZoom = false;
    doorMark = false;
    lastLocalSpawn.clear();
    lastWorldFile.clear();
    lastSongFile.clear();

    ClearWorld();

    autocollect.running = false;
    autocollect.targetX = 0.0f;
    autocollect.targetY = 0.0f;
    autocollect.targetUntil = {};
    autocollect.attempts.clear();

    teleport.x = 0.0f;
    teleport.y = 0.0f;
    teleport.until = {};
    teleport.savedX = 0.0f;
    teleport.savedY = 0.0f;
    teleport.hasSaved = false;

    paste.running = false;

    autoSurgery = false;
    surgFindFix = false;
    autoFish = false;
    fishing.generation++;
    speedEnabled = false;
    speedFactor = 1.5f;
    speed.synced = false;
    longPunch = false;
    noclip = false;
    noclipState.synced = false;
    esp = false;
    xray = false;
    spinInfo = false;
    blackjack = false;
    pathfind.running = false;
    pathfind.destX = -1;
    pathfind.destY = -1;

    position.x = 0.0f;
    position.y = 0.0f;

    clothes.hair_shirt_pants = { 0,0,0 };
    clothes.feet_face_hand = { 0,0,0 };
    clothes.back_mask_necklace = { 0,0,0 };
    clothes.skinColor = 31;
    clothes.ances_unknown_unknown2 = { 0,0,0 };

    inventory.items.clear();
    inventory.size = 16;

    {
        std::lock_guard<std::mutex> lock(dialogSkipper.Skipper);
        dialogSkipper.DROP.clear();
        dialogSkipper.TRASH = false;
        dialogSkipper.SALES_MAN = false;
    }

    design.OnDialogRequest_RED = 0;
    design.OnDialogRequest_GREEN = 0;
    design.OnDialogRequest_BLUE = 0;
    design.OnDialogRequest_ALPHA = 160;

    autofarm.running = false;
    autofarm.id = -1;
    autofarm.X = -1.0f;
    autofarm.Y = -1.0f;
    autofarm.punchX = -1;
    autofarm.punchY = -1;

    autospam.running = false;
    autospam.text1 = "Hello World!";
    autospam.text2.clear();
    autospam.delay = 3600;
    autospam.rainbow = false;
}

// ---------------- Inventory ----------------
int PlayerState::Inventory::GetCount(int id) const {
    auto it = items.find(id);
    if (it != items.end()) return it->second;
    return 0;
}

bool PlayerState::Inventory::HasItem(int id) const {
    return items.find(id) != items.end();
}

void PlayerState::Inventory::AddItem(int id, int count) {
    if (count <= 0) return;
    auto it = items.find(id);
    if (it != items.end()) {
        int newCount = it->second + count;
        if (newCount > MAX_STACK) newCount = MAX_STACK;
        it->second = newCount;
        return;
    }
    if (items.size() >= static_cast<size_t>(size)) return;
    if (count > MAX_STACK) count = MAX_STACK;
    items[id] = count;
}

void PlayerState::Inventory::RemoveItem(int id, int count) {
    if (count <= 0) return;
    auto it = items.find(id);
    if (it == items.end()) return;
    int newCount = it->second - count;
    if (newCount > 200) newCount = 200;
    if (newCount <= 0) items.erase(it);
    else it->second = newCount;
}

void PlayerState::Inventory::ResetInventory(int newSize, const std::vector<std::pair<int, int>>& itemsVec) {
    items.clear();
    size = newSize;
    for (const auto& [id, count] : itemsVec) AddItem(id, count);
}


// ---------------- PlayerState public methods ----------------
std::string PlayerState::InventoryToString() const {
    std::string text;
    int buttonCount = 0;

    for (const auto& pair : inventory.items) {
        uint16_t id = pair.first;
        int amount = pair.second;

        auto it = std::find_if(items.begin(), items.end(), [id](const Items& i) { return i.id == id; });

        if (it != items.end()) {
            text += "text_scaling_string|The Best World Lock Sow!|\n";
            text += "add_button_with_icon||" + it->raw_name + "|staticBlueFrame|" + std::to_string(id) + "|" + std::to_string(amount) + "|\n";
            buttonCount++;
            if (buttonCount % 5 == 0) text += "add_button_with_icon||END_LIST|noflags|0|0|\n";
        }
    }

    return text;
}

int PlayerState::GetItemCount(int id) const {
    return inventory.GetCount(id);
}

bool PlayerState::HasItem(int id) const {
    return inventory.HasItem(id);
}

void PlayerState::AddItem(int id, int count) {
    inventory.AddItem(id, count);
}

void PlayerState::RemoveItem(int id, int count) {
    inventory.RemoveItem(id, count);
}

void PlayerState::ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items) {
    inventory.ResetInventory(newSize, items);
}



// ---------------- Functions ----------------
void PlayerState::Warp(std::string worldName, ENetPeer* CLIENT, ENetPeer* SERVER) {
    std::string newWorld = worldName;
    std::transform(newWorld.begin(), newWorld.end(), newWorld.begin(), [](unsigned char c) { return std::toupper(c); });

    if (newWorld == "EXIT") {
        GamePacket<OnConsoleMessage> p;
        p.text = "Exit from what? Press back if you're done playing.";
        SendGamePacket(CLIENT, p);
        return;
    }
    for (char c : newWorld) if ((c < 'A' || c>'Z') && (c < '0' || c>'9')) {
        GamePacket<OnConsoleMessage> p;
        p.text = "Sorry, spaces and special characters are not allowed in world or door names.  Try again.";
        SendGamePacket(CLIENT, p);
        return;
    }
    if (newWorld.size() < 1 || newWorld.size() >= 24) {
        GamePacket<OnConsoleMessage> p;
        p.text = "`4To reduce confusion, that is not a valid world name.``  Try another?";
        SendGamePacket(CLIENT, p);
        return;
    }
    if (newWorld == "") newWorld = "START";

    GamePacket<OnConsoleMessage> p;
    p.text = "Warping to the world [" + newWorld + "]. This may take up to 2 seconds!";
    SendGamePacket(CLIENT, p);

    std::thread([newWorld = std::move(newWorld), SERVER]() {
        if (!Player.world.empty()) {
            std::string quitPacket = "action|quit_to_exit\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GAME_MESSAGE, quitPacket.c_str(), static_cast<int>(quitPacket.length()));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
        std::string joinPacket = "action|join_request\nname|" + newWorld + "\ninvitedWorld|0\n\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GAME_MESSAGE, joinPacket.c_str(), static_cast<int>(joinPacket.length()));
        }
    }).detach();
}

void PlayerState::LeaveWorld(ENetPeer* CLIENT, ENetPeer* SERVER, const std::string& modName) {
    /* @info: Anti-Mod emergency exit. Sends quit_to_exit straight away (no 2s wait like Warp),
       stops any running automation and clears the world state locally. The world-empty
       check also guards against firing twice for the same world. */
    if (world.empty()) return;

    std::string text = "Moderator [" + modName + "] entered the world! Leaving immediately!";
    LOG_WARN("{}", text);

    {
        GamePacket<OnConsoleMessage> p;
        p.text = text;
        SendGamePacket(CLIENT, p);
    }

    {
        GamePacket<OnTalkBubble> p;
        p.netId = netID;
        p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
        SendGamePacket(CLIENT, p);
    }

    if (autofarm.running) {
        autofarm.running = false;
        autofarm.id = -1;
        autofarm.X = -1.0f;
        autofarm.Y = -1.0f;
        autofarm.punchX = -1;
        autofarm.punchY = -1;
    }
    autospam.running = false;

    if (SERVER && SERVER->state == ENET_PEER_STATE_CONNECTED) {
        std::string quitPacket = "action|quit_to_exit\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GAME_MESSAGE, quitPacket.c_str(), static_cast<int>(quitPacket.length()));
        }
    }

    lastWorld = world;
    world.clear();
    worldPlayers.clear();
}

void PlayerState::ApplyAntiGravity(ENetPacket* packet, float currentY) {
    /* @info: Anti-Gravity. Locks the Y sent to the server at the hover height
       and zeroes vertical speed, so the player floats instead of falling.
       The packet is edited in place and forwarded as usual. Y is at payload
       offset 0x1C, Y speed at 0x24, both behind the 4-byte packet type. */
    if (!antiGravity || !packet || packet->dataLength < 44) return;
    if (hoverY <= 0.0f) hoverY = currentY;
    float zero = 0.0f;
    std::memcpy(packet->data + 4 + 0x1C, &hoverY, sizeof(float));
    std::memcpy(packet->data + 4 + 0x24, &zero, sizeof(float));
}

void PlayerState::Drop(int id, int count, ENetPeer* CLIENT, ENetPeer* SERVER) {
    auto it = std::find_if(items.begin(), items.end(), [&id](const Items& i) { return i.id == id; });
    if (it != items.end()) {
        if (HasItem(id) && GetItemCount(id) >= count) {
            std::string firstPacket = "action|drop\n|itemID|" + std::to_string(id) + "\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, firstPacket.c_str(), static_cast<int>(firstPacket.length()));
            }

            {
                std::lock_guard<std::mutex> lock(Player.dialogSkipper.Skipper);
                dialogSkipper.DROP.push_back(it->raw_name);
            }

            std::string secondPacket = "action|dialog_return\ndialog_name|drop_item\nitemID|" + std::to_string(id) + "|\ncount|" + std::to_string(count) + "\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, secondPacket.c_str(), static_cast<int>(secondPacket.length()));
            }

            {
                GamePacket<OnConsoleMessage> p;
                p.text = "Dropping " + std::to_string(count) + " " + it->raw_name;
                SendGamePacket(CLIENT, p);
            }

            return;
        }
        else LOG_DEBUG("You don't have {}x {}", count, it->raw_name);
    }
    else LOG_ERROR("Item ID: {} couldn't find in the database!", id);
}

void PlayerState::Trash(int id, int count, ENetPeer* CLIENT, ENetPeer* SERVER) {
    auto it = std::find_if(items.begin(), items.end(), [&id](const Items& i) { return i.id == id; });
    if (it != items.end()) {
        if (HasItem(id) && GetItemCount(id) >= count) {
            std::string firstPacket = "action|trash\n|itemID|" + std::to_string(id) + "\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, firstPacket.c_str(), static_cast<int>(firstPacket.length()));
            }

            {
                std::lock_guard<std::mutex> lock(Player.dialogSkipper.Skipper);
                dialogSkipper.TRASH = true;
            }

            std::string secondPacket = "action|dialog_return\ndialog_name|trash_item\nitemID|" + std::to_string(id) + "|\ncount|" + std::to_string(count) + "\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, secondPacket.c_str(), static_cast<int>(secondPacket.length()));
            }

            {
                GamePacket<OnConsoleMessage> p;
                p.text = "Trashing " + std::to_string(count) + " " + it->raw_name;
                SendGamePacket(CLIENT, p);
            }

            return;
        }
        else LOG_DEBUG("You don't have {}x {}", count, it->raw_name);
    }
    else LOG_ERROR("Item ID: {} couldn't find in the database!", id);
}

void PlayerState::Punch(float X, float Y, int punchX, int punchY, ENetPeer* SERVER) {
    /* @debug
        [06:03:2026 17:05:26] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_TILE_CHANGE_REQUEST:3]
        PlantingTree:18|X:1772.00|Y:578.00|PunchX:56|PunchY:18|

        [06:03:2026 17:05:26] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_STATE:0]
        CharacterState:2592|X:1772.00|Y:578.00|PunchX:56|PunchY:18|
    */

    PlayerMovingPacket firstPunch;
    firstPunch
        .PacketType(Network.PACKET_TILE_CHANGE_REQUEST)
        .PlantingTree(18)
        .Position(X, Y)
        .Punch(punchX, punchY)
        .Send(SERVER);

    PlayerMovingPacket secondPunch;
    secondPunch
        .PacketType(Network.PACKET_STATE)
        .CharacterState(2592)
        .Position(X, Y)
        .Punch(punchX, punchY)
        .Send(SERVER);
}

void PlayerState::Place(int id, float X, float Y, int punchX, int punchY, ENetPeer* SERVER) {
    /* @debug
    * Place
        [06:03:2026 16:19:46] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_TILE_CHANGE_REQUEST:3]
        PlantingTree:2|X:709.00|Y:738.00|PunchX:23|PunchY:23|

        [06:03:2026 16:19:46] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_STATE:0]
        CharacterState:3104|PlantingTree:2|X:709.00|Y:738.00|PunchX:23|PunchY:23|
    */

    PlayerMovingPacket firstPlace;
    firstPlace
        .PacketType(Network.PACKET_TILE_CHANGE_REQUEST)
        .PlantingTree(id)
        .Position(X, Y)
        .Punch(punchX, punchY)
        .Send(SERVER);

    PlayerMovingPacket secondPlace;
    secondPlace
        .PacketType(Network.PACKET_STATE)
        .CharacterState(3104)
        .PlantingTree(id)
        .Position(X, Y)
        .Punch(punchX, punchY)
        .Send(SERVER);
}

bool PlayerState::AutofarmReady() {
    return autofarm.id > 0 &&
        autofarm.X > 0 &&
        autofarm.Y > 0 &&
        autofarm.punchX > 0 &&
        autofarm.punchY > 0;
}

void PlayerState::AutoFarm(ENetPeer* CLIENT, ENetPeer* SERVER) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> punchDelay(200, 250);
    std::uniform_int_distribution<int> placeDelay(520, 570);

    while (autofarm.running) {
        if (AutofarmReady() && SERVER) {
            for (int i = 0; i < 3; i++) {
                {
                    std::lock_guard<std::mutex> lock(Network.peer_mutex);
                    Punch(autofarm.X, autofarm.Y, autofarm.punchX, autofarm.punchY, SERVER);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(punchDelay(gen)));
            }
            
            if (HasItem(autofarm.id)) {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                Place(autofarm.id, autofarm.X, autofarm.Y, autofarm.punchX, autofarm.punchY, SERVER);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(placeDelay(gen)));
        }
        else std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void PlayerState::AutoSpam(ENetPeer* CLIENT, ENetPeer* SERVER) {
    static const std::vector<std::string> colors = {
        "`0","`1","`2","`3","`4","`5","`6","`7","`8","`9",
        "`b","`c","`e","`g","`j","`o","`p","`s","`w"
    };

    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dis(0, colors.size() - 1);

    while (autospam.running) {
        std::string text1 = "action|input\n|text|" + (autospam.rainbow ? colors[dis(gen)] : "") + autospam.text1 + "\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, text1.c_str(), static_cast<int>(text1.length()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(autospam.delay));


        std::string text2 = "action|input\n|text|" + (autospam.rainbow ? colors[dis(gen)] : "") + (autospam.text2.empty() ? autospam.text1 : autospam.text2) + "\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, text2.c_str(), static_cast<int>(text2.length()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(autospam.delay));
    }
}

// ---------------- World state ----------------
void PlayerState::ClearWorld() {
    std::lock_guard<std::mutex> lock(worldMutex);
    worldWidth = 0;
    worldHeight = 0;
    worldTiles.clear();
    worldDrops.clear();
    playerPositions.clear();
    lastMapPacket.clear();
    pathfind.running = false;
    pathfind.destX = -1;
    pathfind.destY = -1;
    pathfind.targetX = 0.0f;
    pathfind.targetY = 0.0f;
    pathfind.targetUntil = {};
    espMarkers.clear();
    espDropTiles.clear();
    espPlayerTiles.clear();
    bjEvents.clear();
    bjDropAmounts.clear();
    bjDropEvent.clear();
    speed.synced = false;
    noclipState.synced = false;
    autocollect.targetX = 0.0f;
    autocollect.targetY = 0.0f;
    autocollect.targetUntil = {};
    autocollect.attempts.clear();
    teleport.x = 0.0f;
    teleport.y = 0.0f;
    teleport.until = {};
}

void PlayerState::ApplyAutoCollect(ENetPacket* packet) {
    /* @info: Auto-Collect. While the worker's target window is open, the
       outgoing position is rewritten to the drop's position so the player
       stands on it. With anti-gravity on, only X is overridden and Y stays
       locked at the hover height. Same in-place edit pattern as
       ApplyAntiGravity. */
    if (!autocollect.running || !packet || packet->dataLength < 44) return;
    std::lock_guard<std::mutex> lock(worldMutex);
    if (std::chrono::steady_clock::now() >= autocollect.targetUntil) return;
    std::memcpy(packet->data + 4 + 0x18, &autocollect.targetX, sizeof(float));
    if (!antiGravity) {
        std::memcpy(packet->data + 4 + 0x1C, &autocollect.targetY, sizeof(float));
        float zero = 0.0f;
        std::memcpy(packet->data + 4 + 0x24, &zero, sizeof(float));
    }
}

void PlayerState::ApplyTeleport(ENetPacket* packet) {
    /* @info: Teleport. While the burst window is open, outgoing STATE packets
       carry the target position instead of the real one, and speed is zeroed
       so the player does not drift away. Applied after anti-gravity and
       auto-collect, so the teleport wins while it lasts. */
    if (!packet || packet->dataLength < 44) return;
    std::lock_guard<std::mutex> lock(worldMutex);
    if (std::chrono::steady_clock::now() >= teleport.until) return;
    std::memcpy(packet->data + 4 + 0x18, &teleport.x, sizeof(float));
    std::memcpy(packet->data + 4 + 0x1C, &teleport.y, sizeof(float));
    float zero = 0.0f;
    std::memcpy(packet->data + 4 + 0x20, &zero, sizeof(float));
    std::memcpy(packet->data + 4 + 0x24, &zero, sizeof(float));
}

void PlayerState::TeleportTo(float x, float y, ENetPeer* SERVER) {
    /* @info: Teleport. The override window rewrites the client's own outgoing
       STATE packets, and the burst below sends our own STATE packets straight
       to the server, so the move lands even while the client is idle. After
       the window the server echoes the new position back to the client. */
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        teleport.x = x;
        teleport.y = y;
        teleport.until = std::chrono::steady_clock::now() + std::chrono::milliseconds(1200);
    }

    std::thread([x, y, SERVER]() {
        for (int i = 0; i < 6; i++) {
            if (SERVER && SERVER->state == ENET_PEER_STATE_CONNECTED) {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                PlayerMovingPacket pkt;
                pkt.PacketType(Network.PACKET_STATE)
                    .NetID(Player.netID)
                    .Position(x, y)
                    .Punch(-1, -1)
                    .Send(SERVER);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }).detach();
}

void PlayerState::CollectItem(uint32_t uid, ENetPeer* SERVER) {
    /* @info: ITEM_ACTIVATE_OBJECT_REQUEST (type 11). The drop UID goes in the
       0x14 field (PlantingTree slot); the server looks the drop up by it. */
    if (!SERVER || SERVER->state != ENET_PEER_STATE_CONNECTED) return;
    std::lock_guard<std::mutex> lock(Network.peer_mutex);
    PlayerMovingPacket pkt;
    pkt.PacketType(Network.PACKET_ITEM_ACTIVATE_OBJECT_REQUEST)
        .NetID(netID)
        .PlantingTree(static_cast<int>(uid))
        .Position(position.x, position.y)
        .Punch(-1, -1)
        .Send(SERVER);
}

void PlayerState::AutoCollectLoop(ENetPeer* CLIENT, ENetPeer* SERVER) {
    while (autocollect.running) {
        WorldDrop target{};
        bool found = false;
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            auto now = std::chrono::steady_clock::now();
            float best = 0.0f;
            for (const auto& drop : worldDrops) {
                auto it = autocollect.attempts.find(drop.uid);
                if (it != autocollect.attempts.end() && now - it->second < std::chrono::seconds(3)) continue;
                float dx = drop.x - position.x, dy = drop.y - position.y;
                float dist = dx * dx + dy * dy;
                if (!found || dist < best) {
                    best = dist;
                    target = drop;
                    found = true;
                }
            }
            if (found) {
                autocollect.targetX = target.x;
                autocollect.targetY = target.y;
                autocollect.targetUntil = now + std::chrono::milliseconds(450);
            }
        }

        if (!found) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        /* @info: Let the position override reach the server first, then ask
           for the drop. The client would collect it on its own once it sees
           the player standing on it, but the direct request is faster and
           works even if the server never echoes the position back. */
        std::this_thread::sleep_for(std::chrono::milliseconds(350));
        if (!autocollect.running) break;
        CollectItem(target.uid, SERVER);
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            auto now = std::chrono::steady_clock::now();
            autocollect.attempts[target.uid] = now;
            for (auto it = autocollect.attempts.begin(); it != autocollect.attempts.end();) {
                if (now - it->second > std::chrono::seconds(5)) it = autocollect.attempts.erase(it);
                else ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}


// ---------------- Copy world ----------------
bool PlayerState::SaveWorldLayout(const std::string& path, int& outTiles) {
    outTiles = 0;
    std::vector<WorldTile> tiles;
    int w = 0, h = 0;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        if (worldTiles.empty() || worldWidth <= 0 || worldHeight <= 0) return false;
        tiles = worldTiles;
        w = worldWidth;
        h = worldHeight;
    }

    std::ofstream out(path);
    if (!out) return false;
    out << "ADMAPROXY-WORLD-V1\n";
    out << w << ' ' << h << '\n';
    for (int i = 0; i < static_cast<int>(tiles.size()); i++) {
        if (tiles[i].fg == 0 && tiles[i].bg == 0) continue;
        int x = i % w, y = i / w;
        out << x << ' ' << y << ' ' << tiles[i].fg << ' ' << tiles[i].bg << '\n';
        outTiles++;
    }
    return true;
}

bool PlayerState::LoadWorldLayout(const std::string& path, std::vector<WorldPlacement>& out) {
    out.clear();
    std::ifstream in(path);
    if (!in) return false;
    std::string header;
    if (!std::getline(in, header) || header != "ADMAPROXY-WORLD-V1") return false;
    std::string line;
    if (!std::getline(in, line)) return false;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        WorldPlacement p{};
        if (!(ss >> p.x >> p.y >> p.fg >> p.bg)) continue;
        if (p.x < 0 || p.y < 0 || p.fg < 0 || p.bg < 0) continue;
        out.push_back(p);
    }
    return !out.empty();
}

void PlayerState::PasteWorld(const std::string& path, ENetPeer* CLIENT, ENetPeer* SERVER) {
    std::vector<WorldPlacement> placements;
    if (!LoadWorldLayout(path, placements)) {
        GamePacket<OnConsoleMessage> p;
        p.text = "Paste failed: could not read the world file.";
        SendGamePacket(CLIENT, p);
        paste.running = false;
        return;
    }

    int placed = 0, missing = 0;
    std::vector<int> missingIds;
    for (const auto& tile : placements) {
        if (!paste.running) break;
        float px = position.x > 0.0f ? position.x : static_cast<float>(tile.x * 32 + 16);
        float py = position.y > 0.0f ? position.y : static_cast<float>(tile.y * 32 + 16);
        for (int pass = 0; pass < 2; pass++) {
            int id = pass == 0 ? tile.fg : tile.bg;
            if (id <= 0) continue;
            if (!HasItem(id)) {
                missing++;
                if (std::find(missingIds.begin(), missingIds.end(), id) == missingIds.end()) missingIds.push_back(id);
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                Place(id, px, py, tile.x, tile.y, SERVER);
            }
            placed++;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            if (!paste.running) break;
        }
    }

    std::string text = "Paste finished: " + std::to_string(placed) + " placed, " + std::to_string(missing) + " skipped (not in inventory).";
    if (!missingIds.empty()) {
        text += " Missing item IDs:";
        for (int id : missingIds) text += " " + std::to_string(id);
    }
    {
        GamePacket<OnConsoleMessage> p;
        p.text = text;
        SendGamePacket(CLIENT, p);
    }
    paste.running = false;
}


// ---------------- Copy music ----------------
bool PlayerState::SaveSongLayout(const std::string& path, int& outNotes) {
    outNotes = 0;
    std::vector<WorldTile> tiles;
    int w = 0, h = 0;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        if (worldTiles.empty() || worldWidth <= 0 || worldHeight <= 0) return false;
        tiles = worldTiles;
        w = worldWidth;
        h = worldHeight;
    }

    /* @info: A world song is its sheet-music layout: background tiles whose
       item type is MUSIC_SHEET (28). X is the time, Y is the pitch, the item
       id is the instrument. The Note Block item (220) plays one fixed song
       and carries no pitch, so it is not part of the song. */
    std::ofstream out(path);
    if (!out) return false;
    out << "ADMAPROXY-SONG-V1\n";
    out << w << ' ' << h << '\n';
    for (int i = 0; i < static_cast<int>(tiles.size()); i++) {
        if (tiles[i].bg == 0) continue;
        if (GetItemType(tiles[i].bg) != type::MUSIC_SHEET) continue;
        int x = i % w, y = i / w;
        out << x << ' ' << y << ' ' << tiles[i].bg << '\n';
        outNotes++;
    }
    return true;
}

bool PlayerState::LoadSongLayout(const std::string& path, std::vector<SongNote>& out) {
    out.clear();
    std::ifstream in(path);
    if (!in) return false;
    std::string header;
    if (!std::getline(in, header) || header != "ADMAPROXY-SONG-V1") return false;
    std::string line;
    if (!std::getline(in, line)) return false;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        SongNote n{};
        if (!(ss >> n.x >> n.y >> n.itemId)) continue;
        if (n.x < 0 || n.y < 0 || n.itemId <= 0) continue;
        out.push_back(n);
    }
    return !out.empty();
}

void PlayerState::PasteSong(const std::string& path, ENetPeer* CLIENT, ENetPeer* SERVER) {
    std::vector<SongNote> notes;
    if (!LoadSongLayout(path, notes)) {
        GamePacket<OnConsoleMessage> p;
        p.text = "Paste failed: could not read the song file.";
        SendGamePacket(CLIENT, p);
        paste.running = false;
        return;
    }

    int placed = 0, missing = 0;
    std::vector<int> missingIds;
    for (const auto& note : notes) {
        if (!paste.running) break;
        float px = position.x > 0.0f ? position.x : static_cast<float>(note.x * 32 + 16);
        float py = position.y > 0.0f ? position.y : static_cast<float>(note.y * 32 + 16);
        if (!HasItem(note.itemId)) {
            missing++;
            if (std::find(missingIds.begin(), missingIds.end(), note.itemId) == missingIds.end()) missingIds.push_back(note.itemId);
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            Place(note.itemId, px, py, note.x, note.y, SERVER);
        }
        placed++;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::string text = "Paste finished: " + std::to_string(placed) + " notes placed, " + std::to_string(missing) + " skipped (not in inventory).";
    if (!missingIds.empty()) {
        text += " Missing item IDs:";
        for (int id : missingIds) text += " " + std::to_string(id);
    }
    {
        GamePacket<OnConsoleMessage> p;
        p.text = text;
        SendGamePacket(CLIENT, p);
    }
    paste.running = false;
}

bool PlayerState::ResendMapData(ENetPeer* CLIENT) {
    std::vector<uint8_t> packet;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        if (lastMapPacket.empty()) return false;
        packet = lastMapPacket;
    }
    if (!CLIENT || CLIENT->state != ENET_PEER_STATE_CONNECTED) return false;
    ENetPacket* p = enet_packet_create(packet.data(), packet.size(), ENET_PACKET_FLAG_RELIABLE);
    if (!p) return false;
    {
        std::lock_guard<std::mutex> lock(Network.peer_mutex);
        enet_peer_send(CLIENT, 0, p);
    }
    return true;
}

// ---------------- Batch 2: movement overrides ----------------

void PlayerState::ApplySpeed(ENetPacket* packet, float clientX, float clientY) {
    /* @info: Speed. Scales the client's movement delta by speedFactor and
       adds it to the last sent position, so the server sees the player walk
       faster. A sudden huge jump is a resync (warp, teleport echo), not
       walking, so it re-syncs instead of scaling. The final sent position
       is re-synced by SyncSpeed after the other overrides run. */
    if (!speedEnabled || !packet || packet->dataLength < 44) return;
    if (!speed.synced) {
        speed.prevClientX = clientX;
        speed.prevClientY = clientY;
        speed.prevSentX = clientX;
        speed.prevSentY = clientY;
        speed.synced = true;
        return;
    }
    float dx = clientX - speed.prevClientX;
    float dy = clientY - speed.prevClientY;
    if (std::abs(dx) > 200.0f || std::abs(dy) > 200.0f) {
        speed.prevClientX = clientX;
        speed.prevClientY = clientY;
        speed.prevSentX = clientX;
        speed.prevSentY = clientY;
        return;
    }
    float newX = speed.prevSentX + dx * speedFactor;
    float newY = speed.prevSentY + dy * speedFactor;
    std::memcpy(packet->data + 4 + 0x18, &newX, sizeof(float));
    std::memcpy(packet->data + 4 + 0x1C, &newY, sizeof(float));
    speed.prevClientX = clientX;
    speed.prevClientY = clientY;
    speed.prevSentX = newX;
    speed.prevSentY = newY;
}

void PlayerState::SyncSpeed(float sentX, float sentY) {
    /* @info: Called after every other override, with the packet's final
       position. Keeps the speed baseline honest when a teleport, collect
       or pathfind step replaced the position this packet. */
    if (!speedEnabled) return;
    speed.prevSentX = sentX;
    speed.prevSentY = sentY;
}

void PlayerState::ApplyNoclip(ENetPacket* packet, float clientX, float clientY, int characterState) {
    /* @info: Noclip. Movement intent comes from the character state flags
       (32 = moving, 16 = facing left, 128 = jumping; see the debug notes in
       PACKET_STATE). While the player moves freely the override tracks the
       client position; while the player pushes against a wall (intent set,
       position not changing) the override position is nudged through it,
       and the server echoes the new position back to the client. One quirk:
       at the apex of a jump the Y speed briefly reads as zero while the
       jumping flag is still set, so jumps can go slightly higher. */
    if (!noclip || !packet || packet->dataLength < 44) return;
    if (!noclipState.synced) {
        noclipState.x = clientX;
        noclipState.y = clientY;
        noclipState.lastClientX = clientX;
        noclipState.lastClientY = clientY;
        noclipState.synced = true;
        return;
    }
    bool moving = (characterState & 32) != 0;
    bool jumping = (characterState & 128) != 0;
    float dx = clientX - noclipState.lastClientX;
    float dy = clientY - noclipState.lastClientY;
    if (moving && std::abs(dx) < 0.5f) {
        float dir = (characterState & 16) ? -1.0f : 1.0f;
        noclipState.x += dir * NOCLIP_STEP;
    }
    else {
        noclipState.x = clientX;
    }
    if (jumping && std::abs(dy) < 0.5f) {
        noclipState.y -= NOCLIP_STEP;
    }
    else {
        noclipState.y = clientY;
    }
    noclipState.lastClientX = clientX;
    noclipState.lastClientY = clientY;
    std::memcpy(packet->data + 4 + 0x18, &noclipState.x, sizeof(float));
    std::memcpy(packet->data + 4 + 0x1C, &noclipState.y, sizeof(float));
}

void PlayerState::ApplyPathfind(ENetPacket* packet) {
    /* @info: Pathfind. Same window pattern as auto-collect: while the
       worker's step window is open, outgoing packets carry the step's
       position instead of the real one. Runs after auto-collect, so a path
       step wins over a collect target while both are active. */
    if (!pathfind.running || !packet || packet->dataLength < 44) return;
    std::lock_guard<std::mutex> lock(worldMutex);
    if (std::chrono::steady_clock::now() >= pathfind.targetUntil) return;
    std::memcpy(packet->data + 4 + 0x18, &pathfind.targetX, sizeof(float));
    std::memcpy(packet->data + 4 + 0x1C, &pathfind.targetY, sizeof(float));
    float zero = 0.0f;
    std::memcpy(packet->data + 4 + 0x24, &zero, sizeof(float));
}

void PlayerState::ApplyLongPunch(ENetPacket* packet, int punchX, int punchY) {
    /* @info: Long punch. Extends the punch coordinates to LONG_PUNCH_REACH
       tiles out from the player, in the same direction, so far blocks can
       be punched (and far tiles placed on). Whether the server accepts a
       punch that far away is NOT verified: Gurotopia does no distance
       check, the real server might. Needs in-game testing. */
    if (!longPunch || !packet || packet->dataLength < 56) return;
    if (punchX < 0 || punchY < 0) return;
    if (position.x <= 0.0f || position.y <= 0.0f) return;
    int px = static_cast<int>(position.x) / 32;
    int py = static_cast<int>(position.y) / 32;
    int dx = punchX - px, dy = punchY - py;
    int len = std::max(std::abs(dx), std::abs(dy));
    if (len == 0) return;
    int nx = px + dx * LONG_PUNCH_REACH / len;
    int ny = py + dy * LONG_PUNCH_REACH / len;
    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    std::memcpy(packet->data + 4 + 0x2C, &nx, sizeof(int));
    std::memcpy(packet->data + 4 + 0x30, &ny, sizeof(int));
}

// ---------------- Batch 2: surgery ----------------

void PlayerState::AutoSurgeryStep(const std::string& dialog, ENetPeer* CLIENT, ENetPeer* SERVER) {
    /* @info: Auto surgery. Surgery runs through dialogs: the server sends an
       OnDialogRequest with the patient status, the client answers with a
       dialog_return naming the tool button. The symptom-to-tool rules below
       come from a field-used auto-surgery implementation (CyrusSE
       Growtopia-Internal-Cheat, written for a private server) and match the
       official surgery guide's dialog flow, but they are NOT verified
       against a live capture from the official server. One deliberate
       deviation from that reference: the fix tools (gloves, pins, splint)
       only run when a fixable condition was actually detected, instead of
       firing whenever the player happens to own them. A tool button that is
       missing from the dialog means the player does not own the tool, so
       the proxy stops instead of spamming a dead button. */
    if (!autoSurgery) return;

    auto sendTool = [&](int toolId, const char* log) -> bool {
        std::string btn = "tool" + std::to_string(toolId);
        if (dialog.find(btn) == std::string::npos) {
            autoSurgery = false;
            GamePacket<OnConsoleMessage> p;
            p.text = "Auto surgery stopped: missing tool " + std::to_string(toolId) + ".";
            SendGamePacket(CLIENT, p);
            return false;
        }
        std::string pkt = "action|dialog_return\ndialog_name|surgery\nbuttonClicked|" + btn + "\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, pkt.c_str(), static_cast<int>(pkt.length()));
        }
        LOG_INFO("Auto surgery: {}", log);
        return true;
    };

    // Surgery dialog opened: start the surgery.
    if (dialog.find("Surgeon Skill") != std::string::npos) {
        std::string pkt = "action|dialog_return\ndialog_name|surge\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, pkt.c_str(), static_cast<int>(pkt.length()));
        }
        surgFindFix = false;
        return;
    }

    if (dialog.find("Operation site") == std::string::npos) return;

    // Conditions that need a fix step later.
    if (dialog.find("You've exposed the brain.") != std::string::npos ||
        dialog.find("Patient suffered massive trauma with internal bleeding") != std::string::npos ||
        dialog.find("Patient suffered fatty liver.") != std::string::npos ||
        dialog.find("Patient suffered chaos infection") != std::string::npos) {
        surgFindFix = true;
    }

    size_t failPos = dialog.find("Skill Fail");
    if (failPos != std::string::npos && failPos + 10 < dialog.size()) {
        std::string skill = dialog.substr(failPos + 10);
        size_t end = skill.find("```3]");
        if (end != std::string::npos) skill = skill.substr(0, end);
        LOG_INFO("Auto surgery: skill failed ({})", skill);
    }

    // Ordered symptom -> tool rules. First match wins, one tool per dialog.
    if (dialog.find("`4You can't") != std::string::npos) { sendTool(SURG_SPONGE, "Used sponge"); return; }
    if (dialog.find("`4Heart") != std::string::npos) { sendTool(SURG_DEFIB, "Heart was stopped"); return; }
    if (dialog.find("Patient's fever is `4climbing `4fast!") != std::string::npos ||
        dialog.find("Patient's fever is `6climbing!") != std::string::npos ||
        dialog.find("Temp: `4") != std::string::npos ||
        dialog.find("Temp: `6") != std::string::npos ||
        dialog.find("Temp: `3") != std::string::npos) {
        if (dialog.find("tool4318") != std::string::npos) sendTool(SURG_LABKIT, "Used lab kit");
        else sendTool(SURG_ANTIBIOTICS, "Used antibiotics");
        return;
    }
    if (dialog.find("Patient is losing blood") != std::string::npos) { sendTool(SURG_STITCHES, "Stitching"); return; }
    if (dialog.find("`4Extremely Weak") != std::string::npos ||
        dialog.find("Pulse: `6Weak") != std::string::npos) {
        sendTool(SURG_TRANSFUSION, "Transfusion");
        return;
    }
    if (dialog.find("`4Awake") != std::string::npos ||
        dialog.find("`3Awake") != std::string::npos ||
        dialog.find("`6Coming to") != std::string::npos) {
        sendTool(SURG_ANESTHETIC, "Anesthetic");
        return;
    }
    if (dialog.find("`4Unsanitary") != std::string::npos ||
        dialog.find("`3Not `3sanitized") != std::string::npos ||
        dialog.find("`6Unclean") != std::string::npos) {
        sendTool(SURG_ANTISEPTIC, "Antiseptic");
        return;
    }
    if (dialog.find("not been diagnosed") != std::string::npos) {
        if (sendTool(SURG_ULTRASOUND, "Ultrasound")) surgFindFix = true;
        return;
    }
    if (dialog.find("Patient wants a nose job") != std::string::npos ||
        dialog.find("shattered") != std::string::npos) {
        if (sendTool(SURG_SCALPEL, "Scalpel")) surgFindFix = true;
        return;
    }
    if (dialog.find("You pickup it.") != std::string::npos) {
        sendTool(SURG_STITCHES, "Closing up");
        surgFindFix = false;
        return;
    }

    // No symptom matched: fix the found condition, or keep looking for it.
    if (surgFindFix) {
        if (dialog.find("tool1296") != std::string::npos) {
            if (sendTool(SURG_GLOVES, "Fixing with gloves")) surgFindFix = false;
            return;
        }
        if (dialog.find("tool4308") != std::string::npos) { sendTool(SURG_PINS, "Pins"); return; }
        if (dialog.find("tool1268") != std::string::npos) { sendTool(SURG_SPLINT, "Splint"); return; }
        if (dialog.find("tool1260") != std::string::npos) { sendTool(SURG_SCALPEL, "Looking for the fix"); return; }
        autoSurgery = false;
        GamePacket<OnConsoleMessage> p;
        p.text = "Auto surgery stopped: no fix tool (gloves, pins, splint, scalpel).";
        SendGamePacket(CLIENT, p);
        return;
    }
}

// ---------------- Batch 2: fishing ----------------

bool PlayerState::FindWaterTile(int& outX, int& outY) {
    if (position.x <= 0.0f || position.y <= 0.0f) return false;
    int px = static_cast<int>(position.x) / 32;
    int py = static_cast<int>(position.y) / 32;
    static const int cand[8][2] = { {-1, 1}, {1, 1}, {-1, 0}, {1, 0}, {0, 1}, {-1, -1}, {1, -1}, {0, -1} };
    std::lock_guard<std::mutex> lock(worldMutex);
    if (worldWidth <= 0 || worldHeight <= 0) return false;
    for (auto& c : cand) {
        int x = px + c[0], y = py + c[1];
        if (x < 0 || y < 0 || x >= worldWidth || y >= worldHeight) continue;
        if (worldTiles[static_cast<size_t>(y) * static_cast<size_t>(worldWidth) + static_cast<size_t>(x)].flag & TILE_FLAG_WATER) {
            outX = x;
            outY = y;
            return true;
        }
    }
    return false;
}

void PlayerState::CastFishing(ENetPeer* CLIENT, ENetPeer* SERVER) {
    /* @info: One cast. Runs in a worker thread because the drill/detonate
       step sleeps; the casting flag keeps a splash and a catch message
       right after each other from firing two casts. The cast itself is a
       plain item use (Place with the bait id), which is also how the reel
       works: the splash happens at the bait tile, so casting again reels
       the fish in. */
    if (!autoFish) return;
    bool expected = false;
    if (!fishing.casting.compare_exchange_strong(expected, true)) return;

    std::thread([this, CLIENT, SERVER]() {
        // Drill or detonate first when the water is covered. The packet
        // form is a plain item use, like every other item; it is NOT
        // verified against a capture, so this needs in-game testing.
        if (autoFish) {
            int px = static_cast<int>(position.x) / 32;
            int py = static_cast<int>(position.y) / 32;
            int tool = 0, toolX = -1, toolY = -1;
            {
                std::lock_guard<std::mutex> lock(worldMutex);
                static const int cand[8][2] = { {-1, 1}, {1, 1}, {-1, 0}, {1, 0}, {0, 1}, {-1, -1}, {1, -1}, {0, -1} };
                for (auto& c : cand) {
                    int x = px + c[0], y = py + c[1];
                    if (x < 0 || y < 0 || x >= worldWidth || y >= worldHeight) continue;
                    uint16_t fg = worldTiles[static_cast<size_t>(y) * static_cast<size_t>(worldWidth) + static_cast<size_t>(x)].fg;
                    if (fg == ITEM_ICE_CRUST && HasItem(ITEM_HAND_DRILL)) { tool = ITEM_HAND_DRILL; toolX = x; toolY = y; break; }
                    if (fg == ITEM_URANIUM && HasItem(ITEM_DETONATOR)) { tool = ITEM_DETONATOR; toolX = x; toolY = y; break; }
                }
            }
            if (tool != 0) {
                Place(tool, position.x, position.y, toolX, toolY, SERVER);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

        if (!autoFish) { fishing.casting = false; return; }

        if (!HasItem(ITEM_WIGGLY_WORM)) {
            autoFish = false;
            GamePacket<OnConsoleMessage> p;
            p.text = "Auto fishing stopped: out of Wiggly Worms.";
            SendGamePacket(CLIENT, p);
            fishing.casting = false;
            return;
        }

        int wx = -1, wy = -1;
        if (!FindWaterTile(wx, wy)) {
            autoFish = false;
            GamePacket<OnConsoleMessage> p;
            p.text = "Auto fishing stopped: no water next to you.";
            SendGamePacket(CLIENT, p);
            fishing.casting = false;
            return;
        }

        Place(ITEM_WIGGLY_WORM, position.x, position.y, wx, wy, SERVER);
        fishing.casting = false;
    }).detach();
}

void PlayerState::ScheduleFishingRecast(ENetPeer* CLIENT, ENetPeer* SERVER) {
    /* @info: After a catch (or an empty line) the next cast goes out after
       a short pause. The generation check kills recasts scheduled before
       the last /fish toggle. */
    int gen = fishing.generation.load();
    std::thread([this, CLIENT, SERVER, gen]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        if (!autoFish || fishing.generation.load() != gen) return;
        CastFishing(CLIENT, SERVER);
    }).detach();
}

// ---------------- Batch 2: pathfind ----------------

bool PlayerState::FindPath(int fromX, int fromY, int toX, int toY, std::vector<std::pair<int, int>>& out) {
    /* @info: A* over the parsed tile map. A tile is walkable when it has no
       foreground block, or its block has NO_COLLISION or ON_TOP (platform)
       in items.dat; unknown items count as solid. If the goal tile itself
       is solid, the nearest walkable neighbour is used instead. The path
       is a plain 4-direction grid path: it may climb through open air,
       because the proxy moves the player along it with position overrides
       rather than simulating jumps. */
    out.clear();
    int w = 0, h = 0;
    std::vector<WorldTile> tiles;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        if (worldWidth <= 0 || worldHeight <= 0) return false;
        w = worldWidth;
        h = worldHeight;
        tiles = worldTiles;
    }
    if (fromX < 0 || fromY < 0 || toX < 0 || toY < 0 || fromX >= w || fromY >= h || toX >= w || toY >= h) return false;

    auto walkable = [&](int x, int y) -> bool {
        if (x < 0 || y < 0 || x >= w || y >= h) return false;
        uint16_t fg = tiles[static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)].fg;
        if (fg == 0) return true;
        int c = GetItemCollision(fg);
        if (c < 0) return false;
        return c == collision::NO_COLLISION || c == collision::ON_TOP;
    };

    if (!walkable(toX, toY)) {
        const int nb[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
        bool found = false;
        for (auto& d : nb) {
            if (walkable(toX + d[0], toY + d[1])) { toX += d[0]; toY += d[1]; found = true; break; }
        }
        if (!found) return false;
    }

    auto idx = [&](int x, int y) { return y * w + x; };
    auto heur = [&](int x, int y) { return std::abs(x - toX) + std::abs(y - toY); };

    struct Node { int f, g, x, y; };
    struct Cmp {
        bool operator()(const Node& a, const Node& b) const { return a.f > b.f; }
    };
    std::priority_queue<Node, std::vector<Node>, Cmp> open;
    std::unordered_map<int, int> gScore, cameFrom;
    std::unordered_map<int, bool> closed;

    open.push({ heur(fromX, fromY), 0, fromX, fromY });
    gScore[idx(fromX, fromY)] = 0;
    int expanded = 0;
    const int dirs[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

    while (!open.empty()) {
        Node cur = open.top();
        open.pop();
        int ci = idx(cur.x, cur.y);
        if (closed[ci]) continue;
        closed[ci] = true;
        if (++expanded > 200000) return false;

        if (cur.x == toX && cur.y == toY) {
            std::vector<std::pair<int, int>> path;
            int at = ci;
            while (at != idx(fromX, fromY)) {
                path.emplace_back(at % w, at / w);
                auto it = cameFrom.find(at);
                if (it == cameFrom.end()) return false;
                at = it->second;
            }
            path.emplace_back(fromX, fromY);
            std::reverse(path.begin(), path.end());
            out = std::move(path);
            return true;
        }

        for (auto& d : dirs) {
            int nx = cur.x + d[0], ny = cur.y + d[1];
            if (!walkable(nx, ny)) continue;
            int ni = idx(nx, ny);
            if (closed[ni]) continue;
            int ng = cur.g + 1;
            auto it = gScore.find(ni);
            if (it == gScore.end() || ng < it->second) {
                gScore[ni] = ng;
                cameFrom[ni] = ci;
                open.push({ ng + heur(nx, ny), ng, nx, ny });
            }
        }
    }
    return false;
}

void PlayerState::PathfindLoop(ENetPeer* CLIENT, ENetPeer* SERVER) {
    (void)SERVER;
    int tx = -1, ty = -1;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        tx = pathfind.destX;
        ty = pathfind.destY;
    }
    int fromX = static_cast<int>(position.x) / 32;
    int fromY = static_cast<int>(position.y) / 32;
    std::vector<std::pair<int, int>> path;
    if (!FindPath(fromX, fromY, tx, ty, path) || path.size() < 2) {
        GamePacket<OnConsoleMessage> p;
        p.text = path.size() == 1 ? "Already there." : "No path to that tile.";
        SendGamePacket(CLIENT, p);
        pathfind.running = false;
        return;
    }
    // One tile per 200ms, about walking speed. The override window outlives
    // the sleep so the position stays overridden between steps.
    for (size_t i = 1; i < path.size(); i++) {
        if (!pathfind.running) return;
        float wx = static_cast<float>(path[i].first * 32 + 16);
        float wy = static_cast<float>(path[i].second * 32 + 16);
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            pathfind.targetX = wx;
            pathfind.targetY = wy;
            pathfind.targetUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (!pathfind.running) return;
    {
        GamePacket<OnConsoleMessage> p;
        p.text = "Arrived.";
        SendGamePacket(CLIENT, p);
    }
    pathfind.running = false;
}

void PlayerState::StopPathfind() {
    pathfind.running = false;
    std::lock_guard<std::mutex> lock(worldMutex);
    pathfind.targetUntil = {};
    pathfind.destX = -1;
    pathfind.destY = -1;
}

// ---------------- Batch 2: ESP + map view ----------------

void PlayerState::SendTileUpdate(ENetPeer* CLIENT, int x, int y, uint16_t fg, uint16_t bg, uint16_t flag) {
    /* @info: Forged single-tile update to the client: 4-byte message type,
       56-byte tank packet (type 5, punch = tile, extra size 8, extra-data
       flag bit set), then one serialized tile (fg, bg, parent, flag).
       Built by hand because PlayerMovingPacket::Send zeroes the extra
       region instead of filling it. */
    if (!CLIENT || CLIENT->state != ENET_PEER_STATE_CONNECTED) return;
    uint8_t buf[4 + 56 + 8] = {};
    int msgType = Network.NET_MESSAGE_GAME_PACKET;
    std::memcpy(buf + 0, &msgType, 4);
    int pktType = Network.PACKET_SEND_TILE_UPDATE_DATA;
    std::memcpy(buf + 4 + 0x00, &pktType, 4);
    std::memcpy(buf + 4 + 0x04, &netID, 4);
    int flags = 8;
    std::memcpy(buf + 4 + 0x0C, &flags, 4);
    std::memcpy(buf + 4 + 0x2C, &x, 4);
    std::memcpy(buf + 4 + 0x30, &y, 4);
    int extraSize = 8;
    std::memcpy(buf + 4 + 0x34, &extraSize, 4);
    uint16_t parent = 0;
    std::memcpy(buf + 60 + 0, &fg, 2);
    std::memcpy(buf + 60 + 2, &bg, 2);
    std::memcpy(buf + 60 + 4, &parent, 2);
    std::memcpy(buf + 60 + 6, &flag, 2);
    ENetPacket* p = enet_packet_create(buf, sizeof(buf), ENET_PACKET_FLAG_RELIABLE);
    if (!p) return;
    {
        std::lock_guard<std::mutex> lock(Network.peer_mutex);
        enet_peer_send(CLIENT, 0, p);
    }
}

bool PlayerState::EspMarkTile(ENetPeer* CLIENT, int x, int y, uint16_t blockId) {
    if (x < 0 || y < 0) return false;
    uint64_t key = EspKey(x, y);
    uint16_t bg = 0, flag = 0;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        if (espMarkers.find(key) != espMarkers.end()) return true;
        if (worldWidth <= 0 || x >= worldWidth || y >= worldHeight) return false;
        const auto& t = worldTiles[static_cast<size_t>(y) * static_cast<size_t>(worldWidth) + static_cast<size_t>(x)];
        if (t.fg != 0) return false;
        bg = t.bg;
        flag = t.flag;
        espMarkers[key] = t.fg;
    }
    SendTileUpdate(CLIENT, x, y, blockId, bg, flag);
    return true;
}

void PlayerState::EspUnmarkKey(ENetPeer* CLIENT, uint64_t key) {
    int x = static_cast<int>(key >> 32);
    int y = static_cast<int>(key & 0xFFFFFFFFu);
    uint16_t fg = 0, bg = 0, flag = 0;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        auto it = espMarkers.find(key);
        if (it == espMarkers.end()) return;
        fg = it->second;
        espMarkers.erase(it);
        if (worldWidth > 0 && x < worldWidth && y < worldHeight) {
            const auto& t = worldTiles[static_cast<size_t>(y) * static_cast<size_t>(worldWidth) + static_cast<size_t>(x)];
            bg = t.bg;
            flag = t.flag;
        }
    }
    SendTileUpdate(CLIENT, x, y, fg, bg, flag);
}

void PlayerState::EspReleaseKey(ENetPeer* CLIENT, uint64_t key) {
    bool restore = false;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        for (const auto& kv : espDropTiles) if (kv.second == key) return;
        for (const auto& kv : espPlayerTiles) if (kv.second == key) return;
        if (espMarkers.find(key) == espMarkers.end()) return;
        restore = true;
    }
    if (restore) EspUnmarkKey(CLIENT, key);
}

void PlayerState::EspMarkDrop(ENetPeer* CLIENT, const WorldDrop& drop) {
    int x = static_cast<int>(drop.x) / 32;
    int y = static_cast<int>(drop.y) / 32;
    if (EspMarkTile(CLIENT, x, y, ESP_DROP_BLOCK)) {
        std::lock_guard<std::mutex> lock(worldMutex);
        espDropTiles[drop.uid] = EspKey(x, y);
    }
}

void PlayerState::EspUnmarkDrop(ENetPeer* CLIENT, uint32_t uid) {
    uint64_t key = 0;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        auto it = espDropTiles.find(uid);
        if (it == espDropTiles.end()) return;
        key = it->second;
        espDropTiles.erase(it);
    }
    EspReleaseKey(CLIENT, key);
}

void PlayerState::EspMarkPlayer(ENetPeer* CLIENT, int netId, float x, float y) {
    int tx = static_cast<int>(x) / 32;
    int ty = static_cast<int>(y) / 32;
    uint64_t key = EspKey(tx, ty);
    uint64_t oldKey = 0;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        auto it = espPlayerTiles.find(netId);
        if (it != espPlayerTiles.end() && it->second == key) return;
        if (it != espPlayerTiles.end()) oldKey = it->second;
        espPlayerTiles[netId] = key;
    }
    if (oldKey != 0 && oldKey != key) EspReleaseKey(CLIENT, oldKey);
    EspMarkTile(CLIENT, tx, ty, ESP_PLAYER_BLOCK);
}

void PlayerState::EspUnmarkPlayer(ENetPeer* CLIENT, int netId) {
    uint64_t key = 0;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        auto it = espPlayerTiles.find(netId);
        if (it == espPlayerTiles.end()) return;
        key = it->second;
        espPlayerTiles.erase(it);
    }
    EspReleaseKey(CLIENT, key);
}

void PlayerState::RefreshEspMarkers(ENetPeer* CLIENT) {
    ClearEspMarkers(CLIENT);
    std::vector<WorldDrop> drops;
    std::vector<std::pair<int, Position>> players;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        drops = worldDrops;
        for (const auto& kv : playerPositions) players.emplace_back(kv.first, kv.second);
    }
    for (const auto& d : drops) EspMarkDrop(CLIENT, d);
    for (const auto& p : players) EspMarkPlayer(CLIENT, p.first, p.second.x, p.second.y);
}

void PlayerState::ClearEspMarkers(ENetPeer* CLIENT) {
    std::vector<uint64_t> keys;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        for (const auto& kv : espMarkers) keys.push_back(kv.first);
        espMarkers.clear();
        espDropTiles.clear();
        espPlayerTiles.clear();
    }
    for (uint64_t key : keys) {
        int x = static_cast<int>(key >> 32);
        int y = static_cast<int>(key & 0xFFFFFFFFu);
        uint16_t bg = 0, flag = 0;
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            if (worldWidth > 0 && x < worldWidth && y < worldHeight) {
                const auto& t = worldTiles[static_cast<size_t>(y) * static_cast<size_t>(worldWidth) + static_cast<size_t>(x)];
                bg = t.bg;
                flag = t.flag;
            }
        }
        SendTileUpdate(CLIENT, x, y, 0, bg, flag);
    }
}

void PlayerState::EspListItems(ENetPeer* CLIENT) {
    struct Entry { std::string name; int amount; float dist; std::string dir; };
    std::vector<Entry> entries;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        for (const auto& d : worldDrops) {
            const Items* it = FindItemById(d.itemId);
            std::string name = it ? it->raw_name : ("item " + std::to_string(d.itemId));
            float dx = d.x - position.x, dy = d.y - position.y;
            float dist = std::sqrt(dx * dx + dy * dy) / 32.0f;
            entries.push_back({ name, d.amount, dist, EspDirection(dx, dy) });
        }
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.dist < b.dist; });
    std::string text = entries.empty() ? "No drops in this world." : "Drops in this world:";
    size_t shown = 0;
    for (const auto& e : entries) {
        if (shown >= 15) break;
        text += "\n" + e.name + " x" + std::to_string(e.amount) + " - " +
            std::to_string(static_cast<int>(e.dist + 0.5f)) + " tiles " + e.dir;
        shown++;
    }
    if (entries.size() > shown) text += "\n... and " + std::to_string(entries.size() - shown) + " more.";
    GamePacket<OnConsoleMessage> p;
    p.text = text;
    SendGamePacket(CLIENT, p);
}

void PlayerState::EspListPlayers(ENetPeer* CLIENT) {
    struct Entry { std::string name; float dist; std::string dir; };
    std::vector<Entry> entries;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        for (const auto& kv : worldPlayers) {
            auto it = playerPositions.find(kv.first);
            if (it == playerPositions.end()) continue;
            float dx = it->second.x - position.x, dy = it->second.y - position.y;
            float dist = std::sqrt(dx * dx + dy * dy) / 32.0f;
            entries.push_back({ kv.second, dist, EspDirection(dx, dy) });
        }
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.dist < b.dist; });
    std::string text = entries.empty() ? "No other players with a known position." : "Players in this world:";
    size_t shown = 0;
    for (const auto& e : entries) {
        if (shown >= 15) break;
        text += "\n" + e.name + " - " + std::to_string(static_cast<int>(e.dist + 0.5f)) + " tiles " + e.dir;
        shown++;
    }
    if (entries.size() > shown) text += "\n... and " + std::to_string(entries.size() - shown) + " more.";
    GamePacket<OnConsoleMessage> p;
    p.text = text;
    SendGamePacket(CLIENT, p);
}

bool PlayerState::ReapplyMapView(ENetPeer* CLIENT) {
    /* @info: Re-sends the world map with the current rewrite toggles
       applied (xray hides fg, door marker colors doors), or the original
       map when no rewrite is on. ESP markers are re-applied afterwards,
       because a fresh map wipes them. Returns false when no map is stored
       yet, in which case the toggles apply on the next world join. */
    bool sent = false;
    if (xray || doorMark) sent = PACKET_SEND_MAP_DATA::ResendRewrittenMap(CLIENT);
    else sent = ResendMapData(CLIENT);
    if (sent && esp) RefreshEspMarkers(CLIENT);
    return sent;
}

// ---------------- Batch 3: blackjack gems counter ----------------

void PlayerState::OnChandelierBreak(int x, int y, int puncherNetId, ENetPeer* CLIENT) {
    /* @info: A Chandelier tile (fg 340) just disappeared. The puncher is
       read from the tile update's netid field: Gurotopia builds the tile
       update from the incoming punch state, which carries the puncher's
       netid. That is not verified against a live capture from the official
       server, so an unknown netid just means no name and no talk bubble.
       Gems land within a couple of seconds, so the result is reported
       after a short window instead of on the first drop. */
    if (!blackjack) return;
    int eventId = 0;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        bjEvents.erase(std::remove_if(bjEvents.begin(), bjEvents.end(),
            [](const BjEvent& e) { return e.reported; }), bjEvents.end());
        BjEvent ev;
        ev.id = bjNextEventId++;
        ev.x = x;
        ev.y = y;
        ev.at = std::chrono::steady_clock::now();
        ev.puncherNetId = puncherNetId;
        if (puncherNetId == netID) ev.puncherName = tankIDName;
        else {
            auto it = worldPlayers.find(puncherNetId);
            if (it != worldPlayers.end()) ev.puncherName = it->second;
        }
        eventId = ev.id;
        bjEvents.push_back(std::move(ev));
    }
    std::thread([this, eventId, CLIENT]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(BJ_WINDOW_MS));
        std::string text;
        std::string name;
        int bubbleNetId = -1;
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            BjEvent* ev = nullptr;
            for (auto& e : bjEvents) if (e.id == eventId) { ev = &e; break; }
            if (!ev || ev->reported) return;
            ev->reported = true;
            name = ev->puncherName;
            if (ev->puncherNetId > 0 && !ev->puncherName.empty()) bubbleNetId = ev->puncherNetId;
            int total = 0;
            for (const auto& kv : bjDropEvent) {
                if (kv.second != eventId) continue;
                auto amt = bjDropAmounts.find(kv.first);
                if (amt != bjDropAmounts.end()) total += amt->second;
            }
            for (auto it = bjDropEvent.begin(); it != bjDropEvent.end();) {
                if (it->second == eventId) {
                    bjDropAmounts.erase(it->first);
                    it = bjDropEvent.erase(it);
                }
                else ++it;
            }
            text = "Blackjack: ";
            if (!name.empty()) text += name + " got ";
            text += std::to_string(total);
            if (total > 21) text += " - BUST!";
            else if (total == 21) text += " - BLACKJACK!";
            bjHistory.push_back(text);
            if (bjHistory.size() > BJ_HISTORY_MAX) bjHistory.erase(bjHistory.begin());
        }
        if (!CLIENT || CLIENT->state != ENET_PEER_STATE_CONNECTED) return;
        GamePacket<OnConsoleMessage> p;
        p.text = text;
        SendGamePacket(CLIENT, p);
        if (bubbleNetId > 0) {
            GamePacket<OnTalkBubble> b;
            b.netId = bubbleNetId;
            b.text = text;
            SendGamePacket(CLIENT, b);
        }
    }).detach();
}

void PlayerState::OnGemDrop(uint32_t uid, int amount, float x, float y) {
    /* @info: A Gems drop (item 112) appeared or changed. Update packets
       refresh the amount of an already attributed drop; new drops are
       attributed to the nearest unreported chandelier break within 1 tile.
       Gems spawn at the punch position plus a small random offset, so they
       always land on or next to the broken tile. */
    if (!blackjack) return;
    int tx = static_cast<int>(x) / 32;
    int ty = static_cast<int>(y) / 32;
    std::lock_guard<std::mutex> lock(worldMutex);
    auto known = bjDropEvent.find(uid);
    if (known != bjDropEvent.end()) {
        bjDropAmounts[uid] = amount;
        return;
    }
    auto now = std::chrono::steady_clock::now();
    int bestId = -1, bestDist = 2;
    for (const auto& e : bjEvents) {
        if (e.reported) continue;
        if (now - e.at > std::chrono::milliseconds(BJ_WINDOW_MS)) continue;
        int d = std::max(std::abs(tx - e.x), std::abs(ty - e.y));
        if (d <= 1 && (bestId < 0 || d < bestDist)) { bestId = e.id; bestDist = d; }
    }
    if (bestId < 0) return;
    bjDropEvent[uid] = bestId;
    bjDropAmounts[uid] = amount;
}

void PlayerState::BjShowLog(ENetPeer* CLIENT) {
    std::string text;
    {
        std::lock_guard<std::mutex> lock(worldMutex);
        if (bjHistory.empty()) text = "No blackjack results yet.";
        else {
            text = "Blackjack log:";
            for (const auto& line : bjHistory) text += "\n" + line;
        }
    }
    GamePacket<OnConsoleMessage> p;
    p.text = text;
    SendGamePacket(CLIENT, p);
}
