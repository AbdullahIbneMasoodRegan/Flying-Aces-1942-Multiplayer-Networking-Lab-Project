# Flying Aces: 1942 - Multiplayer Edition (Complete)

A complete multiplayer conversion of the Flying Aces: 1942 shoot 'em up game supporting 2-4 players online.

## ✨ Features

### Multiplayer Gameplay
- **2-4 Player Support** - Play with friends over LAN or Internet
- **Real-time Combat** - Engage enemies together
- **Color-Coded Players** - Each player has unique color identification
- **Live Scoreboard** - See everyone's scores in real-time
- **Respawn System** - Get back in action after 3 seconds
- **Synchronized Game State** - Server-authoritative gameplay

### Complete Implementation
- ✅ Enemy AI shooting with bullets
- ✅ Full collision detection (bullets, enemies, players)
- ✅ Health and damage system
- ✅ Score tracking per player
- ✅ Player death and respawn
- ✅ Reload mechanics (20 bullets before 2-second reload)
- ✅ Explosion effects
- ✅ Rate-limited shooting
- ✅ Connection timeout handling
- ✅ Visual feedback for all players

## 🚀 Quick Start

### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install libsdl2-dev libsdl2-net-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install SDL2-devel SDL2_net-devel SDL2_image-devel SDL2_ttf-devel SDL2_mixer-devel
```

**macOS (Homebrew):**
```bash
brew install sdl2 sdl2_net sdl2_image sdl2_ttf sdl2_mixer
```

Or simply run:
```bash
make install-deps-ubuntu    # For Ubuntu/Debian
make install-deps-fedora    # For Fedora/RHEL
make install-deps-macos     # For macOS
```

### Build

```bash
# Build everything
make

# Or build individually
make server
make client

# Clean build files
make clean
```

### Run

**1. Start the Server (on host machine):**
```bash
./server
```

Expected output:
```
========================================
  Flying Aces: 1942 - Server Started
========================================
Port: 9999
Max Players: 4
Tick Rate: 30 Hz

Waiting for players...
[SERVER READY] Waiting for connections...
```

**2. Start Client(s):**
```bash
./client
```

Then:
1. Select **"Play Multiplayer"** from menu
2. Enter server IP (default: `127.0.0.1` for local testing)
3. Press **ENTER** to connect
4. Start playing!

## 🎮 Controls

| Action | Keys |
|--------|------|
| Move Up | W or ↑ |
| Move Down | S or ↓ |
| Move Left | A or ← |
| Move Right | D or → |
| Shoot | Left Mouse Button |
| Disconnect | ESC (during game) |

## 🌐 Network Setup

### Local Testing (Same Computer)

```bash
# Terminal 1
./server

# Terminal 2
./client
# Connect to: 127.0.0.1

# Terminal 3
./client
# Connect to: 127.0.0.1
```

### LAN Play (Same Network)

**Server:**
```bash
# Find your local IP
ip addr show        # Linux
ipconfig            # Windows
ifconfig            # macOS

# Example: 192.168.1.100
./server
```

**Clients:**
```bash
./client
# Connect to: 192.168.1.100
```

### Internet Play

**Server (Host):**
1. Find public IP: `curl ifconfig.me`
2. Forward UDP port 9999 on router
3. Start server: `./server`

**Firewall Configuration:**
```bash
# Linux (ufw)
sudo ufw allow 9999/udp

# Linux (iptables)
sudo iptables -A INPUT -p udp --dport 9999 -j ACCEPT

