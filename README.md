# AdmaProxy

by Unicornlite (https://github.com/unicornlite)

Based on GrowtopiaProxy by batutekoz77.

AdmaProxy is a bridge proxy for Growtopia on Windows. It sits between the game
client and the server, so it can inspect, modify, and inject packets in real
time. Everything is controlled from inside the game with slash commands: type
`/proxy` to open the command menu.

## How it works

- The proxy runs on the same PC as the game. On start it redirects the
  client's connection to itself (via the hosts file, restored automatically
  when the proxy exits), fetches the real server address, and forwards traffic
  both ways: client to proxy to server, and server to proxy to client.
- A built-in HTTP server handles the server-address lookup, and the game
  traffic itself runs over ENet.
- It needs administrator rights (it relaunches itself as admin on start) and
  it closes Growtopia first if the game is already running, so start the proxy
  before launching the game.
- Packet logging is built in. What gets logged is tuned in
  `Source/Main/Config.hpp` (`ShouldLogNetMessage`, `BASIC_LOGS`).

## Commands

### Info

- `/proxy` - Opens the command menu.
- `/ping` - Shows your client and server ping.

### Customize

- `/ui` - Opens the UI customizer menu.
- `/country <code>` - Changes your country flag.

### Main features

- `/farm` - Starts/stops auto farming.
- `/spam` - Opens the spam settings menu.
- `/wrench` - Toggles auto wrench (left click pulls, right click kicks).
- `/antimod` - Auto-leaves the world the moment a moderator enters
  (mstate/smstate detection in OnSpawn).
- `/antigravitasi` - Floats in place at the current height instead of falling.
  Alias: `/antigravity`.

### World tools

- `/collect` - Automatically walks to nearby dropped items and picks them up.
  Nearest drop first. With `/antigravitasi` on, only X moves.
- `/zoom` - Unlimited zoom (client-side only, via local OnSpawn mstate).
  Turning it off restores normal zoom.
- `/copyworld` - Saves the current world layout (foreground/background per
  tile) to `world_<NAME>.txt` in the proxy folder.
- `/pasteworld [name]` - Rebuilds a saved world layout here, placing blocks
  from your inventory one by one. Without a name it pastes the last copied
  world. Run again to stop.
- `/copymusic` - Saves the world song (all sheet-music background tiles:
  position is time/pitch, item is the instrument) to `song_<NAME>.txt`.
- `/pastemusic [name]` - Rebuilds a saved song here. Same rules as
  `/pasteworld`.
- `/tp <x> <y>` - Teleports to tile coordinates.
- `/tp <player>` - Teleports to a player's last known position.
- `/tp` - Teleports to the point saved with `/tpset`.
- `/tpset` - Saves the current position as the teleport point.
- `/doormark` - Shows open doors as green blocks and closed doors as red
  blocks. It applies right away; turning it off restores the
  view right away, no rejoin needed.

### Automation

- `/surg` - Auto surgery. Answers the tool prompts in surgery dialogs.
  Start surgery normally (wrench the patient); the proxy clicks the tools.
- `/fish` - Auto fishing. Casts the bait, reels when a fish bites, recasts
  after a catch. Equip the Fishing Rod and stand next to water.

### Movement

- `/pathfind <x> <y>` - Walks to tile coordinates along a path (A* over the
  tile map). Run again with no arguments to stop.
- `/pfind <item>` - Walks to the nearest drop with that name.
- `/speed [number]` - Walks faster. Default 1.5x; `/speed 2` sets 2x
  (0.5 to 5). Plain `/speed` toggles it.
- `/longpunch` - Punches and places from up to 10 tiles away.
- `/noclip` - Walks through walls. Push against a wall to phase through it.

### Vision

- `/esp` - Marks drops with yellow blocks and players with aqua blocks on
  your screen. This is not a real overlay; the proxy cannot draw on the
  game screen.
- `/espitem` - Lists drops in this world with distance and direction.
- `/espplayer` - Lists players in this world with distance and direction.
- `/xray` - Hides foreground blocks so you can see through walls. Toggle
  restores the normal view right away.
