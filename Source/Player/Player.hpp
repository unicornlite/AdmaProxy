#pragma once

#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <vector>
#include <cstdint>

#include "ENET/enet.h"

struct PlayerVec2 { float x; float y; };
struct PlayerVec3 { float x; float y; float z; };


class PlayerState {
public:
    int userID = -1;


    std::string tankIDName = "";
    int player_age = 0;
    std::string klv = "", meta = "", rid = "", mac = "", wk = "";


    int netID = -1;
    std::string country = "";
    std::string world = "", lastWorld = "", saveWorld = "";
    std::unordered_map<int, std::string> worldPlayers;


    bool dropMode = false;
    bool trashMode = false;
    bool wrenchMode = false;
    bool autoChange = false;
    bool hideClothes = false;
    bool antiMod = false;
    bool antiGravity = false;
    float hoverY = -1.0f;
    bool unlimitedZoom = false;
    bool doorMark = false;
    bool autoSurgery = false;
    /* @info: Set when the surgery patient has a condition that needs a fix
       (exposed brain, internal bleeding, fatty liver, chaos infection,
       undiagnosed, nose job); cleared once the fix tool is used. */
    bool surgFindFix = false;
    std::atomic<bool> autoFish{ false };
    bool speedEnabled = false;
    float speedFactor = 1.5f;
    bool longPunch = false;
    bool noclip = false;
    bool esp = false;
    bool xray = false;
    bool spinInfo = false;
    /* @info: Original local OnSpawn text, kept so /zoom off can send the
       unmodified spawn back and restore normal zoom without a rejoin. */
    std::string lastLocalSpawn;
    /* @info: Last layout files written by /copyworld and /copymusic, so
       /pasteworld and /pastemusic need no argument after a warp. */
    std::string lastWorldFile;
    std::string lastSongFile;

    struct Position {
        float x = 0.0f;
        float y = 0.0f;
    }; Position position;


    struct Clothes {
        PlayerVec3 hair_shirt_pants{ 0.000000f, 0.000000f, 0.000000f };
        PlayerVec3 feet_face_hand{ 0.000000f, 0.000000f, 0.000000f };
        PlayerVec3 back_mask_necklace{ 0.000000f, 0.000000f, 0.000000f };
        uint32_t skinColor = 31;
        PlayerVec3 ances_unknown_unknown2{ 0.000000f, 0.000000f, 0.000000f };
    }; Clothes clothes;


    struct Inventory {
        static constexpr int MAX_STACK = 200;
        int size = 16;
        std::unordered_map<int, int> items;