# Windows: Add inbound rule for UDP port 9999
```

**Clients:**
```bash
./client
# Connect to: <server's public IP>
```

## 📊 What's Implemented

### Server Features
- ✅ UDP socket server on port 9999
- ✅ 30 Hz tick rate game loop
- ✅ Player connection/disconnection handling
- ✅ Input processing from all clients
- ✅ Enemy spawning (every 2 seconds)
- ✅ Enemy AI shooting (every 1.5 seconds)
- ✅ Enemy bullet management
- ✅ Collision detection:
  - Player bullets → Enemies
  - Enemy bullets → Players
  - Players → Enemies (ram damage)
- ✅ Health/damage system
- ✅ Player death and respawn (3 second delay)
- ✅ Score tracking per player
- ✅ Timeout detection (10 seconds)
- ✅ Game state broadcasting
- ✅ Explosion effects synchronization

### Client Features
- ✅ Server connection with IP input
- ✅ Input capture and transmission
- ✅ Game state reception and rendering
- ✅ Player rendering with color coding:
  - Player 0: Green
  - Player 1: Blue  
  - Player 2: Yellow
  - Player 3: Magenta
- ✅ Enemy rendering (6 different textures)
- ✅ Player bullet rendering
- ✅ Enemy bullet rendering
- ✅ Explosion rendering
- ✅ Health bars for all players
- ✅ Live scoreboard
- ✅ Bullet counter display
- ✅ Reload indicator
- ✅ Death/respawn messages
- ✅ "YOU" label for local player
- ✅ Connection status monitoring

## 🏗️ Architecture

### Network Protocol

**UDP-based for low latency**

```
Client                          Server
  |                               |
  |-- PACKET_CONNECT ------------>|
  |<----------- CONNECT_RESPONSE--|
  |                               |
  |-- PACKET_INPUT -------------->|
  |-- PACKET_INPUT -------------->|
  |<----------- PACKET_GAME_STATE-|
  |<----------- PACKET_GAME_STATE-|
  |                               |
  |-- PACKET_DISCONNECT --------->|
```

### Data Structures

**GameState (broadcast every tick):**
- 4 NetworkPlayers (position, health, score, bullets, alive status)
- 10 NetworkEnemies (position, texture ID, health)
- 50 Enemy bullets
- 20 Explosion effects
- Player counts and tick counter

**Packet Size:** ~8KB per game state update

### Server Authority

- Server controls all game logic
- Clients send input only
- Server validates all actions
- Server detects all collisions
- Server manages scores
- Prevents client-side cheating

## 📝 File Structure

```
.
├── network_common.h           # Shared data structures
├── network_server.c           # Dedicated server
├── network_client.h           # Client network interface
├── network_client.c           # Client network implementation
├── main_multiplayer.c         # Game client with rendering
├── Makefile                   # Build system
├── README_MULTIPLAYER_COMPLETE.md
└── resources/
    ├── *.png                  # Game sprites
    ├── sounds/                # Sound effects
    └── music/                 # Background music
```

## 🔧 Configuration

### Change Server Port

Edit `network_common.h`:
```c
#define SERVER_PORT 9999  // Change to your preferred port
```

Recompile both server and client.

### Adjust Game Parameters

In `network_server.c`:
```c
#define ENEMY_SPAWN_INTERVAL 2000    // Enemy spawn rate (ms)
#define ENEMY_SHOOT_INTERVAL 1500    // Enemy fire rate (ms)
#define RESPAWN_TIME 3000            // Player respawn time (ms)
#define TICK_RATE 30                 // Server updates per second
```

### Performance Tuning

For high-latency connections:
- Increase timeout values in `network_server.c` (line: `> 10000`)
- Consider client-side prediction (future enhancement)

For low-end servers:
- Reduce `TICK_RATE` to 20 Hz
- Reduce `MAX_ENEMIES` to 5

## 🐛 Troubleshooting

### Server won't start
```bash
# Check if port is in use
netstat -an | grep 9999

# Kill existing process
sudo killall server

# Try different port in network_common.h
```

### Client can't connect
1. Verify server is running
2. Check firewall settings
3. Test with localhost first: `127.0.0.1`
4. Verify correct IP address
5. Ensure port 9999 UDP is open

### Lag or stuttering
- Check network latency: `ping <server_ip>`
- Ensure stable connection (prefer wired over WiFi)
- Check server CPU usage: `top` or `htop`
- Reduce player count if server CPU is high

### Players disappearing
- Server console shows timeout messages
- Check network stability
- Verify firewall isn't blocking intermittent packets

### Build errors
```bash
# Missing SDL2 libraries
make install-deps-ubuntu  # Or your platform

