#pragma once

#include "NetworkDefs.h"

#include <functional>
#include <string>
#include <vector>

namespace net {

class NetworkManager {
public:
    static NetworkManager& getInstance();

    bool init();
    void shutdown();

    bool hostPrivateLobby(std::string& outLobbyCode);
    bool joinPrivateLobby(const std::string& lobbyCode);

    void update(float deltaTime);

    bool isInitialized() const { return m_initialized; }
    bool isConnected() const { return m_connected; }
    bool isHost() const { return m_role == Role::Host; }

    Role getRole() const { return m_role; }
    PlayerSlot getLocalPlayerSlot() const { return m_localSlot; }
    uint32_t getPlayerCount() const { return static_cast<uint32_t>(m_players.size()); }

    void broadcastReliable(const void* data, size_t size);
    void broadcastUnreliable(const void* data, size_t size);

    void sendReliable(PlayerSlot slot, const void* data, size_t size);
    void sendUnreliable(PlayerSlot slot, const void* data, size_t size);

    using MessageCallback = std::function<void(
        PlayerSlot fromSlot,
        const void* data,
        size_t size,
        bool reliable
    )>;

    void setMessageCallback(MessageCallback callback) {
        m_messageCallback = std::move(callback);
    }
    
    void showInviteDialog();
    void joinLobbyBySteamId(uint64_t steamLobbyId);
    void finalizeLobbyJoin();
    void checkForPendingInvite(int argc, char** argv);

private:
    NetworkManager();
    ~NetworkManager();

    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    struct RemotePlayer {
        PlayerSlot slot = InvalidSlot;
        SteamPlayerId steamId = InvalidSteamId;
        std::string name;
    };

    void handleReceivedData(
        SteamPlayerId fromSteamId,
        const void* data,
        size_t size,
        bool reliable
    );

    void syncPlayersFromLobby();

    PlayerSlot findSlotBySteamId(SteamPlayerId steamId) const;
    SteamPlayerId findSteamIdBySlot(PlayerSlot slot) const;
    PlayerSlot allocateFreeSlot() const;

    bool m_initialized = false;
    bool m_connected = false;

    Role m_role = Role::Disconnected;
    PlayerSlot m_localSlot = InvalidSlot;

    std::vector<RemotePlayer> m_players;

    MessageCallback m_messageCallback;
};

}