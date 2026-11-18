#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"
#include "../../Items/Items.hpp"
#include "../../Player/Player.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace {

// Tile flag bits, see GTProxy src/world/tile.hpp.
constexpr uint16_t TILE_FLAG_EXTRA = 1 << 0;
constexpr uint16_t TILE_FLAG_LOCKED = 1 << 1;
constexpr uint16_t TILE_FLAG_OPEN = 1 << 6;

// Door marker colors. Solid blocks read better at a glance than a paint
// tint over the door sprite, and they never clobber a player's paint.
constexpr uint16_t BLOCK_RED = 170;
constexpr uint16_t BLOCK_GREEN = 176;

// Bounds-checked cursor over the raw map packet. Every read fails cleanly
// instead of running past the buffer, so a malformed packet only costs the
// proxy's copy of the world, never a crash and never the client's map.
struct MapCursor {
    const uint8_t* data = nullptr;
    size_t size = 0;
    size_t pos = 0;

    bool ReadU8(uint8_t& out) {
        if (pos + 1 > size) return false;
        out = data[pos];
        pos += 1;
        return true;
    }
    bool ReadU16(uint16_t& out) {
        if (pos + 2 > size) return false;
        std::memcpy(&out, data + pos, 2);
        pos += 2;
        return true;
    }
    bool ReadU32(uint32_t& out) {
        if (pos + 4 > size) return false;
        std::memcpy(&out, data + pos, 4);
        pos += 4;
        return true;
    }
    bool ReadI32(int32_t& out) {
        if (pos + 4 > size) return false;
        std::memcpy(&out, data + pos, 4);
        pos += 4;
        return true;
    }
    bool Skip(size_t n) {
        if (pos + n > size) return false;
        pos += n;
        return true;
    }
    bool ReadString(std::string& out) {
        uint16_t len = 0;
        if (!ReadU16(len)) return false;
        if (pos + len > size) return false;
        out.assign(reinterpret_cast<const char*>(data + pos), len);
        pos += len;
        return true;
    }
};

// Skips one tile extra block. Only the sizes matter here: the proxy walks
// past the extra data to reach the next tile. Layouts mirrored from GTProxy
// src/world/tile_extra.cpp (known serializers plus the unknown-type table).
// Returns false on an unknown type or truncated data, which aborts the whole
// map parse rather than desyncing it.
inline bool SkipTileExtra(MapCursor& cur, uint8_t extraType, uint16_t fg, uint16_t version) {
    std::string tmp;
    uint32_t count = 0;
    switch (extraType) {
        case 0: return true; // None: no data after the type byte.
        case 1: return cur.ReadString(tmp) && cur.Skip(1); // Door: label + 1
        case 2: return cur.ReadString(tmp) && cur.Skip(4); // Sign: label + 4
        case 3: { // Lock: 1 + owner 4 + access count 4 + accesses + 8 (+16 for id 5814)
            if (!cur.Skip(1 + 4)) return false;
            if (!cur.ReadU32(count)) return false;
            if (!cur.Skip(static_cast<size_t>(count) * 4 + 8)) return false;
            if (fg == 5814 && !cur.Skip(16)) return false;
            return true;
        }
        case 4: return cur.Skip(5); // Seed: growth 4 + fruit 1
        case 8: return cur.Skip(1); // Dice
        case 9: { // Provider: 4, plus 4 for ids 5318 / 10656 (v17+)
            if (!cur.Skip(4)) return false;
            if (fg == 5318 || (fg == 10656 && version >= 17)) return cur.Skip(4);
            return true;
        }
        case 10: return cur.Skip(5); // Achievement: 4 + 1
        case 11: return cur.Skip(4) && cur.ReadString(tmp); // HeartMonitor: 4 + label
        case 14: return cur.ReadString(tmp) && cur.Skip(23); // Mannequin
        case 15: return cur.Skip(4); // BunnyEgg
        case 17: return cur.Skip(1); // GameGen
        case 18: return cur.Skip(5); // Xenonite: 1 + 4
        case 19: return cur.Skip(18); // Dressup
        case 20: return cur.ReadString(tmp); // Crystal: string
        case 21: return cur.ReadString(tmp) && cur.Skip(5); // Burglar: string + 4 + 1
        case 23: return cur.Skip(4); // DisplayBlock: item id
        case 24: return cur.Skip(8); // Vending: item id + price
        case 25: { // Solar: 1 + count 4 + count * 4
            if (!cur.Skip(1)) return false;
            if (!cur.ReadU32(count)) return false;
            return cur.Skip(static_cast<size_t>(count) * 4);
        }
        case 27: return cur.Skip(4);
        case 28: return cur.Skip(6); // Deco: 1 + 4 + 1
        case 30: return cur.Skip(5); // Steam Organ?
        case 32: return cur.Skip(4); // SewingMachine
        case 33: return cur.ReadString(tmp); // CountryFlag: string
        case 35: return cur.Skip(4) && cur.ReadString(tmp);
        case 36: return cur.ReadString(tmp) && cur.Skip(12); // BattleCage: label + 3 pets
        case 39: return cur.Skip(4);
        case 40: return cur.Skip(4); // WeatherSpecial: color
        case 42: return cur.Skip(21);
        case 43: return cur.Skip(16);
        case 44: { // VipEntrance: 1 + owner 4 + count 4 + accesses
            if (!cur.Skip(1 + 4)) return false;
            if (!cur.ReadU32(count)) return false;
            return cur.Skip(static_cast<size_t>(count) * 4);
        }
        case 48: return cur.ReadString(tmp) && cur.Skip(26);
        case 49: return cur.Skip(9);
        case 55: { // count + count * 4 + 16
            if (!cur.ReadU32(count)) return false;
            return cur.Skip(static_cast<size_t>(count) * 4 + 16);
        }
        case 56: return cur.ReadString(tmp) && cur.Skip(4);
        case 57: return cur.Skip(4); // GeigerCharger
        case 58: return true;
        case 61: return cur.Skip(35);
        case 62: return cur.Skip(14);
        case 63: { // count + count * 15 + 8
            if (!cur.ReadU32(count)) return false;
            return cur.Skip(static_cast<size_t>(count) * 15 + 8);
        }
        case 65: return cur.Skip(17);
        case 66: return cur.Skip(1);
        case 69: return cur.Skip(16);
        case 71: return cur.Skip(44);
        case 72: return cur.Skip(12);
        case 73: return cur.Skip(4);
        case 74: return true;
        case 75: return cur.Skip(312);
        case 77: { // 4 + count + count * 4
            if (!cur.Skip(4)) return false;
            if (!cur.ReadU32(count)) return false;
            return cur.Skip(static_cast<size_t>(count) * 4);
        }
        case 81: return cur.Skip(8);
        default: return false;
    }
}

