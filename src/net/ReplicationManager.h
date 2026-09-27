#pragma once

#include "NetMessage.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

class Node;
class Scene;

namespace net {

class ReplicationManager {
public:
    static ReplicationManager& getInstance();

    void initialize(Scene* scene);
    void shutdown();

    // Called once per frame from Engine or Game.
    void update(float deltaTime);

    // Assigns a stable network ID to a node.
    uint32_t assignNetworkId(Node* node);

    // Finds a node from its network ID.
    Node* findByNetworkId(uint32_t networkId) const;

    // Host-side replication commands.
    void spawnNodeOnClients(Node* node);
    void despawnNodeOnClients(Node* node);

    // Lua gameplay event replication.
    void sendLuaEvent(const std::string& eventName, const std::string& payload);

private:
    ReplicationManager() = default;

    ReplicationManager(const ReplicationManager&) = delete;
    ReplicationManager& operator=(const ReplicationManager&) = delete;

    void handleNetMessage(
        PlayerSlot fromSlot,
        const void* data,
        size_t size,
        bool reliable
    );

    void sendTransformSnapshots();
    void applyTransformSnapshots(float deltaTime);

    Scene* m_scene = nullptr;

    uint32_t m_nextNetworkId = 1;
    uint32_t m_sceneVersion = 0;

    std::unordered_map<uint32_t, Node*> m_networkIdToNode;
    std::unordered_map<Node*, uint32_t> m_nodeToNetworkId;

    struct TransformTarget {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        bool active = false;
    };

    std::unordered_map<uint32_t, TransformTarget> m_transformTargets;

    float m_transformSendTimer = 0.0f;
};

}