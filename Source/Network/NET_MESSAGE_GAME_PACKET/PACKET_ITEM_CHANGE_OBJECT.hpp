#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"
#include "../../Player/Player.hpp"

#include <vector>
#include <algorithm>
#include <cstring>
#include <mutex>

class PACKET_ITEM_CHANGE_OBJECT {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet,
        ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {

        /* @info: Keeps the parsed drop list fresh between map packets.
           Layout, confirmed against Gurotopia's item_change_object (which
           the real client accepts): 0x04 change type (-1 add, -3 update,
           anything else remove), 0x08 drop UID, 0x10 amount as float,
           0x14 item id (add/update) or drop UID (remove), 0x18/0x1C
           pixel position. GTProxy reads the position from 0x2C/0x30 and
           the remove UID from 0x08 instead; both are covered below. */
        if (Method != "SERVER -> PROXY -> CLIENT") return true;

        int changeType = pm.NetID();
        if (changeType == -1 || changeType == -3) {
            uint32_t uid = static_cast<uint32_t>(pm.Uid());
            uint16_t itemId = static_cast<uint16_t>(pm.PlantingTree() & 0xFFFF);
            int amount = static_cast<int>(pm.FloatVar());
            float x = pm.X(), y = pm.Y();
            if (x == 0.0f && y == 0.0f && (pm.PunchX() != 0 || pm.PunchY() != 0)) {
                x = static_cast<float>(pm.PunchX() * 32);
                y = static_cast<float>(pm.PunchY() * 32);
            }
            PlayerState::WorldDrop added{};
            {
                std::lock_guard<std::mutex> lock(Player.worldMutex);
                bool updated = false;
                for (auto& drop : Player.worldDrops) {
                    if (drop.uid == uid) {
                        drop.itemId = itemId;
                        drop.amount = amount;
                        drop.x = x;
                        drop.y = y;
                        added = drop;
                        updated = true;
                        break;
                    }
                }
                if (!updated) {
                    PlayerState::WorldDrop drop{};
                    drop.uid = uid;
                    drop.itemId = itemId;
                    drop.amount = amount;
                    drop.x = x;
                    drop.y = y;
                    Player.worldDrops.push_back(drop);
                    added = drop;
                }
            }
            if (Player.esp) Player.EspMarkDrop(CLIENT, added);
            if (Player.blackjack && added.itemId == 112) Player.OnGemDrop(added.uid, added.amount, added.x, added.y);
            return true;
        }

        /* Remove. The two references disagree on where the removed drop's
           UID lives, but the 0x08 field tells them apart: Gurotopia sends
           -1 there and the UID at 0x14, GTProxy reads the UID from 0x08. */
        uint32_t uid08 = static_cast<uint32_t>(pm.Uid());
        uint32_t uid14 = static_cast<uint32_t>(pm.PlantingTree());
        uint32_t removedUid = uid08 != 0xFFFFFFFFu ? uid08 : uid14;
        {
            std::lock_guard<std::mutex> lock(Player.worldMutex);
            auto& drops = Player.worldDrops;
            if (uid08 != 0xFFFFFFFFu) {
                drops.erase(std::remove_if(drops.begin(), drops.end(), [uid08](const PlayerState::WorldDrop& d) {
                    return d.uid == uid08;
                }), drops.end());
            }
            else {
                drops.erase(std::remove_if(drops.begin(), drops.end(), [uid14](const PlayerState::WorldDrop& d) {
                    return d.uid == uid14;
                }), drops.end());
            }
        }
        if (Player.esp) Player.EspUnmarkDrop(CLIENT, removedUid);
        return true;
    }
};