// Some item types always carry extra data even without the Extra flag.
// Mirrors GTProxy Tile::idiot_growtopia_dev exactly. When items.dat is not
// loaded yet, only the flag is trusted.
inline bool TileHasForcedExtra(uint16_t fg) {
    int t = GetItemType(fg);
    if (t < 0) return false;
    return t == type::LOCK || t == type::MAIN_DOOR || t == type::VENDING_MACHINE || t == type::DISPLAY_BLOCK;
}

inline bool TileIsDoor(uint16_t fg) {
    int t = GetItemType(fg);
    if (t >= 0) return t == type::DOOR || t == type::MAIN_DOOR;
    return fg == 6 || fg == 12; // Main Door / Door, when items.dat is not loaded.
}

} // namespace

// World packet parse class
class PACKET_SEND_MAP_DATA {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet,
        ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {

        if (Type != 4 || !packet || !packet->data) return true;
        if (Method != "SERVER -> PROXY -> CLIENT") return true;
        if (packet->dataLength < 60) return true;

        /* Keep the untouched packet so rewrite toggles can restore it. */
        {
            std::lock_guard<std::mutex> lock(Player.worldMutex);
            Player.lastMapPacket.assign(packet->data, packet->data + packet->dataLength);
        }

        ParseMap(packet->data, packet->dataLength, true);

        if (Player.esp) Player.RefreshEspMarkers(CLIENT);

        return true;
    }

    /* @info: Re-sends the stored map with the current rewrite toggles
       (xray, door marker) applied, so toggling works without a rejoin.
       The stored world state is NOT overwritten: drops collected since
       the map arrived would otherwise come back from the dead. */
    static inline bool ResendRewrittenMap(ENetPeer* CLIENT) {
        std::vector<uint8_t> copy;
        {
            std::lock_guard<std::mutex> lock(Player.worldMutex);
            if (Player.lastMapPacket.empty()) return false;
            copy = Player.lastMapPacket;
        }
        ParseMap(copy.data(), copy.size(), false);
        if (!CLIENT || CLIENT->state != ENET_PEER_STATE_CONNECTED) return false;
        ENetPacket* p = enet_packet_create(copy.data(), copy.size(), ENET_PACKET_FLAG_RELIABLE);
        if (!p) return false;
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            enet_peer_send(CLIENT, 0, p);
        }
        return true;
    }

