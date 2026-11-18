#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"
#include "../../Items/Items.hpp"
#include "../../Player/Player.hpp"

#include <vector>
#include <cstdint>
#include <cstring>
#include <mutex>

class PACKET_SEND_TILE_UPDATE_DATA {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet,
        ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {

        /* @info: Single-tile update. Coordinates are the punch fields
           (0x2C/0x30); the extra data past the 56-byte header is one
           serialized tile (fg u16, bg u16, parent u16, flag u16, then lock
           parent and extra data when the flags say so). Corroborated by
           Gurotopia's tile_update, which builds this packet from the
           incoming punch state. Only the first 8 bytes are read here. */
        if (Method != "SERVER -> PROXY -> CLIENT") return true;
        if (!packet || !packet->data || packet->dataLength < 68) return true;

        int x = pm.PunchX();
        int y = pm.PunchY();
        if (x < 0 || y < 0) return true;

        const uint8_t* extra = packet->data + 60;
        uint16_t fg = 0, bg = 0, flag = 0;
        std::memcpy(&fg, extra + 0, 2);
        std::memcpy(&bg, extra + 2, 2);
        std::memcpy(&flag, extra + 6, 2);

        {
            std::lock_guard<std::mutex> lock(Player.worldMutex);
            if (Player.worldWidth > 0 && x < Player.worldWidth && y < Player.worldHeight) {
                size_t idx = static_cast<size_t>(y) * static_cast<size_t>(Player.worldWidth) + static_cast<size_t>(x);
                if (idx < Player.worldTiles.size()) {
                    Player.worldTiles[idx].fg = fg;
                    Player.worldTiles[idx].bg = bg;
                    Player.worldTiles[idx].flag = flag;
                }
            }
            /* @info: The real tile took over, so an ESP marker on this tile
               is gone. The drop/player reference maps are left alone: the
               drop or player is still at this tile, only the visual marker
               was replaced. */
            uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y);
            Player.espMarkers.erase(key);
        }

        /* @info: Client-side rewrites, same as the map packet. X-ray hides
           every foreground block; door marker colors doors green when open,
           red when closed. */
        if (Player.xray) {
            uint16_t air = 0;
            std::memcpy(packet->data + 60, &air, sizeof(uint16_t));
        }
        else if (Player.doorMark) {
            int t = GetItemType(fg);
            bool isDoor = t >= 0 ? (t == type::DOOR || t == type::MAIN_DOOR) : (fg == 6 || fg == 12);
            if (isDoor) {
                uint16_t marker = (flag & (1 << 6)) ? 176 : 170;
                std::memcpy(packet->data + 60, &marker, sizeof(uint16_t));
            }
        }

        return true;
    }
};