        int GetCount(int id) const;
        bool HasItem(int id) const;
        void AddItem(int id, int count);
        void RemoveItem(int id, int count);
        void ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items);
    };


    struct DialogSkipper {
        std::mutex Skipper;
        std::vector<std::string> DROP;
        bool TRASH = false;
        bool SALES_MAN = false;
    }; DialogSkipper dialogSkipper;


    struct Design {
        int OnDialogRequest_RED = 0;
        int OnDialogRequest_GREEN = 0;
        int OnDialogRequest_BLUE = 0;
        int OnDialogRequest_ALPHA = 160;
    }; Design design;


    struct Autofarm {
        std::atomic<bool> running = false;
        int id = -1;
        float X = -1.00f, Y = -1.00f;
        int punchX = -1, punchY = -1;
        std::thread worker;
    }; Autofarm autofarm;


    struct Autospam {
        std::atomic<bool> running = false;
        std::string text1 = "Hello World!";
        std::string text2 = "";
        int delay = 3600;
        bool rainbow = false;
        std::thread worker;
    }; Autospam autospam;


    /* @info: Parsed world state, filled by PACKET_SEND_MAP_DATA and kept fresh
       by PACKET_SEND_TILE_UPDATE_DATA and PACKET_ITEM_CHANGE_OBJECT. The
       network thread writes it and the auto-collect worker reads it, so every
       access goes through worldMutex. */
    struct WorldTile {
        uint16_t fg = 0;
        uint16_t bg = 0;
        uint16_t flag = 0;
    };

    struct WorldDrop {
        uint32_t uid = 0;
        uint16_t itemId = 0;
        int amount = 0;
        float x = 0.0f;
        float y = 0.0f;
    };

    struct WorldPlacement {
        int x = 0;
        int y = 0;
        int fg = 0;
        int bg = 0;
    };

    struct SongNote {
        int x = 0;
        int y = 0;
        int itemId = 0;
    };

    int worldWidth = 0;
    int worldHeight = 0;
    std::vector<WorldTile> worldTiles;
    std::vector<WorldDrop> worldDrops;
    std::unordered_map<int, Position> playerPositions;
    /* @info: Last SEND_MAP_DATA packet exactly as the server sent it, before
       any client-side rewrite (door marker). Resent to restore the original
       view when a rewrite toggle goes off. */
    std::vector<uint8_t> lastMapPacket;
    std::mutex worldMutex;


    struct AutoCollect {
        std::atomic<bool> running = false;
        std::thread worker;
        /* @info: Current collect target, guarded by worldMutex. The worker
           sets it, PACKET_STATE overrides the outgoing position while the
           window is open. */
        float targetX = 0.0f;
        float targetY = 0.0f;
        std::chrono::steady_clock::time_point targetUntil{};
        /* @info: Drops already attempted, worker thread only. A drop that did
           not vanish after a collect attempt is retried after a cooldown
           instead of being hammered every loop. */
        std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> attempts;
    }; AutoCollect autocollect;


    struct Teleport {
        /* @info: Active teleport target, guarded by worldMutex. While the
           window is open, outgoing STATE packets carry this position. */
        float x = 0.0f;
        float y = 0.0f;
        std::chrono::steady_clock::time_point until{};
        float savedX = 0.0f;
        float savedY = 0.0f;
        bool hasSaved = false;
    }; Teleport teleport;


    struct Paste {
        std::atomic<bool> running = false;
        std::thread worker;
    }; Paste paste;

    struct Fishing {
        /* @info: Bumped on every /fish toggle so stale recast threads die
           instead of casting after the user turned fishing off and on. */
        std::atomic<int> generation{ 0 };
        /* @info: True while a cast worker is running (drill check + cast),
           so a splash and a catch message right after each other do not
           fire two casts. */
        std::atomic<bool> casting{ false };
    }; Fishing fishing;

    /* @info: Speed state, network thread only. The delta between the last
       two client positions is scaled by speedFactor and added to the last
       sent position, so the server sees the player walk faster. */
    struct Speed {
        float prevClientX = 0.0f, prevClientY = 0.0f;
        float prevSentX = 0.0f, prevSentY = 0.0f;
        bool synced = false;
    }; Speed speed;

    /* @info: Noclip state, network thread only. Tracks the client position
       while it moves freely; while the player pushes against a wall (moving
       intent in the character state, position not changing) the override
       position is nudged through it instead. */
    struct NoclipState {
        float x = 0.0f, y = 0.0f;
        float lastClientX = 0.0f, lastClientY = 0.0f;
        bool synced = false;
    }; NoclipState noclipState;

    struct Pathfind {
        std::atomic<bool> running = false;
        std::thread worker;
        int destX = -1, destY = -1;
        /* @info: Current path step, guarded by worldMutex. The worker sets
           it, PACKET_STATE overrides the outgoing position while the window
           is open, same pattern as auto-collect. */
        float targetX = 0.0f;
        float targetY = 0.0f;
        std::chrono::steady_clock::time_point targetUntil{};
    }; Pathfind pathfind;

    /* @info: ESP markers. The proxy cannot draw on the game screen, so it
       marks tiles with bright blocks instead (yellow for drops, aqua for
       players). Keys are (x << 32) | y; the value is the tile's real fg, so
       turning ESP off restores it. Only air tiles are marked, because a
       real block's extra data cannot be restored from a tile update.
       Guarded by worldMutex. */
    std::unordered_map<uint64_t, uint16_t> espMarkers;
    std::unordered_map<uint32_t, uint64_t> espDropTiles;
    std::unordered_map<int, uint64_t> espPlayerTiles;

private:
    Inventory inventory;