private:
    static inline void ParseMap(uint8_t* data, size_t size, bool storeState) {
        /* @info: Map data starts behind the 4-byte message type and the
           56-byte tank header: version u16, skip 4, name (u16-len string),
           tile map (w i32, h i32, count u32, skip 5, tiles), object map
           (skip 12, count u32, drop id u32, objects). Layout from GTProxy
           src/world/world.hpp, tile_map.hpp, object_map.hpp. */
        if (!data || size < 60) return;

        MapCursor cur{ data, size, 60 };
        uint16_t version = 0;
        std::string worldName;
        if (!cur.ReadU16(version)) return;
        if (!cur.Skip(4)) return;
        if (!cur.ReadString(worldName)) return;

        int32_t w = 0, h = 0;
        uint32_t tileCount = 0;
        if (!cur.ReadI32(w) || !cur.ReadI32(h) || !cur.ReadU32(tileCount)) return;
        if (!cur.Skip(5)) return;
        if (w <= 0 || h <= 0 || w > 1000 || h > 1000 || tileCount > 1000000) {
            LOG_WARN("Map data for [{}] has impossible size {}x{} ({} tiles), ignoring.", worldName, w, h, tileCount);
            return;
        }
        if (tileCount != static_cast<uint32_t>(w) * static_cast<uint32_t>(h)) {
            LOG_WARN("Map data for [{}]: tile count {} != {}x{}.", worldName, tileCount, w, h);
        }

        std::vector<PlayerState::WorldTile> tiles;
        tiles.reserve(tileCount);
        bool doorMark = Player.doorMark;
        bool xray = Player.xray;
        bool ok = true;
        for (uint32_t i = 0; i < tileCount; i++) {
            size_t tileStart = cur.pos;
            uint16_t fg = 0, bg = 0, parent = 0, flag = 0;
            if (!cur.ReadU16(fg) || !cur.ReadU16(bg) || !cur.ReadU16(parent) || !cur.ReadU16(flag)) { ok = false; break; }
            if (flag & TILE_FLAG_LOCKED) {
                uint16_t lockParent = 0;
                if (!cur.ReadU16(lockParent)) { ok = false; break; }
            }
            if ((flag & TILE_FLAG_EXTRA) || TileHasForcedExtra(fg)) {
                uint8_t extraType = 0;
                if (!cur.ReadU8(extraType) || !SkipTileExtra(cur, extraType, fg, version)) {
                    LOG_WARN("Map data for [{}]: unknown tile extra type {} at tile {}, stopping parse.", worldName, static_cast<int>(extraType), i);
                    ok = false;
                    break;
                }
            }
            PlayerState::WorldTile tile{};
            tile.fg = fg;
            tile.bg = bg;
            tile.flag = flag;
            tiles.push_back(tile);

            /* @info: Client-side rewrites. The stored tile keeps the real
               ids; only the packet the client sees is changed. X-ray hides
               every foreground block, door marker colors doors green when
               the Open flag is set, red when it is not. */
            if (xray) {
                uint16_t air = 0;
                std::memcpy(data + tileStart, &air, sizeof(uint16_t));
            }
            else if (doorMark && TileIsDoor(fg)) {
                uint16_t marker = (flag & TILE_FLAG_OPEN) ? BLOCK_GREEN : BLOCK_RED;
                std::memcpy(data + tileStart, &marker, sizeof(uint16_t));
            }
        }

        std::vector<PlayerState::WorldDrop> drops;
        if (ok) {
            if (!cur.Skip(12)) ok = false;
            uint32_t dropCount = 0, dropId = 0;
            if (ok && (!cur.ReadU32(dropCount) || !cur.ReadU32(dropId))) ok = false;
            if (ok && dropCount > 100000) {
                LOG_WARN("Map data for [{}]: impossible drop count {}.", worldName, dropCount);
                ok = false;
            }
            for (uint32_t i = 0; ok && i < dropCount; i++) {
                uint16_t itemId = 0;
                int32_t px = 0, py = 0;
                uint8_t amount = 0, flags = 0;
                uint32_t uid = 0;
                if (!cur.ReadU16(itemId) || !cur.ReadI32(px) || !cur.ReadI32(py) ||
                    !cur.ReadU8(amount) || !cur.ReadU8(flags) || !cur.ReadU32(uid)) { ok = false; break; }
                PlayerState::WorldDrop drop{};
                drop.uid = uid;
                drop.itemId = itemId;
                drop.amount = amount;
                drop.x = static_cast<float>(px);
                drop.y = static_cast<float>(py);
                drops.push_back(drop);
            }
        }

        if (!ok) {
            LOG_WARN("Map data for [{}] did not parse fully ({} of {} tiles, {} drops). Features using the map work on partial data.", worldName, tiles.size(), tileCount, drops.size());
        }

        if (!storeState) return;

        LOG_DEBUG("World Name: {}", worldName);
        LOG_DEBUG("World Size: {} x {}", w, h);
        LOG_DEBUG("Total Blocks: {}", tileCount);
        LOG_DEBUG("Drops: {}", drops.size());

        Player.world = worldName;
        {
            std::lock_guard<std::mutex> lock(Player.worldMutex);
            Player.worldWidth = w;
            Player.worldHeight = h;
            Player.worldTiles = std::move(tiles);
            Player.worldDrops = std::move(drops);
            Player.playerPositions.clear();
        }
    }
};