- `/spininfo` - Shows whether a roulette spin is real or fake, plus the
  REME number. Info only; the result is decided by the server.

### Casino

- `/bj` - Blackjack gems counter. When a Chandelier breaks, the gems that
  come out are counted and shown as the blackjack number (over 21 is a
  bust). `/bjlog` shows the last 10 results.
- `/host` - Casino hosting mode. Auto-collect takes only Diamond Locks
  and Blue Gem Locks, and the pot is tracked. Turn it off to stop.
- `/hosttax <percent>` - Sets the host tax (default 10%).
- `/pot` - Shows the current pot (DL + BGL + total value), the tax, and
  the payout.
- `/pay <player>` - Pays the winner the pot minus tax, BGL first then
  DL. The winner must be in this world.
- `/hostreset` - Empties the pot (after paying, or to start a new round).

### Local look

These change only what you see on your own screen. Other players still
see your real name and clothes.

- `/nick <text>` - Shows a custom name above your character, e.g.
  `/nick Adma of Legend`. `/nickreset` restores your real name.
- `/vset <slot> <itemID>` - Shows a different item in one clothing slot,
  e.g. `/vset hair 2`. Slots: hair, shirt, pants, feet, face, hand,
  back, mask, necklace. `/vset <slot> off` restores that slot, plain
  `/vset` lists the current overrides, `/vsetreset` clears them all.

### Information

- `/balance` - Shows your current lock balance.
- `/account` - Opens the account information menu.
- `/inventory` - Opens the inventory menu.

### Shortcuts

- `/drop` - Toggles fast drop.
- `/trash` - Toggles fast trash.
- `/pullall` - Pulls everyone in the world.
- `/kickall` - Kicks everyone in the world.
- `/banall` - Bans everyone in the world.
- `/warp <name>` - Warps to a world.
- `/back` - Warps to the last world.
- `/setsave <name>` - Sets the save world.
- `/save` - Warps to the save world.
- `/relog` - Leaves and rejoins the same world.
- `/change` - Toggles fast BGL change.

### Drop

- `/cd <amount>` - Drops that many locks.
- `/wd <amount>` - Drops that many World Locks.
- `/dd <amount>` - Drops that many Diamond Locks.
- `/bd <amount>` - Drops that many Blue Gem Locks.
- `/daw` - Drops all your locks.

### Anti lag

- `/hideclothes` - Hides everyone's clothes and removes their effects.

### Built-in extras

- Roulette verifier: tags real server roulette results in chat.
- Custom dialog colors.
- Fast forwarding: packets are relayed without added delay.

## World tools notes

- **Teleport hotkey:** hold **F8** and punch a tile with your fist to teleport
  to it. The punch packet is dropped, so the block is not hit. Change the key
  in `Source/Main/Config.hpp` (`Main.TELEPORT_HOTKEY`, a Windows virtual-key
  code, `0x77` = F8).
- **Teleport, auto-collect, and paste need in-game testing:** they assume the
  server accepts the proxy's position and placement packets the same way a
  private server (Gurotopia) does. If the server rubber-bands or rejects them,
  the feature will visibly fail rather than silently misbehave.
- **Copy world is foreground/background only (v1):** locks, signs, doors, and
  other tile extras are not copied. The file format is plain text: a header
  line, the world size, then one `x y fg bg` line per non-empty tile.
- **Copy music copies sheet music, not note blocks:** the world song is the
  sheet-music layout (X = time, Y = pitch, item = instrument). The Note Block
  item (220) plays one fixed song and has no pitch, so it is not part of the
  song.
- **Door marker** replaces door tiles with solid green (open) and red (closed)
  blocks on your screen only. It applies right away; turning it off
  resends the original map to restore the view.
- **Unlimited zoom** only changes what your client renders. The server still
  treats you as a normal player.
- **Map parsing limits:** the proxy parses the world tile map and drop list
  from the map packet. Tiles with an unknown extra-data type stop the parse
  with a warning in the proxy log; features using the map then work on partial
  data until you rejoin.

## Automation, movement and vision notes

