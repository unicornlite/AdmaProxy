#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"
#include "../../Player/Player.hpp"
#include "../../Items/Items.hpp"

#include <variant>
#include <vector>
#include <cstring>
#include <filesystem>

class COMMANDS {
public:
    static inline bool Inject(std::string command, ENetPeer* CLIENT, ENetPeer* SERVER) {
		LOG_DEBUG("Command:[{}]", command);
		
        if (command == "/proxy") {
            GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`w" + Main.NAME + " V" + Main.VERSION + "|left|32|\n";
			p.text += "add_custom_textbox|`4Warning: `oThis proxy has been made by dc:xoidnl and using this proxy is at your own risk!|size:tiny|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wInfo:|size:medium|\n";
			p.text += "add_custom_textbox|`4/proxy  `o(Shows Proxy Commands)|size:small|\n";
			p.text += "add_custom_textbox|`4/news  `o(Shows Growtopia News)|size:small|\n";
			p.text += "add_custom_textbox|`4/ping  `o(Shows Your Real Ping)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wCustomize:|size:medium|\n";
			p.text += "add_custom_textbox|`4/ui  `o(Opens UI Customizer Menu)|size:small|\n";
			p.text += "add_custom_textbox|`4/country [code]  `o(Changes Your Country Flag)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wMain Features:|size:medium|\n";
			p.text += "add_custom_textbox|`4/farm  `o(Starts/Stops Auto-Farming)|size:small|\n";
			p.text += "add_custom_textbox|`4/spam  `o(Opens Auto-Spam Settings Menu)|size:small|\n";
			p.text += "add_custom_textbox|`4/wrench  `o(Starts/Stops Auto-Wrench (Left = Pull & Right = Kick)|size:small|\n";
			p.text += "add_custom_textbox|`4/antimod  `o(Starts/Stops Auto-Leave When A Mod Enters)|size:small|\n";
			p.text += "add_custom_textbox|`4/antigravitasi  `o(Starts/Stops Anti-Gravity (Float In Place))|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wWorld Tools:|size:medium|\n";
			p.text += "add_custom_textbox|`4/collect  `o(Starts/Stops Auto-Collect Dropped Items)|size:small|\n";
			p.text += "add_custom_textbox|`4/zoom  `o(Starts/Stops Unlimited Zoom)|size:small|\n";
			p.text += "add_custom_textbox|`4/copyworld  `o(Saves The Current World Layout To A File)|size:small|\n";
			p.text += "add_custom_textbox|`4/pasteworld [name]  `o(Rebuilds A Saved World Layout Here)|size:small|\n";
			p.text += "add_custom_textbox|`4/copymusic  `o(Saves The World Song (Sheet Music) To A File)|size:small|\n";
			p.text += "add_custom_textbox|`4/pastemusic [name]  `o(Rebuilds A Saved Song Here)|size:small|\n";
			p.text += "add_custom_textbox|`4/tp [x y / name]  `o(Teleports To Coordinates, A Player, Or The Saved Point)|size:small|\n";
			p.text += "add_custom_textbox|`4/tpset  `o(Saves The Current Position As Teleport Point)|size:small|\n";
			p.text += "add_custom_textbox|`4/doormark  `o(Marks Open Doors Green And Closed Doors Red)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wAutomation:|size:medium|\n";
			p.text += "add_custom_textbox|`4/surg  `o(Starts/Stops Auto Surgery)|size:small|\n";
			p.text += "add_custom_textbox|`4/fish  `o(Starts/Stops Auto Fishing)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wMovement:|size:medium|\n";
			p.text += "add_custom_textbox|`4/pathfind <x> <y>  `o(Walks To Tile Coordinates)|size:small|\n";
			p.text += "add_custom_textbox|`4/pfind <item>  `o(Walks To The Nearest Drop With That Name)|size:small|\n";
			p.text += "add_custom_textbox|`4/speed [number]  `o(Walks Faster, Default 1.5x)|size:small|\n";
			p.text += "add_custom_textbox|`4/longpunch  `o(Starts/Stops Long Punch)|size:small|\n";
			p.text += "add_custom_textbox|`4/noclip  `o(Starts/Stops Walking Through Walls)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wVision:|size:medium|\n";
			p.text += "add_custom_textbox|`4/esp  `o(Marks Drops And Players With Bright Blocks)|size:small|\n";
			p.text += "add_custom_textbox|`4/espitem  `o(Lists Drops In This World)|size:small|\n";
			p.text += "add_custom_textbox|`4/espplayer  `o(Lists Players In This World)|size:small|\n";
			p.text += "add_custom_textbox|`4/xray  `o(Hides Foreground Blocks)|size:small|\n";
			p.text += "add_custom_textbox|`4/spininfo  `o(Shows If A Roulette Spin Is Real Or Fake, Plus REME)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wCasino:|size:medium|\n";
			p.text += "add_custom_textbox|`4/bj  `o(Starts/Stops The Blackjack Gems Counter)|size:small|\n";
			p.text += "add_custom_textbox|`4/bjlog  `o(Shows The Last 10 Blackjack Results)|size:small|\n";
			p.text += "add_custom_textbox|`4/host  `o(Starts/Stops Casino Hosting Mode, Collects Only DL And BGL)|size:small|\n";
			p.text += "add_custom_textbox|`4/hosttax <percent>  `o(Sets The Host Tax, Default 10%)|size:small|\n";
			p.text += "add_custom_textbox|`4/pot  `o(Shows The Current Pot, Tax And Payout)|size:small|\n";
			p.text += "add_custom_textbox|`4/pay <player>  `o(Pays The Winner The Pot Minus Tax)|size:small|\n";
			p.text += "add_custom_textbox|`4/hostreset  `o(Empties The Pot)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wInformation:|size:medium|\n";
			p.text += "add_custom_textbox|`4/balance  `o(Shows Your Current World Lock Balance)|size:small|\n";
			p.text += "add_custom_textbox|`4/account  `o(Opens a Menu That Displays All Your Account Informations)|size:small|\n";
			p.text += "add_custom_textbox|`4/inventory  `o(Opens a Menu That Displays All The Items In Your Inventory)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wShortcuts:|size:medium|\n";
			p.text += "add_custom_textbox|`4/drop  `o(Enables Fast Drop)|size:small|\n";
			p.text += "add_custom_textbox|`4/trash  `o(Enables Fast Trash/Recycle)|size:small|\n";
			p.text += "add_custom_textbox|`4/pullall  `o(Pulls Everyone In The World)|size:small|\n";
			p.text += "add_custom_textbox|`4/kickall  `o(Kicks Everyone In The World)|size:small|\n";
			p.text += "add_custom_textbox|`4/banall  `o(Bans Everyone In The World)|size:small|\n";
			p.text += "add_custom_textbox|`4/warp [world]  `o(Warps To A World)|size:small|\n";
			p.text += "add_custom_textbox|`4/back  `o(Warps You To A Previously Visited World)|size:small|\n";
			p.text += "add_custom_textbox|`4/setsave [World]  `o(Set Save World)|size:small|\n";
			p.text += "add_custom_textbox|`4/save  `o(Warps You To A Save World)|size:small|\n";
			p.text += "add_custom_textbox|`4/relog  `o(Fast Exit & Join Back To The World)|size:small|\n";
			p.text += "add_custom_textbox|`4/change  `o(Changes DL->BGL When Wrench The Phone)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wDrop Features:|size:medium|\n";
			p.text += "add_custom_textbox|`4/cd [number]  `o(Drops Custom Amount Of Lock)|size:small|\n";
			p.text += "add_custom_textbox|`4/wd [number]  `o(Drops Custom Amount Of WL)|size:small|\n";
			p.text += "add_custom_textbox|`4/dd [number]  `o(Drops Custom Amount Of DL)|size:small|\n";
			p.text += "add_custom_textbox|`4/bd [number]  `o(Drops Custom Amount Of BGL)|size:small|\n";
			p.text += "add_custom_textbox|`4/daw  `o(Drops All Your WLS/DLS/BGLS)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wAnti Lag:|size:medium|\n";
			p.text += "add_custom_textbox|`4/hideclothes  `o(Hides Everyone's Clothes, (Also For Anti Teleport Glitcher)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|ProxyMenu|||\n";
            SendGamePacket(CLIENT, p);

			return false;
        }

		else if (command == "/ping") {
			int serverPing = SERVER ? SERVER->roundTripTime : 0, clientPing = 0;
			if (Local.IP == "127.0.0.1") clientPing = 0;
			else clientPing = CLIENT ? CLIENT->roundTripTime : 0;

			GamePacket<OnConsoleMessage> p;
			p.text = "Your ping is " + std::to_string(serverPing + clientPing) + " (C" + std::to_string(clientPing) + "+S" + std::to_string(serverPing) + ")";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (command == "/ui") {
			GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`wUser Interface|left|32|\n";
			p.text += "add_custom_textbox|`4Explaination: `oYou can modify the growtopia dialog packets woth your custom 'r,g,b,a' color codes!|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_text_input|input_red|`4Red `o(0-255)|0|3|\n";
			p.text += "add_text_input|input_green|`rGreen `o(0-255)|0|3|\n";
			p.text += "add_text_input|input_blue|`cBlue `o(0-255)|0|3|\n";
			p.text += "add_text_input|input_alpha|`bAlpha `o(0-255)|160|3|\n";
			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|PROXY_DIALOGS_UI|Cancel|Apply|\n";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (Packet::Contains(command, "/country ")) {
			std::string text = "", code = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
			if (code.length() == 2) {
				text = "Changed your country flag with [" + code + "]";
				Player.country = code;
			}
			else text = "Wrong country code, example: 'uk'";

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}
		else if (command == "/farm") {
			std::string text;

			if (Player.autofarm.running) {
				text = "Stopped auto-farming!";

				Player.autofarm.running = false;
				Player.autofarm.id = -1;
				Player.autofarm.X = -1.0f;
				Player.autofarm.Y = -1.0f;
				Player.autofarm.punchX = -1;
				Player.autofarm.punchY = -1;
			}
			else {
				text = "Activated auto-farming! (`4place a block to start``)";

				Player.autofarm.running = true;
				Player.autofarm.worker = std::thread(&PlayerState::AutoFarm, &Player, CLIENT, SERVER);
				Player.autofarm.worker.detach();
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnSetFreezeState> p;
				p.seconds = 0;
				SendGamePacket(CLIENT, p, Player.netID);
			}

			return false;
		}

		else if (command == "/spam") {
			GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`wSpam Menu|left|32|\n";
			p.text += "add_custom_textbox|`4Explaination: `oSet the message what you want to spam and click to 'Apply' button. Leave empty the second text if you want to use only 1. Rainbow mode is for random color!|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_text_input|text1|`wFirst Text: |" + Player.autospam.text1 + "|64|\n";
			p.text += "add_text_input|text2|`wSecond Text: |" + Player.autospam.text2 + "|64|\n";
			p.text += "add_text_input|delay|`wDelay(ms): |" + std::to_string(Player.autospam.delay) + "|64|\n";
			p.text += "add_checkbox|rainbow|`wRainbow Mode|" + std::string(Player.autospam.rainbow == true ? "1" : "0") + "\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_button|" + std::string(Player.autospam.running ? "stop" : "start") + "|" + std::string(Player.autospam.running ? "`4STOP" : "`2START") + "||\n";
			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|PROXY_DIALOGS_SPAM|||\n";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (command == "/wrench") {
			std::string text;

			if (Player.wrenchMode) text = "Wrench mode deactivated!";
			else text = "Wrench mode activated, Left click for pull and right click for Kick!";

			Player.wrenchMode = !Player.wrenchMode;

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w Changed the wrench mode!";
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/antimod") {
			std::string text;
			if (Player.antiMod) text = "Anti-Mod deactivated! You will stay in the world when a moderator enters.";
			else text = "Anti-Mod activated! You will leave the world automatically when a moderator enters.";

			Player.antiMod = !Player.antiMod;

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/antigravitasi" || command == "/antigravity") {
			std::string text;
			if (Player.antiGravity) {
				text = "Anti-Gravity deactivated! Gravity is back to normal.";
				Player.hoverY = -1.0f;
			}
			else {
				text = "Anti-Gravity activated! You will float at your current height.";
				Player.hoverY = Player.position.y > 0.0f ? Player.position.y : -1.0f;
			}

			Player.antiGravity = !Player.antiGravity;

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/balance") {
			struct Item { int id; const char* name; };

			Item items[] = {
				{7188, "Blue Gem Lock"},
				{1796, "Diamond Lock"},
				{242,  "World Lock"}
			};

			std::string text;

			for (auto& item : items) {
				int count = Player.GetItemCount(item.id);
				if (count > 0) text += std::to_string(count) + "x " + item.name + ". ";
			}

			if (text.empty()) text = "You don't have any locks.";

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/account") {
			GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`wAccount Menu|left|32|\n";
			p.text += "add_custom_textbox|`4Explaination: `oThis dialog shows all of your account informations!|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`oName: `$" + Player.tankIDName + "|size:small|\n";
			p.text += "add_custom_textbox|`oAge: `$" + std::to_string(Player.player_age) + "|size:small|\n";
			p.text += "add_custom_textbox|`oKLV Address: `$" + Player.klv + "|size:small|\n";
			p.text += "add_custom_textbox|`oMETA Address: `$" + Player.meta + "|size:small|\n";
			p.text += "add_custom_textbox|`oRID Address: `$" + Player.rid + "|size:small|\n";
			p.text += "add_custom_textbox|`oMAC Address: `$" + Player.mac + "|size:small|\n";
			p.text += "add_custom_textbox|`oWK Address: `$" + Player.wk + "|size:small|\n";
			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|PROXY_DIALOGS_ACCOUNT|||\n";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (command == "/inventory") {
			GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`wInventory Menu|left|32|\n";
			p.text += "add_custom_textbox|`4Explaination: `oThis dialog shows all of your inventory!|size:small|\n";
			p.text += "add_spacer|small|\n";
			

			p.text += Player.InventoryToString();

			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|PROXY_DIALOGS_ACCOUNT|||\n";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (command == "/drop") {
			std::string text;
			if (Player.dropMode) text = "Drop mode is deactivated!";
			else text = "Drop mode is activated!";

			Player.dropMode = !Player.dropMode;
			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/trash") {
			std::string text;
			if (Player.trashMode) text = "Trash mode is deactivated!";
			else text = "Trash mode is activated!";

			Player.trashMode = !Player.trashMode;

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/pullall" || command == "/kickall" || command == "/banall") {
			std::string action = "pull";
			if (command == "/kickall") action = "kick";
			else if (command == "/banall") action = "ban";

			std::string text = "Everyone in the world is auto " + action + "ing!";
			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			std::thread([action, SERVER]() {
				for (const auto& pair : Player.worldPlayers) {
					std::string packetNew = "action|input\n|text|/" + action + " " + pair.second + "\n";
					{
						std::lock_guard<std::mutex> lock(Network.peer_mutex);
						SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, packetNew.c_str(), static_cast<int>(packetNew.length()));
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}).detach();

			return false;
		}

		else if (Packet::Contains(command, "/warp ")) {
			std::string newWorld = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
			if (newWorld.empty()) return true;
			Player.Warp(newWorld, CLIENT, SERVER);

			return false;
		}

		else if (Packet::Contains(command, "/setsave ")) {
			std::string newSave = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
			if (newSave.empty()) return true;
			Player.saveWorld = newSave;

			std::string text = "Changed the saving world with [" + newSave + "]";

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/save") {
			if (Player.saveWorld.empty()) {
				std::string text = "Couldn't find the save world, use '/setsave world] command!";

				{
					GamePacket<OnConsoleMessage> p;
					p.text = text;
					SendGamePacket(CLIENT, p);
				}

				{
					GamePacket<OnTalkBubble> p;
					p.netId = Player.netID;
					p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
					SendGamePacket(CLIENT, p);
				}
			}
			else {
				GamePacket<OnConsoleMessage> p;
				p.text = "Warping to the save world!";
				SendGamePacket(CLIENT, p);
				Player.Warp(Player.saveWorld, CLIENT, SERVER);
			}

			return false;
		}

        else if (command == "/back") {
			if (Player.lastWorld.empty()) {
				std::string text = "Couldn't find the last world!";

				{
					GamePacket<OnConsoleMessage> p;
					p.text = text;
					SendGamePacket(CLIENT, p);
				}

				{
					GamePacket<OnTalkBubble> p;
					p.netId = Player.netID;
					p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + "Changed the wrench mode!";
					SendGamePacket(CLIENT, p);
				}
			}
			else {
				std::string newWorld = Player.lastWorld;
				Player.Warp(newWorld, CLIENT, SERVER);
			}

			return false;
		}

        else if (command == "/relog") {
			std::string newWorld = Player.world;
			Player.Warp(newWorld, CLIENT, SERVER);

			return false;
		}

		else if (command == "/change") {
			std::string text;
			if (Player.autoChange) text = "Change mode is deactivated!";
			else text = "Change mode is activated!";

			Player.autoChange = !Player.autoChange;

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (Packet::Contains(command, "/cd ")) {
			int NEEDS = Packet::ExtractCustom<int>(command, " ", 1, "\n");
			if (NEEDS < 1) return false;
			if (NEEDS > 2020200) NEEDS = 2020200;

			int myBGl = Player.HasItem(7188) ? Player.GetItemCount(7188) : 0;
			int myDL = Player.HasItem(1796) ? Player.GetItemCount(1796) : 0;
			int myWL = Player.HasItem(242) ? Player.GetItemCount(242) : 0;

			int myTOTAL = myWL + (myDL * 100) + (myBGl * 10000);

			if (myTOTAL < NEEDS) {
				GamePacket<OnConsoleMessage> p;
				p.text = "Not enough locks!";
				SendGamePacket(CLIENT, p);
				return false;
			}

			std::thread([NEEDS, myBGl, myDL, myWL, CLIENT, SERVER]() mutable {
				int dropBGL = NEEDS / 10000;
				if (dropBGL > myBGl) dropBGL = myBGl;

				int remain = NEEDS - (dropBGL * 10000);

				int dropDL = remain / 100;
				int dropWL = remain % 100;

				while (myWL >= 100 && myDL < dropDL && myDL < 200) {
					LOG_DEBUG("Converting WL -> DL");

					PlayerMovingPacket convertPacket;
					convertPacket
						.PacketType(Network.PACKET_ITEM_ACTIVATE_REQUEST)
						.PlantingTree(242)
						.Send(SERVER);

					std::this_thread::sleep_for(std::chrono::milliseconds(225));

					myWL -= 100;
					myDL += 1;
				}

				if (dropBGL > 0) {
					LOG_DEBUG("Dropping {} BGL", dropBGL);
					Player.Drop(7188, dropBGL, CLIENT, SERVER);
					myBGl -= dropBGL;
					std::this_thread::sleep_for(std::chrono::milliseconds(120));
				}

				while ((myDL < dropDL || dropWL > myWL) && myBGl > 0 && myDL <= 200) {
					LOG_DEBUG("Converting BGL -> DL");

					PlayerMovingPacket convertPacket;
					convertPacket
						.PacketType(Network.PACKET_ITEM_ACTIVATE_REQUEST)
						.PlantingTree(7188)
						.Send(SERVER);

					std::this_thread::sleep_for(std::chrono::milliseconds(225));

					myBGl -= 1;
					myDL += 100;
				}

				if (dropDL > 0) {
					int sendDL = dropDL;
					if (sendDL > myDL) sendDL = myDL;
					LOG_DEBUG("Dropping {} DL", sendDL);
					Player.Drop(1796, sendDL, CLIENT, SERVER);
					myDL -= sendDL;
					std::this_thread::sleep_for(std::chrono::milliseconds(120));
				}

				while (myWL < dropWL && myDL > 0 && myWL <= 100) {
					LOG_DEBUG("Converting DL -> WL");

					PlayerMovingPacket convertPacket;
					convertPacket
						.PacketType(Network.PACKET_ITEM_ACTIVATE_REQUEST)
						.PlantingTree(1796)
						.Send(SERVER);

					std::this_thread::sleep_for(std::chrono::milliseconds(225));

					myDL -= 1;
					myWL += 100;
				}

				if (dropWL > 0) {
					int sendWL = dropWL;
					if (sendWL > myWL) sendWL = myWL;
					LOG_DEBUG("Dropping {} WL", sendWL);
					Player.Drop(242, sendWL, CLIENT, SERVER);
					myWL -= sendWL;
				}
			}).detach();

			return false;
		}

        else if (Packet::Contains(command, "/wd ") || Packet::Contains(command, "/dd ") || Packet::Contains(command, "/bd ")) {
            int item_id = 242;
            if (Packet::Contains(command, "/dd ")) item_id = 1796;
            else if (Packet::Contains(command, "/bd ")) item_id = 7188;

            int item_count = Packet::ExtractCustom<int>(command, " ", 1, "\n");
            if (item_count < 1) return false;
            if (item_count > 200) item_count = 200;

            Player.Drop(item_id, item_count, CLIENT, SERVER);

            return false;
        }

        else if (command == "/daw") {
            std::thread([CLIENT, SERVER]() {
                if (Player.HasItem(7188)) {
                    Player.Drop(7188, Player.GetItemCount(7188), CLIENT, SERVER);
                    std::this_thread::sleep_for(std::chrono::milliseconds(120));
                }

                if (Player.HasItem(1796)) {
                    Player.Drop(1796, Player.GetItemCount(1796), CLIENT, SERVER);
                    std::this_thread::sleep_for(std::chrono::milliseconds(120));
                }

                if (Player.HasItem(242)) {
                    Player.Drop(242, Player.GetItemCount(242), CLIENT, SERVER);
                }
            }).detach();

            return false;
        }

        else if (command == "/hideclothes") {
			std::string text;
			if (Player.hideClothes) text = "Hide Clothes mode is deactivated!";
			else text = "Hide Clothes mode is activated!";

			Player.hideClothes = !Player.hideClothes;

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
        }

		else if (command == "/collect") {
			std::string text;

			if (Player.autocollect.running) {
				text = "Stopped auto-collect!";
				Player.autocollect.running = false;
				Player.hostOwnsWorker = false;
			}
			else {
				text = "Activated auto-collect! Walking to nearby drops automatically.";
				Player.autocollect.running = true;
				Player.autocollect.worker = std::thread(&PlayerState::AutoCollectLoop, &Player, CLIENT, SERVER);
				Player.autocollect.worker.detach();
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/zoom") {
			std::string text;
			if (Player.unlimitedZoom) {
				text = "Unlimited zoom deactivated! Zoom is back to normal.";
				Player.unlimitedZoom = false;
				if (!Player.lastLocalSpawn.empty()) {
					GamePacket<OnSpawn> p;
					p.text = Player.lastLocalSpawn;
					SendGamePacket(CLIENT, p);
				}
			}
			else {
				text = "Unlimited zoom activated!";
				Player.unlimitedZoom = true;
				if (!Player.lastLocalSpawn.empty()) {
					std::string spawn = Player.lastLocalSpawn;
					if (Packet::Change(spawn, "\nmstate|", 1, "\n", "1")) {
						GamePacket<OnSpawn> p;
						p.text = spawn;
						SendGamePacket(CLIENT, p);
					}
				}
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/copyworld") {
			std::string text;
			if (Player.world.empty()) {
				text = "You are not in a world.";
			}
			else {
				int tiles = 0;
				std::string path = "world_" + Player.world + ".txt";
				if (Player.SaveWorldLayout(path, tiles)) {
					Player.lastWorldFile = path;
					text = "World layout saved to " + path + " (" + std::to_string(tiles) + " tiles).";
				}
				else text = "Could not save the world layout. Wait for the world to load and try again.";
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/pasteworld" || Packet::Contains(command, "/pasteworld ")) {
			std::string text;
			if (Player.paste.running) {
				text = "Paste stopped.";
				Player.paste.running = false;
			}
			else if (Player.world.empty()) {
				text = "You are not in a world.";
			}
			else {
				std::string path = Player.lastWorldFile;
				if (Packet::Contains(command, "/pasteworld ")) {
					std::string name = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
					if (!name.empty()) path = "world_" + name + ".txt";
				}
				if (path.empty()) path = "world_" + Player.world + ".txt";
				text = "Pasting world layout from " + path + "...";
				Player.paste.running = true;
				Player.paste.worker = std::thread(&PlayerState::PasteWorld, &Player, path, CLIENT, SERVER);
				Player.paste.worker.detach();
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/copymusic") {
			std::string text;
			if (Player.world.empty()) {
				text = "You are not in a world.";
			}
			else {
				int notes = 0;
				std::string path = "song_" + Player.world + ".txt";
				if (Player.SaveSongLayout(path, notes)) {
					Player.lastSongFile = path;
					if (notes > 0) text = "Song saved to " + path + " (" + std::to_string(notes) + " notes).";
					else text = "No sheet music found in this world. Nothing saved.";
				}
				else text = "Could not save the song. Wait for the world to load and try again.";
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/pastemusic" || Packet::Contains(command, "/pastemusic ")) {
			std::string text;
			if (Player.paste.running) {
				text = "Paste stopped.";
				Player.paste.running = false;
			}
			else if (Player.world.empty()) {
				text = "You are not in a world.";
			}
			else {
				std::string path = Player.lastSongFile;
				if (Packet::Contains(command, "/pastemusic ")) {
					std::string name = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
					if (!name.empty()) path = "song_" + name + ".txt";
				}
				if (path.empty()) path = "song_" + Player.world + ".txt";
				text = "Pasting song from " + path + "...";
				Player.paste.running = true;
				Player.paste.worker = std::thread(&PlayerState::PasteSong, &Player, path, CLIENT, SERVER);
				Player.paste.worker.detach();
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/tp" || Packet::Contains(command, "/tp ")) {
			std::string text;
			bool ok = false;
			float tx = 0.0f, ty = 0.0f;

			if (command == "/tp") {
				if (Player.teleport.hasSaved) {
					tx = Player.teleport.savedX;
					ty = Player.teleport.savedY;
					ok = true;
					text = "Teleporting to the saved point.";
				}
				else text = "No saved point. Use /tpset first, or /tp <x> <y> or /tp <player>.";
			}
			else {
				std::string rest = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
				if (rest.find(' ') != std::string::npos) {
					int x = Packet::ExtractCustom<int>(rest, "", 0, " ");
					int y = Packet::ExtractCustom<int>(rest, " ", 1, "\n");
					if (x > 0 && y > 0) {
						tx = static_cast<float>(x * 32 + 16);
						ty = static_cast<float>(y * 32 + 16);
						ok = true;
						text = "Teleporting to (" + std::to_string(x) + ", " + std::to_string(y) + ").";
					}
					else text = "Invalid coordinates. Usage: /tp <x> <y>";
				}
				else {
					int foundNetID = -1;
					for (const auto& pair : Player.worldPlayers) {
						if (pair.second == rest) { foundNetID = pair.first; break; }
					}
					if (foundNetID == -1) text = "Player [" + rest + "] is not in this world.";
					else {
						std::lock_guard<std::mutex> lock(Player.worldMutex);
						auto it = Player.playerPositions.find(foundNetID);
						if (it == Player.playerPositions.end()) text = "No known position for [" + rest + "] yet.";
						else {
							tx = it->second.x;
							ty = it->second.y;
							ok = true;
							text = "Teleporting to [" + rest + "].";
						}
					}
				}
			}

			if (ok) Player.TeleportTo(tx, ty, SERVER);

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/tpset") {
			std::string text;
			if (Player.position.x <= 0.0f || Player.position.y <= 0.0f) {
				text = "Unknown position. Move around a bit, then try again.";
			}
			else {
				Player.teleport.savedX = Player.position.x;
				Player.teleport.savedY = Player.position.y;
				Player.teleport.hasSaved = true;
				int tx = static_cast<int>(Player.position.x / 32.0f);
				int ty = static_cast<int>(Player.position.y / 32.0f);
				text = "Teleport point saved at (" + std::to_string(tx) + ", " + std::to_string(ty) + ").";
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/doormark") {
			std::string text;
			if (Player.doorMark) {
				text = "Door marker deactivated! Doors show normally again.";
				Player.doorMark = false;
				if (!Player.ReapplyMapView(CLIENT)) text += " (Takes effect when you join a world.)";
			}
			else {
				text = "Door marker activated! Open doors show green, closed doors show red.";
				Player.doorMark = true;
				if (!Player.ReapplyMapView(CLIENT)) text += " (Takes effect when you join a world.)";
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/surg") {
			std::string text;
			if (Player.autoSurgery) {
				text = "Auto surgery deactivated!";
				Player.autoSurgery = false;
			}
			else {
				text = "Auto surgery activated! Wrench a patient to start; the proxy clicks the tools.";
				Player.autoSurgery = true;
				Player.surgFindFix = false;
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/fish") {
			std::string text;
			if (Player.autoFish) {
				text = "Auto fishing deactivated!";
				Player.autoFish = false;
				Player.fishing.generation++;
			}
			else {
				text = "Auto fishing activated! Equip your Fishing Rod and stand next to water.";
				Player.autoFish = true;
				Player.fishing.generation++;
				Player.CastFishing(CLIENT, SERVER);
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/pathfind" || Packet::Contains(command, "/pathfind ")) {
			std::string text;
			if (command == "/pathfind") {
				if (Player.pathfind.running) {
					Player.StopPathfind();
					text = "Pathfind stopped.";
				}
				else text = "Usage: /pathfind <x> <y>";
			}
			else {
				std::string rest = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
				if (rest.find(' ') == std::string::npos) {
					text = "Usage: /pathfind <x> <y>";
				}
				else {
					int x = Packet::ExtractCustom<int>(rest, "", 0, " ");
					int y = Packet::ExtractCustom<int>(rest, " ", 1, "\n");
					if (x < 0 || y < 0) {
						text = "Invalid coordinates. Usage: /pathfind <x> <y>";
					}
					else {
						Player.StopPathfind();
						{
							std::lock_guard<std::mutex> lock(Player.worldMutex);
							Player.pathfind.destX = x;
							Player.pathfind.destY = y;
						}
						Player.pathfind.running = true;
						Player.pathfind.worker = std::thread(&PlayerState::PathfindLoop, &Player, CLIENT, SERVER);
						Player.pathfind.worker.detach();
						text = "Walking to (" + std::to_string(x) + ", " + std::to_string(y) + ").";
					}
				}
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/pfind" || Packet::Contains(command, "/pfind ")) {
			std::string text;
			if (command == "/pfind") {
				if (Player.pathfind.running) {
					Player.StopPathfind();
					text = "Pathfind stopped.";
				}
				else text = "Usage: /pfind <item name>";
			}
			else {
				std::string query = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
				auto toLower = [](std::string s) {
					for (auto& c : s) if (c >= 'A' && c <= 'Z') c += ('a' - 'A');
					return s;
				};
				std::string needle = toLower(query);
				PlayerState::WorldDrop best{};
				bool found = false;
				float bestDist = 0.0f;
				{
					std::lock_guard<std::mutex> lock(Player.worldMutex);
					for (const auto& drop : Player.worldDrops) {
						const Items* it = FindItemById(drop.itemId);
						std::string name = it ? toLower(it->raw_name) : "";
						if (name.find(needle) == std::string::npos) continue;
						float dx = drop.x - Player.position.x, dy = drop.y - Player.position.y;
						float dist = dx * dx + dy * dy;
						if (!found || dist < bestDist) {
							bestDist = dist;
							best = drop;
							found = true;
						}
					}
				}
				if (!found) {
					text = "No drop named [" + query + "] in this world.";
				}
				else {
					int tx = static_cast<int>(best.x) / 32, ty = static_cast<int>(best.y) / 32;
					Player.StopPathfind();
					{
						std::lock_guard<std::mutex> lock(Player.worldMutex);
						Player.pathfind.destX = tx;
						Player.pathfind.destY = ty;
					}
					Player.pathfind.running = true;
					Player.pathfind.worker = std::thread(&PlayerState::PathfindLoop, &Player, CLIENT, SERVER);
					Player.pathfind.worker.detach();
					const Items* it = FindItemById(best.itemId);
					std::string name = it ? it->raw_name : ("item " + std::to_string(best.itemId));
					text = "Walking to the nearest " + name + ".";
				}
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/speed" || Packet::Contains(command, "/speed ")) {
			std::string text;
			if (command == "/speed") {
				if (Player.speedEnabled) {
					text = "Speed deactivated! Walking normally again.";
					Player.speedEnabled = false;
				}
				else {
					text = "Speed activated! Walking faster.";
					Player.speedEnabled = true;
				}
				Player.speed.synced = false;
			}
			else {
				std::string rest = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
				float factor = 0.0f;
				try { factor = std::stof(rest); } catch (...) { factor = 0.0f; }
				if (factor < 0.5f || factor > 5.0f) {
					text = "Invalid speed. Usage: /speed <0.5 - 5>";
				}
				else {
					Player.speedFactor = factor;
					Player.speedEnabled = true;
					Player.speed.synced = false;
					text = "Speed set to " + rest + "x.";
				}
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/longpunch") {
			std::string text;
			if (Player.longPunch) {
				text = "Long punch deactivated! Normal reach again.";
				Player.longPunch = false;
			}
			else {
				text = "Long punch activated! Punches and placements reach 10 tiles.";
				Player.longPunch = true;
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/noclip") {
			std::string text;
			if (Player.noclip) {
				text = "Noclip deactivated! Walls are solid again.";
				Player.noclip = false;
			}
			else {
				text = "Noclip activated! Push against a wall to phase through it.";
				Player.noclip = true;
			}
			Player.noclipState.synced = false;

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/esp") {
			std::string text;
			if (Player.esp) {
				text = "ESP deactivated! Markers removed.";
				Player.esp = false;
				Player.ClearEspMarkers(CLIENT);
			}
			else {
				text = "ESP activated! Drops show yellow, players show aqua. This is not a real overlay.";
				Player.esp = true;
				Player.RefreshEspMarkers(CLIENT);
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/espitem") {
			Player.EspListItems(CLIENT);
			return false;
		}

		else if (command == "/espplayer") {
			Player.EspListPlayers(CLIENT);
			return false;
		}

		else if (command == "/xray") {
			std::string text;
			if (Player.xray) {
				text = "X-ray deactivated! Blocks show normally again.";
				Player.xray = false;
				if (!Player.ReapplyMapView(CLIENT)) text += " (Takes effect when you join a world.)";
			}
			else {
				text = "X-ray activated! Foreground blocks are hidden.";
				Player.xray = true;
				if (!Player.ReapplyMapView(CLIENT)) text += " (Takes effect when you join a world.)";
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/spininfo") {
			std::string text;
			if (Player.spinInfo) {
				text = "Spin info deactivated!";
				Player.spinInfo = false;
			}
			else {
				text = "Spin info activated! Roulette spins show REAL/FAKE plus REME.";
				Player.spinInfo = true;
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/bj") {
			std::string text;
			if (Player.blackjack) {
				text = "Blackjack counter deactivated!";
				Player.blackjack = false;
			}
			else {
				text = "Blackjack counter activated! Break a Chandelier and the gems are counted.";
				Player.blackjack = true;
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/bjlog") {
			Player.BjShowLog(CLIENT);
			return false;
		}

		else if (command == "/host") {
			std::string text;
			if (Player.hosting) {
				Player.hosting = false;
				if (Player.hostOwnsWorker) {
					Player.autocollect.running = false;
					Player.hostOwnsWorker = false;
					text = "Hosting mode deactivated! Auto-collect stopped.";
				}
				else text = "Hosting mode deactivated! Auto-collect takes all drops again.";
			}
			else {
				text = "Hosting mode activated! Auto-collect takes only DL and BGL, and the pot is tracked.";
				Player.hosting = true;
				if (!Player.autocollect.running) {
					Player.EnsureCollectRunning(CLIENT, SERVER);
					Player.hostOwnsWorker = true;
				}
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/hosttax" || Packet::Contains(command, "/hosttax ")) {
			std::string text;
			if (command == "/hosttax") {
				text = "Host tax is " + std::to_string(Player.hostTaxPercent) + "%. Usage: /hosttax <0 - 100>";
			}
			else {
				std::string rest = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
				int pct = -1;
				try { pct = std::stoi(rest); } catch (...) { pct = -1; }
				if (pct < 0 || pct > 100) {
					text = "Invalid tax. Usage: /hosttax <0 - 100>";
				}
				else {
					Player.hostTaxPercent = pct;
					text = "Host tax set to " + std::to_string(pct) + "%.";
				}
			}

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/pot") {
			Player.HostShowPot(CLIENT);
			return false;
		}

		else if (command == "/pay" || Packet::Contains(command, "/pay ")) {
			if (command == "/pay") {
				GamePacket<OnConsoleMessage> p;
				p.text = "Usage: /pay <player name>";
				SendGamePacket(CLIENT, p);
			}
			else {
				std::string name = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
				Player.HostPay(name, CLIENT, SERVER);
			}
			return false;
		}

		else if (command == "/hostreset") {
			Player.HostReset();

			{
				GamePacket<OnConsoleMessage> p;
				p.text = "Pot cleared.";
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w Pot cleared.";
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

        return true;
    }
};