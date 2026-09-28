#include "ReplicationManager.h"
#include "NetworkManager.h"

#include "scene/Node.h"
#include "scene/Scene.h"
#include "scene/MeshNode.h"
#include "scene/PlayerAvatarNode.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstring>
#include <iostream>

#include <stdexcept>
#include <string>

namespace net {


// Singleton


ReplicationManager& ReplicationManager::getInstance() {
    static ReplicationManager instance;
    return instance;
}


// Lifecycle


void ReplicationManager::initialize(Scene* scene) {
    m_scene = scene;

    NetworkManager::getInstance().setMessageCallback(
        [this](
            PlayerSlot fromSlot,
            const void* data,
            size_t size,
            bool reliable
        ) {
            handleNetMessage(fromSlot, data, size, reliable);
        }
    );

    m_lastPlayerCount = 0;
}

void ReplicationManager::shutdown() {
    m_scene = nullptr;
    m_networkIdToNode.clear();
    m_nodeToNetworkId.clear();
    m_transformTargets.clear();
    m_nextNetworkId = 1;
    m_sceneVersion = 0;
    m_transformSendTimer = 0.0f;
}


// Per-frame update


void ReplicationManager::update(float deltaTime) {
    if (m_scene == nullptr) {
        return;
    }

    auto& network = NetworkManager::getInstance();

    if (!network.isConnected()) {
        return;
    }

    if (network.isHost()) {
        // If the player count changed, resend spawn messages for existing nodes.
        // This helps late joiners receive replicated objects.
        uint32_t playerCount = network.getPlayerCount();

        if (playerCount != m_lastPlayerCount) {
            m_lastPlayerCount = playerCount;

            if (playerCount > 1) {
                resendAllSpawnNodes();
            }
        }

        // Existing transform snapshot logic.
        constexpr float transformSendRate = 20.0f;
        constexpr float transformSendInterval = 1.0f / transformSendRate;

        m_transformSendTimer += deltaTime;

        if (m_transformSendTimer >= transformSendInterval) {
            m_transformSendTimer = 0.0f;
            sendTransformSnapshots();
        }
    } else {
        // Client applies received transform snapshots.
        applyTransformSnapshots(deltaTime);
    }
}


// Network ID management


uint32_t ReplicationManager::assignNetworkId(Node* node) {
    if (node == nullptr) {
        return 0;
    }

    // Check if this node already has a network ID.
    auto it = m_nodeToNetworkId.find(node);
    if (it != m_nodeToNetworkId.end()) {
        return it->second;
    }

    const uint32_t networkId = m_nextNetworkId++;

    m_networkIdToNode[networkId] = node;
    m_nodeToNetworkId[node] = networkId;

    // Also set the network ID on the node itself.
    node->setNetworkId(networkId);

    return networkId;
}

Node* ReplicationManager::findByNetworkId(uint32_t networkId) const {
    auto it = m_networkIdToNode.find(networkId);
    if (it != m_networkIdToNode.end()) {
        return it->second;
    }
    return nullptr;
}


// Host-side spawn / despawn


void ReplicationManager::spawnNodeOnClients(Node* node) {
    if (node == nullptr) {
        return;
    }

    auto& network = NetworkManager::getInstance();

    if (!network.isHost()) {
        return;
    }

    // Ensure the node has a network ID.
    uint32_t networkId = assignNetworkId(node);

    SpawnNodeMessage message{};
    message.header.type = MessageType::SpawnNode;
    message.header.sequence = 0;
    message.header.senderSlot = network.getLocalPlayerSlot();

    message.networkId = networkId;

    // Copy node name.
    const std::string& name = node->getName();
    std::strncpy(message.name, name.c_str(), sizeof(message.name) - 1);
    message.name[sizeof(message.name) - 1] = '\0';

    // If this is a MeshNode, try to get the GLB path.
    // For now, leave it empty since MeshNode doesn't store the path.
    message.glbPath[0] = '\0';

    // Copy transform.
    const glm::vec3 position = node->getPosition();
    const glm::quat rotation = node->getRotation();
    const glm::vec3 scale = node->getScale();

    message.position[0] = position.x;
    message.position[1] = position.y;
    message.position[2] = position.z;

    message.rotation[0] = rotation.x;
    message.rotation[1] = rotation.y;
    message.rotation[2] = rotation.z;
    message.rotation[3] = rotation.w;

    message.scale[0] = scale.x;
    message.scale[1] = scale.y;
    message.scale[2] = scale.z;

    // Physics description (default values for now).
    message.bodyType = 0;       // no body
    message.colliderType = 0;   // none
    message.friction = 0.5f;
    message.restitution = 0.0f;

    network.broadcastReliable(&message, sizeof(message));

    // Increment scene version.
    ++m_sceneVersion;
}

void ReplicationManager::despawnNodeOnClients(Node* node) {
    if (node == nullptr) {
        return;
    }

    auto& network = NetworkManager::getInstance();

    if (!network.isHost()) {
        return;
    }

    // Find the network ID for this node.
    auto it = m_nodeToNetworkId.find(node);
    if (it == m_nodeToNetworkId.end()) {
        return;
    }

    const uint32_t networkId = it->second;

    DespawnNodeMessage message{};
    message.header.type = MessageType::DespawnNode;
    message.header.sequence = 0;
    message.header.senderSlot = network.getLocalPlayerSlot();
    message.networkId = networkId;

    network.broadcastReliable(&message, sizeof(message));

    // Remove from our maps.
    m_networkIdToNode.erase(networkId);
    m_nodeToNetworkId.erase(it);

    // Remove any pending transform targets.
    m_transformTargets.erase(networkId);

    // Increment scene version.
    ++m_sceneVersion;
}


// Lua gameplay events (placeholder for step 10)


void ReplicationManager::sendLuaEvent(
    const std::string& eventName,
    const std::string& payload
) {
    auto& network = NetworkManager::getInstance();

    LuaEventMessage message{};
    message.header.type = MessageType::LuaEvent;
    message.header.sequence = 0;
    message.header.senderSlot = network.getLocalPlayerSlot();

    // Copy event name.
    std::strncpy(message.eventName, eventName.c_str(), sizeof(message.eventName) - 1);
    message.eventName[sizeof(message.eventName) - 1] = '\0';

    // Copy payload.
    const size_t payloadSize = std::min(
        payload.size(),
        sizeof(message.payload) - 1
    );

    std::strncpy(message.payload, payload.c_str(), payloadSize);
    message.payload[payloadSize] = '\0';
    message.payloadSize = static_cast<uint32_t>(payloadSize);

    network.broadcastReliable(&message, sizeof(message));
}


// Network message handler


void ReplicationManager::handleNetMessage(
    PlayerSlot fromSlot,
    const void* data,
    size_t size,
    bool reliable
) {
    if (data == nullptr || size < sizeof(MessageHeader)) {
        return;
    }

    const auto* header = static_cast<const MessageHeader*>(data);

    switch (header->type) {

        // Spawn a replicated node (received by clients from host)
        case MessageType::SpawnNode: {
            if (NetworkManager::getInstance().isHost()) {
                break;
            }

            if (size < sizeof(SpawnNodeMessage)) {
                break;
            }

            const auto* message =
                static_cast<const SpawnNodeMessage*>(data);

            // If we already have this exact network ID, ignore it.
            if (findByNetworkId(message->networkId) != nullptr) {
                break;
            }

            Node* newNode = nullptr;

            std::string nodeName(message->name);

            // Reuse an existing node with the same name if present.
            // This can happen if Lua already created a placeholder avatar.
            if (m_scene != nullptr) {
                newNode = m_scene->findByName(nodeName);
            }

            // If this is a player avatar, create it.
            if (newNode == nullptr &&
                nodeName.rfind("PlayerAvatar_", 0) == 0) {

                int slot = 0;

                try {
                    slot = std::stoi(nodeName.substr(13));
                } catch (...) {
                    slot = 0;
                }

                glm::vec3 color(1.0f);

                if (slot == 0) {
                    color = glm::vec3(0.25f, 0.75f, 1.0f);
                } else if (slot == 1) {
                    color = glm::vec3(1.0f, 0.55f, 0.2f);
                } else {
                    color = glm::vec3(0.55f, 1.0f, 0.35f);
                }

                auto avatar = std::make_unique<PlayerAvatarNode>(
                    nodeName,
                    color
                );

                PlayerAvatarNode* rawAvatar = avatar.get();

                if (m_scene != nullptr) {
                    m_scene->adopt(std::unique_ptr<MeshNode>(avatar.release()));
                }

                newNode = rawAvatar;
            }

            if (newNode != nullptr) {
                // Remove old mapping if this node already had one.
                auto oldMapping = m_nodeToNetworkId.find(newNode);
                if (oldMapping != m_nodeToNetworkId.end()) {
                    m_networkIdToNode.erase(oldMapping->second);
                }

                newNode->setNetworkId(message->networkId);

                newNode->setPosition(glm::vec3(
                    message->position[0],
                    message->position[1],
                    message->position[2]
                ));

                newNode->setRotation(glm::normalize(glm::quat(
                    message->rotation[3],
                    message->rotation[0],
                    message->rotation[1],
                    message->rotation[2]
                )));

                newNode->setScale(glm::vec3(
                    message->scale[0],
                    message->scale[1],
                    message->scale[2]
                ));

                m_networkIdToNode[message->networkId] = newNode;
                m_nodeToNetworkId[newNode] = message->networkId;

                std::cout << "[Replication] Spawned replicated node: "
                        << message->name
                        << " (ID: " << message->networkId << ")" << std::endl;
            }

            break;
        }
        case MessageType::AvatarTransform: {
            if (!NetworkManager::getInstance().isHost()) {
                break;
            }

            if (size < sizeof(AvatarTransformMessage)) {
                break;
            }

            const auto* message =
                static_cast<const AvatarTransformMessage*>(data);

            PlayerSlot slot =
                (fromSlot != InvalidSlot) ? fromSlot : message->slot;

            if (slot == InvalidSlot || m_scene == nullptr) {
                break;
            }

            const std::string avatarName =
                "PlayerAvatar_" + std::to_string(slot);

            Node* node = m_scene->findByName(avatarName);

            if (node != nullptr) {
                node->setPosition(glm::vec3(
                    message->position[0],
                    message->position[1],
                    message->position[2]
                ));

                // Make sure it is registered for replication.
                if (node->getNetworkId() == 0) {
                    assignNetworkId(node);
                }
            }

            break;
        }
        
        case MessageType::DespawnNode: {
            if (NetworkManager::getInstance().isHost()) {
                break;
            }

            if (size < sizeof(DespawnNodeMessage)) {
                break;
            }

            const auto* message =
                static_cast<const DespawnNodeMessage*>(data);

            Node* node = findByNetworkId(message->networkId);
            if (node != nullptr) {
                // Remove from our maps.
                m_networkIdToNode.erase(message->networkId);
                m_nodeToNetworkId.erase(node);

                // Remove any pending transform targets.
                m_transformTargets.erase(message->networkId);

                // TODO: Remove the node from the scene.
                // You need to implement scene->removeNode(node)
                // or similar functionality.
            }

            break;
        }

        case MessageType::TransformSnapshot: {
            if (NetworkManager::getInstance().isHost()) {
                break;
            }

            if (size < sizeof(TransformSnapshotMessage)) {
                break;
            }

            const auto* message =
                static_cast<const TransformSnapshotMessage*>(data);

            TransformTarget target;

            target.position = glm::vec3(
                message->position[0],
                message->position[1],
                message->position[2]
            );

            target.rotation = glm::normalize(glm::quat(
                message->rotation[3],  // w
                message->rotation[0],  // x
                message->rotation[1],  // y
                message->rotation[2]   // z
            ));

            target.active = true;

            m_transformTargets[message->networkId] = target;

            break;
        }

        case MessageType::LuaEvent: {
            if (size < sizeof(LuaEventMessage)) {
                break;
            }

            const auto* message =
                static_cast<const LuaEventMessage*>(data);

            break;
        }

        default: {
            break;
        }
    }
}

void ReplicationManager::sendTransformSnapshots() {
    auto& network = NetworkManager::getInstance();

    if (!network.isHost()) {
        return;
    }

    for (const auto& pair : m_networkIdToNode) {
        const uint32_t networkId = pair.first;
        Node* node = pair.second;

        if (node == nullptr) {
            continue;
        }

        TransformSnapshotMessage message{};

        message.header.type = MessageType::TransformSnapshot;
        message.header.sequence = 0;
        message.header.senderSlot = network.getLocalPlayerSlot();

        message.networkId = networkId;

        const glm::vec3 position = node->getPosition();
        const glm::quat rotation = node->getRotation();

        message.position[0] = position.x;
        message.position[1] = position.y;
        message.position[2] = position.z;

        message.rotation[0] = rotation.x;
        message.rotation[1] = rotation.y;
        message.rotation[2] = rotation.z;
        message.rotation[3] = rotation.w;

        network.broadcastUnreliable(&message, sizeof(message));
    }
}

void ReplicationManager::applyTransformSnapshots(float deltaTime) {
    if (NetworkManager::getInstance().isHost()) {
        return;
    }

    constexpr float smoothingStrength = 12.0f;

    const float alpha =
        1.0f - std::exp(-smoothingStrength * deltaTime);

    const PlayerSlot localSlot =
        NetworkManager::getInstance().getLocalPlayerSlot();

    const std::string localAvatarName =
        "PlayerAvatar_" + std::to_string(localSlot);

    for (auto& pair : m_transformTargets) {
        const uint32_t networkId = pair.first;
        TransformTarget& target = pair.second;

        if (!target.active) {
            continue;
        }

        Node* node = findByNetworkId(networkId);

        if (node == nullptr) {
            continue;
        }

        // Do not let host snapshots fight the local player's own avatar.
        if (node->getName() == localAvatarName) {
            continue;
        }

        const glm::vec3 currentPosition = node->getPosition();
        const glm::quat currentRotation = node->getRotation();

        const float distance = glm::distance(currentPosition, target.position);

        if (distance > 4.0f) {
            node->setPosition(target.position);
        } else {
            node->setPosition(
                glm::mix(
                    currentPosition,
                    target.position,
                    alpha
                )
            );
        }

        node->setRotation(
            glm::slerp(
                currentRotation,
                target.rotation,
                alpha
            )
        );
    }
}

void ReplicationManager::resendAllSpawnNodes() {
    auto& network = NetworkManager::getInstance();

    if (!network.isHost()) {
        return;
    }

    for (const auto& pair : m_networkIdToNode) {
        const uint32_t networkId = pair.first;
        Node* node = pair.second;

        if (node == nullptr) {
            continue;
        }

        SpawnNodeMessage message{};

        message.header.type = MessageType::SpawnNode;
        message.header.sequence = 0;
        message.header.senderSlot = network.getLocalPlayerSlot();

        message.networkId = networkId;

        const std::string& name = node->getName();
        std::strncpy(message.name, name.c_str(), sizeof(message.name) - 1);
        message.name[sizeof(message.name) - 1] = '\0';

        message.glbPath[0] = '\0';

        const glm::vec3 position = node->getPosition();
        const glm::quat rotation = node->getRotation();
        const glm::vec3 scale = node->getScale();

        message.position[0] = position.x;
        message.position[1] = position.y;
        message.position[2] = position.z;

        message.rotation[0] = rotation.x;
        message.rotation[1] = rotation.y;
        message.rotation[2] = rotation.z;
        message.rotation[3] = rotation.w;

        message.scale[0] = scale.x;
        message.scale[1] = scale.y;
        message.scale[2] = scale.z;

        message.bodyType = 0;
        message.colliderType = 0;
        message.friction = 0.5f;
        message.restitution = 0.0f;

        network.broadcastReliable(&message, sizeof(message));
    }

    std::cout << "[Replication] Resent spawn messages for "
              << m_networkIdToNode.size()
              << " replicated node(s)" << std::endl;
}

void ReplicationManager::sendAvatarTransform(
    PlayerSlot slot,
    const glm::vec3& position
) {
    auto& network = NetworkManager::getInstance();

    if (!network.isConnected()) {
        return;
    }

    // Only clients send their avatar transform to the host.
    if (network.isHost()) {
        return;
    }

    AvatarTransformMessage message{};

    message.header.type = MessageType::AvatarTransform;
    message.header.sequence = 0;
    message.header.senderSlot = network.getLocalPlayerSlot();

    message.slot = slot;

    message.position[0] = position.x;
    message.position[1] = position.y;
    message.position[2] = position.z;

    network.sendUnreliable(HostSlot, &message, sizeof(message));
}

}