- **Auto surgery** reads the patient status from the surgery dialog text
  and answers with the matching tool. The symptom-to-tool rules come from
  a field-used auto-surgery implementation written for a private server,
  and match the official surgery guide's dialog flow, but they are not
  verified against a live capture from the official server. The player
  must own the tools; the proxy stops with a message when a needed tool
  is missing.
- **Auto fishing** casts with a plain item use (Wiggly Worm on a water
  tile, found from the map), detects the bite from the splash sound
  packet, and reels by casting again. The drill (Hand Drill on Ice Crust
  Block) and detonate (Nuclear Detonator on Uranium Block) steps are also
  plain item uses; that packet form is not verified against a capture, so
  it needs in-game testing. The `PACKET_GONE_FISHIN` packet (type 31) is
  not used by the real fishing flow.
- **Pathfind** runs A* over the parsed tile map. A tile is walkable when
  it has no foreground block, or its block has no collision (or is a
  platform) in items.dat; unknown items count as solid. The path may climb
  through open air, because the proxy moves the player along it with
  position overrides rather than simulating jumps. Needs in-game testing.
- **Speed** scales the movement delta in outgoing packets. The server may
  reject movement that is too fast (rubber-band). Needs in-game testing.
- **Long punch** extends punch coordinates to 10 tiles out. The server
  may validate the distance. Needs in-game testing.
- **Noclip** nudges the player through a wall while pushing against it.
  Jumps can go slightly higher while it is on (at the jump apex the
  vertical speed briefly reads as zero while the jumping flag is still
  set). Needs in-game testing.
- **ESP** is not a real overlay: it replaces air tiles with bright blocks
  (yellow for drops, aqua for players) and prints lists to the console.
  Markers only sit on air tiles, because a real block's extra data cannot
  be restored from a tile update.
- **X-ray** hides foreground blocks client-side by rewriting the map; tile
  updates stay hidden while it is on. Toggle restores the original view.
- **Spin info:** a real spin is a server talk bubble with no `OID:`
  marker; a fake spin is player chat shaped like a result, and chat
  carries `OID:`. REME is computed from the number: 0, 19 and 28 give 0,
  otherwise the last digit of tens + ones (14 gives 5, 18 gives 9). QEME
  is the last digit of the number. Info only; the result is decided by
  the server and cannot be changed.

## Blackjack counter notes

- **How it counts:** the proxy watches tile updates for a Chandelier
  (item 340) disappearing, then sums the amounts of the Gems drops
  (item 112) that appear on or next to that tile within 2.5 seconds.
  Gems are split into several drops (10s, 5s, 1s), so the total is the
  sum of every drop's amount. A drop collected before the report still
  counts: the number is the gems that came out, not the gems left.
- **The result** shows in the console and, when the breaker is known,
  as a talk bubble from that player. Over 21 is marked BUST, exactly 21
  is BLACKJACK. `/bjlog` shows the last 10 results.
- **The breaker** is read from the tile update's netid field. That
  matches how the open reimplementation builds the packet (from the
  punch state), but it is not verified against a live capture from the
  official server. If the netid is unknown, the result shows without a
  name and without a talk bubble.
- **No tile-attached text exists:** no variant in any surveyed source
  draws text at a tile position, so the console + talk bubble is the
  display. Breaks reported through multi-tile updates (type 6) are not
  seen yet, like the rest of the map tracking.

## Casino hosting notes

- **The pot counts what the host actually collected.** A drop counts
  when its remove packet arrives and the host is the collector (the
  remove packet names the collector in the open reimplementation; if
  the official server does not, the proxy falls back to drops its own
  collect worker asked for). Drops someone else picks up never count.
- **Totals only, no per-player bets.** The drop packet carries no
  player info: a new drop arrives with netid -1, and dropping only
  broadcasts that one packet. There is no verified way to tell who
  dropped a lock, so the pot is tracked as totals (DL + BGL).
- **Value and tax:** 1 BGL counts as 100 DL. Tax is a percentage of
  the total pot, default 10%, set with `/hosttax <percent>`.
- **Payout rounding:** `/pay` pays BGL first, then DL. A BGL cannot be
  split, so when the payout does not divide evenly the payout is
  rounded down and the remainder goes to the tax. `/pot` shows the
  split remainder when it applies. `/pay` clears the pot once the
  drops are sent, so the same pot cannot be paid twice.
