#pragma once

#include "NetworkDefs.h"

#include <cstdint>
#include <cstring>

namespace net {

#pragma pack(push, 1)

enum class MessageType : uint8_t {
    Invalid = 0,

    // Connection handshake
    Hello = 1,
    Welcome = 2,
    PlayerJoined = 3,
    PlayerLeft = 4,

    // Late join
    LateJoinRequest = 5,
    LateJoinResponse = 6,

    // Scene replication
    SpawnNode = 10,
    DespawnNode = 11,

    // World state
    TransformSnapshot = 20,
    PhysicsSnapshot = 21,
    RigSnapshot = 22,

    // Animation
    AnimationCommand = 30,

    // Lua gameplay events
    LuaEvent = 40,

    // Utility
    Ping = 50,
    Pong = 51
};

struct MessageHeader {
    MessageType type = MessageType::Invalid;
    uint32_t sequence = 0;
    PlayerSlot senderSlot = InvalidSlot;
};

// Sent from client to host when joining.
struct HelloMessage {
    MessageHeader header;

    char playerName[64] = {};
};

// Sent from host to a newly joined client.
struct WelcomeMessage {
    MessageHeader header;

    PlayerSlot assignedSlot = InvalidSlot;
    uint32_t currentSceneVersion = 0;
};

// Sent by host when another player joins.
struct PlayerJoinedMessage {
    MessageHeader header;

    PlayerSlot slot = InvalidSlot;
    char playerName[64] = {};
};

struct PlayerLeftMessage {
    MessageHeader header;

    PlayerSlot slot = InvalidSlot;
};

// Sent by a late-joining client to request full world state.
struct LateJoinRequestMessage {
    MessageHeader header;
};

// Sent by host to mark that a full late-join dump is finished.
struct LateJoinResponseMessage {
    MessageHeader header;

    uint32_t sceneVersion = 0;
};

// Host tells clients to create a replicated node.
struct SpawnNodeMessage {
    MessageHeader header;

    uint32_t networkId = 0;

    char name[64] = {};

    // Optional GLB path if this node was loaded from a mesh.
    char glbPath[256] = {};

    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};

    // Physics description.
    //
    // 0 = no body
    // 1 = static
    // 2 = kinematic
    // 3 = dynamic
    uint8_t bodyType = 0;

    // 0 = none
    // 1 = box
    // 2 = convex
    uint8_t colliderType = 0;

    float friction = 0.5f;
    float restitution = 0.0f;
};

// Host tells clients to remove a replicated node.
struct DespawnNodeMessage {
    MessageHeader header;

    uint32_t networkId = 0;
};

// A transform update for one replicated node.
struct TransformSnapshotMessage {
    MessageHeader header;

    uint32_t networkId = 0;

    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

// A physics snapshot for one replicated physics body.
//
// This can include velocity so clients can extrapolate between snapshots.
struct PhysicsSnapshotMessage {
    MessageHeader header;

    uint32_t networkId = 0;

    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    float linearVelocity[3] = {0.0f, 0.0f, 0.0f};
    float angularVelocity[3] = {0.0f, 0.0f, 0.0f};
};

// A VR rig snapshot for one player.
struct RigSnapshotMessage {
    MessageHeader header;

    PlayerSlot playerSlot = InvalidSlot;

    float rigPosition[3] = {0.0f, 0.0f, 0.0f};
    float rigYaw = 0.0f;
    float headPosition[3] = {0.0f, 0.0f, 0.0f};
    float headRotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float leftPosition[3] = {0.0f, 0.0f, 0.0f};
    float leftRotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float rightPosition[3] = {0.0f, 0.0f, 0.0f};
    float rightRotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

enum class AnimationNetCommand : uint8_t {
    None = 0,

    PlayProcedural = 1,
    StopProcedural = 2,
    PlayAnimation = 3,
    StopAnimation = 4
};

struct AnimationCommandMessage {
    MessageHeader header;

    uint32_t networkId = 0;

    AnimationNetCommand command = AnimationNetCommand::None;

    char name[64] = {};
    uint8_t axis = 0;
    uint8_t direction = 0;
    uint8_t animationType = 0;

    float speed = 0.0f;
    uint32_t handle = 0;
};

struct LuaEventMessage {
    MessageHeader header;

    char eventName[64] = {};
    uint32_t payloadSize = 0;
    char payload[1024] = {};
};

struct PingMessage {
    MessageHeader header;

    float clientTime = 0.0f;
};

struct PongMessage {
    MessageHeader header;

    float clientTime = 0.0f;
    float serverTime = 0.0f;
};

#pragma pack(pop)

}