public:
    void Reset();
    void ClearWorld();

    std::string InventoryToString() const;
    int GetItemCount(int id) const;
    bool HasItem(int id) const;
    void AddItem(int id, int count);
    void RemoveItem(int id, int count);
    void ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items);

public:
    void Warp(std::string worldName, ENetPeer* CLIENT, ENetPeer* SERVER);
    void LeaveWorld(ENetPeer* CLIENT, ENetPeer* SERVER, const std::string& modName);
    void ApplyAntiGravity(ENetPacket* packet, float currentY);
    void ApplyAutoCollect(ENetPacket* packet);
    void ApplyTeleport(ENetPacket* packet);
    void ApplySpeed(ENetPacket* packet, float clientX, float clientY);
    void SyncSpeed(float sentX, float sentY);
    void ApplyNoclip(ENetPacket* packet, float clientX, float clientY, int characterState);
    void ApplyPathfind(ENetPacket* packet);
    void ApplyLongPunch(ENetPacket* packet, int punchX, int punchY);

    void AutoSurgeryStep(const std::string& dialog, ENetPeer* CLIENT, ENetPeer* SERVER);
    bool FindWaterTile(int& outX, int& outY);
    void CastFishing(ENetPeer* CLIENT, ENetPeer* SERVER);
    void ScheduleFishingRecast(ENetPeer* CLIENT, ENetPeer* SERVER);

    bool FindPath(int fromX, int fromY, int toX, int toY, std::vector<std::pair<int, int>>& out);
    void PathfindLoop(ENetPeer* CLIENT, ENetPeer* SERVER);
    void StopPathfind();

    void SendTileUpdate(ENetPeer* CLIENT, int x, int y, uint16_t fg, uint16_t bg, uint16_t flag);
    bool EspMarkTile(ENetPeer* CLIENT, int x, int y, uint16_t blockId);
    void EspUnmarkKey(ENetPeer* CLIENT, uint64_t key);
    void EspReleaseKey(ENetPeer* CLIENT, uint64_t key);
    void EspMarkDrop(ENetPeer* CLIENT, const WorldDrop& drop);
    void EspUnmarkDrop(ENetPeer* CLIENT, uint32_t uid);
    void EspMarkPlayer(ENetPeer* CLIENT, int netId, float x, float y);
    void EspUnmarkPlayer(ENetPeer* CLIENT, int netId);
    void RefreshEspMarkers(ENetPeer* CLIENT);
    void ClearEspMarkers(ENetPeer* CLIENT);
    void EspListItems(ENetPeer* CLIENT);
    void EspListPlayers(ENetPeer* CLIENT);

    void TeleportTo(float x, float y, ENetPeer* SERVER);
    void CollectItem(uint32_t uid, ENetPeer* SERVER);
    void AutoCollectLoop(ENetPeer* CLIENT, ENetPeer* SERVER);

    bool SaveWorldLayout(const std::string& path, int& outTiles);
    bool LoadWorldLayout(const std::string& path, std::vector<WorldPlacement>& out);
    void PasteWorld(const std::string& path, ENetPeer* CLIENT, ENetPeer* SERVER);

    bool SaveSongLayout(const std::string& path, int& outNotes);
    bool LoadSongLayout(const std::string& path, std::vector<SongNote>& out);
    void PasteSong(const std::string& path, ENetPeer* CLIENT, ENetPeer* SERVER);

    bool ResendMapData(ENetPeer* CLIENT);
    bool ReapplyMapView(ENetPeer* CLIENT);

    void Drop(int id, int count, ENetPeer* CLIENT, ENetPeer* SERVER);
    void Trash(int id, int count, ENetPeer* CLIENT, ENetPeer* SERVER);

    void Punch(float X, float Y, int punchX, int punchY, ENetPeer* SERVER);
    void Place(int id, float X, float Y, int punchX, int punchY, ENetPeer* SERVER);

    bool AutofarmReady();
    void AutoFarm(ENetPeer* CLIENT, ENetPeer* SERVER);

    void AutoSpam(ENetPeer* CLIENT, ENetPeer* SERVER);
};

extern PlayerState Player;