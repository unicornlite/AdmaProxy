#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <variant>
#include <vector>
#include <cstring>

class PACKET_STATE {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet, ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {
        /* @debug of 'CharacterState' while moving:
            UPDATE_PACKET_FALLING_DOWN = 0,
	        UPDATE_PACKET_PLAYER_MOVING_RIGHT = 32,
	        UPDATE_PACKET_PLAYER_MOVING_LEFT = 48,
	        UPDATE_PACKET_PLAYER_JUMPING_RIGHT = 128,
	        UPDATE_PACKET_PLAYER_JUMPING_LEFT = 144,
        */
        
        if (Method == "CLIENT -> PROXY -> SERVER" && (pm.NetID() == Player.netID || pm.NetID() == 0)) {
            if (pm.PunchX() == -1 && pm.PunchY() == -1) Player.position.x = pm.X(), Player.position.y = pm.Y();
            Player.ApplyAntiGravity(packet, pm.Y());
            Player.ApplyAutoCollect(packet);
            Player.ApplyTeleport(packet);
        }

        if (Method == "SERVER -> PROXY -> CLIENT" && pm.NetID() > 0 && pm.NetID() != Player.netID) {
            /* @info: Last known position of every other player, used by
               /tp <player name>. Only movement packets carry a position. */
            if (pm.PunchX() == -1 && pm.PunchY() == -1) {
                std::lock_guard<std::mutex> lock(Player.worldMutex);
                auto& pos = Player.playerPositions[pm.NetID()];
                pos.x = pm.X();
                pos.y = pm.Y();
            }
        }

        return true;
    }
};