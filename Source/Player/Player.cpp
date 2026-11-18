#include "Player.hpp"
#include "../Packets/Packets.hpp"
#include "../Network/Handler.hpp"
#include "../Network/Network.hpp"
#include "../Items/Items.hpp"

#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>

PlayerState Player;


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
