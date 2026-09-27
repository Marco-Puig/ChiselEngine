#pragma once

#include <cstdint>

namespace net {

constexpr uint32_t MaxPlayers = 3;

enum class Role : uint8_t {
    Disconnected = 0,
    Host = 1,
    Client = 2
};

using PlayerSlot = uint8_t;

constexpr PlayerSlot InvalidSlot = 255;
constexpr PlayerSlot HostSlot = 0;

using SteamPlayerId = uint64_t;

constexpr SteamPlayerId InvalidSteamId = 0;

enum class Channel : int {
    Reliable = 0,
    Unreliable = 1
};

}