- **The host must be in the same world** as the players and able to
  pick up the drops. `/pay` drops the locks at the host's feet; the
  winner picks them up. The winner must be in the world when paid.

## Local look notes

- **Local only, by design.** The name and clothes other players see are
  sent by the server from account data, so the proxy cannot change
  them. `/nick` rewrites the `name` field of your own client-bound
  OnSpawn packet, and `/vset` rewrites the clothing item IDs in your
  own client-bound OnSetClothing packet. Nobody else sees any of it.
- **The name in other places does not change.** The name in the screen
  corner, in dialogs, and in chat comes from login data, not from
  OnSpawn, so it still shows your real name.
- **How the visual set works:** clothes arrive as three triples of
  item IDs (hair/shirt/pants, feet/face/hand, back/mask/necklace).
  Each `/vset` overrides one slot; the rest keep showing your real
  clothes. The overrides re-apply on every clothing update, so they
  survive world changes. Skin color is not covered.
- **The nick applies on every spawn,** so it survives world changes
  too. Turning zoom off and on keeps the nick: all spawn rewrites
  (country, zoom, nick) go through one resend path.

## Screenshots

![Info, Customize, Main Features](Docs/Proxy1.png)
![Information and Shortcuts](Docs/Proxy2.png)
![Drop Features and Anti Lag](Docs/Proxy3.png)

![UI Menu](Docs/UiMenu.png)
![Spam Menu](Docs/SpamMenu.png)

## Building

### Requirements

- Windows 10/11
- Visual Studio 2026 Community with the C++ workload
- Git
- vcpkg (only for OpenSSL; `enet.lib` is bundled in this repo)

### Steps

1. Clone this repo:
   ```
   git clone https://github.com/unicornlite/AdmaProxy.git
   ```
2. Set up vcpkg and install OpenSSL:
   ```
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .bootstrap-vcpkg.bat
   .vcpkg install openssl:x64-windows
   .vcpkg integrate install
   ```
3. Open `GrowtopiaProxy.slnx` in Visual Studio.
4. Make sure vcpkg is enabled for the project (Project -> Properties ->
   vcpkg -> Use Vcpkg: Yes).
5. Build the `x64/Release` configuration (`Ctrl + Shift + B`).
6. Run `x64/Release/Source.exe`. It asks for administrator rights on start.

The project targets C++23 (`/std:c++23preview`) and C17 (`/std:c17`).

## Configuration

All settings live in `Source/Main/Config.hpp`:

- `Main.NAME` / `Main.VERSION` - proxy name and version shown in-game.
- `Main.TELEPORT_HOTKEY` - teleport hotkey, a Windows virtual-key code
  (`0x77` = F8). Hold it and punch a tile with your fist to teleport there.
- `Local` - address the proxy listens on (`127.0.0.1`, TCP 443, UDP 17123).
- `Server` - leave `IP` empty and `UDP` at -1 to use the real Growtopia
  server (address fetched automatically).
- `Client` - client version and protocol the proxy reports (5.61 / 225).
- `ShouldLogNetMessage` - which packet types get logged.

## Running

1. Start `Source.exe` (admin). It closes Growtopia first if the game is
   already running.
2. Launch Growtopia and log in as usual. Traffic now goes through the proxy.
3. Type `/proxy` in game to open the command menu.
4. Close the proxy when done; it restores the hosts file automatically.

## Notes and limits

- Teleport, auto-collect, and paste assume the server accepts the proxy's
  position and placement packets the same way a private server does. If the
  server rubber-bands or rejects them, the feature visibly fails instead of
  silently misbehaving. In-game testing is needed.
- Unlimited zoom and door marker only change what your client renders.
- Use at your own risk: proxies and modified clients can be detected by the
  game operator. No ban-safety claim is made here.

## Credits

- Unicornlite - author and maintainer
- ENet - low-latency networking library
- OpenSSL - HTTPS handling in the built-in HTTP server
- vcpkg - dependency management

## License

This project is licensed under the MIT License - see the LICENSE file for
details.