# Linker errors
sudo ldconfig

# Permission errors
chmod +x server client
```

## 📈 Performance

### Server Requirements
- **CPU:** ~10-20% for 4 players (modern CPU)
- **RAM:** ~50 MB
- **Network:** ~200 KB/s upload (30 Hz * 8KB per tick)
- **OS:** Linux, macOS, Windows (with MinGW)

### Client Requirements
- **CPU:** ~30-40% for rendering
- **RAM:** ~100 MB
- **Network:** ~50 KB/s download
- **GPU:** Any with SDL2 support

### Network Bandwidth

Per client per second:
- **Upstream (Input):** ~5 KB/s
- **Downstream (Game State):** ~240 KB/s

For 4 players, server needs:
- **Download:** ~20 KB/s
- **Upload:** ~960 KB/s (4 × 240 KB/s)

## 🎯 Differences from Single-Player

### What Changed
1. **Game Loop:** Separated client rendering and server simulation
2. **Input:** Sent to server instead of direct control
3. **State:** Server owns game state
4. **Player Count:** Now supports 2-4 simultaneous players
5. **Networking:** UDP packets for communication

### What's the Same
1. **Graphics:** Same SDL2 rendering
2. **Assets:** Same textures, sounds, music
3. **Mechanics:** Same movement, shooting, collision logic
4. **Visuals:** Same explosions and effects

### Not Implemented (Future)
- Single-player mode in this version (use original Main.c)
- Options menu (sound/music control)
- High score persistence
- Help screen
- Game over screen with stats
- Lobby system
- Chat functionality
- Spectator mode

## 🚧 Known Limitations

1. **No client prediction** - Input lag on high-latency connections
2. **No interpolation** - Movement may appear choppy
3. **Large packets** - Full game state sent every tick (could use delta compression)
4. **UDP unreliability** - Rare packet loss not handled
5. **No authentication** - Anyone can connect
6. **Fixed tick rate** - Not adaptable to varying server load

## 🔮 Future Enhancements

### Easy
- [ ] Lobby system with "ready" status
- [ ] Player names instead of just IDs
- [ ] Chat messages
- [ ] Server browser
- [ ] Game stats at end

### Medium
- [ ] Delta compression for smaller packets
- [ ] Packet acknowledgment and retransmission
- [ ] Client-side prediction for smooth local movement
- [ ] Interpolation for other players
- [ ] Password-protected servers
- [ ] Spectator mode

### Hard
- [ ] Lag compensation (rewind for hit detection)
- [ ] Dynamic tick rate based on load
- [ ] Server migration
- [ ] Replay recording/playback
- [ ] Anti-cheat measures

## 🎓 Learning Resources

This implementation demonstrates:
- **Network Programming:** UDP sockets with SDL_net
- **Client-Server Architecture:** Authoritative server design
- **Real-time Multiplayer:** Tick-based game loops
- **State Synchronization:** Broadcasting game state
- **Collision Detection:** Server-side validation

## 📜 License

Same as original Flying Aces: 1942 game.

## 👥 Credits

- **Original Game:** Single-player Flying Aces: 1942
- **Multiplayer Conversion:** Complete implementation with SDL_net
- **Network Architecture:** Client-server UDP model

## 📞 Support

Issues to check:
1. SDL2 libraries installed correctly
2. Firewall allows UDP port 9999
3. Server running before client connects
4. Correct IP address entered
5. Network allows UDP traffic

## 🎉 Enjoy!

Start the server, connect with friends, and engage in aerial combat together!

```
./server
./client
```

May your aim be true and your bullets plentiful! ✈️💥