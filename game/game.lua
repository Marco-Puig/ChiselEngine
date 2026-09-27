local game = {}

local elapsedTime = 0.0

local playerAvatars = {}
local localPlayerSlot = 0

local function isDestructible(node)
    return node ~= nil
        and node.applyDamage ~= nil
        and node.isDestroyed ~= nil
end

local function tryApplyDamage(node, amount)
    if isDestructible(node) then
        node:applyDamage(amount)
    end
end

local function getPlayerColor(slot)
    if slot == 0 then
        -- Host / player 0: blue
        return 0.25, 0.75, 1.0
    elseif slot == 1 then
        -- Player 1: orange
        return 1.0, 0.55, 0.2
    else
        -- Player 2: green
        return 0.55, 1.0, 0.35
    end
end

local function spawnPlayerAvatar(scene, slot, x, y, z)
    if scene.createPlayerAvatar == nil then
        return nil
    end

    local name = "PlayerAvatar_" .. tostring(slot)

    local existing = scene:findNode(name)
    if existing ~= nil then
        playerAvatars[slot] = existing
        return existing
    end

    local r, g, b = getPlayerColor(slot)

    local avatar = scene:createPlayerAvatar(
        name,
        x,
        y,
        z,
        r,
        g,
        b
    )

    if avatar ~= nil then
        playerAvatars[slot] = avatar

        -- If your multiplayer replication module exists and we are the host,
        -- replicate this avatar to other players.
        if Net ~= nil and Net.replicateNode ~= nil and Net.isHost ~= nil then
            if Net.isHost() then
                Net.replicateNode(avatar)
            end
        end
    end

    return avatar
end

local function updateLocalAvatar()
    local avatar = playerAvatars[localPlayerSlot]

    if avatar == nil then
        return
    end

    -- Keep the local player's avatar aligned with the VR rig.
    avatar:setPosition(
        Engine.getVRPlayerPositionX(),
        Engine.getVRPlayerPositionY(),
        Engine.getVRPlayerPositionZ()
    )
end

function game.onStart(scene, animator)
    local light = scene:createDirectionalLight("Sun")
    light:setIntensity(0.4)
    light:setColor(1.0, 0.9, 0.8)
    light:setPosition(0.0, 4.0, 0.0)

    Engine.setSkybox("resources/graphics/skybox.jpg")

    local floor = scene:loadMesh("resources/plane.glb", "Floor")
    floor:setPosition(0.0, 0.0, 0.0)
    Physics.addRigidBody(floor, "static", "convex", 0.8, 0.0)
    animator:procedural(floor, "y", "positive", "rotation", 0.1)

    local frog = scene:loadMesh("resources/models/frog.glb", "Frog")
    if frog ~= nil then
        frog:setPosition(0.0, 3.0, 0.0)
        Physics.addRigidBody(frog, "dynamic", "convex", 0.6, 0.1)
    end

    if scene.createDestructibleBuilding ~= nil then
        local tower = scene:createDestructibleBuilding(
            "Tower",
            0.0,
            0.0,
            -4.0,
            3.0,
            100.0
        )

        if tower ~= nil then
            Physics.addRigidBody(tower, "static", "box", 0.8, 0.0)
        end
    end

    if scene.createFire ~= nil then
        local fire = scene:createFire("Fire", 0.0, 0.25, -2.0)

        if fire ~= nil then
            fire:setParticleEmissionRate(140.0)
        end
    end

    if scene.createSmoke ~= nil then
        local smoke = scene:createSmoke("Smoke", 0.0, 1.25, -2.0)

        if smoke ~= nil then
            smoke:setParticleEmissionRate(30.0)
        end
    end

    -- If your multiplayer module exists, ask it for our player slot.
    if Net ~= nil and Net.getLocalPlayerSlot ~= nil then
        localPlayerSlot = Net.getLocalPlayerSlot()
    end

    local networkActive = false

    if Net ~= nil and Net.isConnected ~= nil then
        networkActive = Net.isConnected()
    end

    if not networkActive then
        -- Offline/single-player preview:
        -- spawn two fake remote players so you can see the visuals.
        spawnPlayerAvatar(scene, 1, 1.5, 0.0, -1.5)
        spawnPlayerAvatar(scene, 2, -1.5, 0.0, -1.5)
    else
        -- Networked session:
        -- spawn avatars for all possible player slots.
        --
        -- Later, when you have proper join/leave messages, you should
        -- spawn remote avatars when players join instead of spawning
        -- all slots immediately.
        for slot = 0, 2 do
            local x = 0.0
            local z = 1.0

            if slot == 1 then
                x = 1.5
                z = -1.5
            elseif slot == 2 then
                x = -1.5
                z = -1.5
            end

            spawnPlayerAvatar(scene, slot, x, 0.0, z)
        end
    end
end

function game.onUpdate(deltaTime, scene, animator)
    elapsedTime = elapsedTime + deltaTime

    ------------------------------------------------------------------
    -- Frog respawn logic
    ------------------------------------------------------------------
    local frog = scene:findNode("Frog")

    if frog ~= nil then
        if frog:getPositionY() < -100.0 then
            frog:setPosition(0.0, 3.0, 0.0)
        end
    end

    ------------------------------------------------------------------
    -- Tower damage demo
    ------------------------------------------------------------------
    local tower = scene:findNode("Tower")

    if isDestructible(tower) and not tower:isDestroyed() then
        -- Damage the tower slowly after 5 seconds.
        if elapsedTime > 5.0 then
            tryApplyDamage(tower, 10.0 * deltaTime)
        end

        -- Fully destroy the tower after 15 seconds.
        if elapsedTime > 15.0 then
            tryApplyDamage(tower, 9999.0)
        end

        -- Damage the tower when the frog is close to it.
        if frog ~= nil then
            local dx = frog:getPositionX() - tower:getPositionX()
            local dz = frog:getPositionZ() - tower:getPositionZ()

            local distanceSquared = dx * dx + dz * dz

            if distanceSquared < 2.5 then
                tryApplyDamage(tower, 20.0 * deltaTime)
            end
        end
    end

    -- Particle System Demo
    if elapsedTime > 30.0 then
        local fire = scene:findNode("Fire")
        if fire ~= nil and fire.stopParticles ~= nil then
            fire:stopParticles()
        end

        local smoke = scene:findNode("Smoke")
        if smoke ~= nil and smoke.stopParticles ~= nil then
            smoke:stopParticles()
        end
    end


    -- Multiplayer
    updateLocalAvatar()
    local networkActive = false

    if Net ~= nil and Net.isConnected ~= nil then
        networkActive = Net.isConnected()
    end

    if not networkActive then
        local avatar1 = playerAvatars[1]
        local avatar2 = playerAvatars[2]

        if avatar1 ~= nil then
            avatar1:setPosition(
                math.sin(elapsedTime * 0.8) * 2.0,
                0.0,
                -2.0 + math.cos(elapsedTime * 0.8) * 2.0
            )
        end

        if avatar2 ~= nil then
            avatar2:setPosition(
                math.sin(elapsedTime * 0.8 + math.pi) * 2.0,
                0.0,
                -2.0 + math.cos(elapsedTime * 0.8 + math.pi) * 2.0
            )
        end
    end
end

return game