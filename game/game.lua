local game = {}

local elapsedTime = 0.0

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
end

function game.onUpdate(deltaTime, scene, animator)
    elapsedTime = elapsedTime + deltaTime
    local frog = scene:findNode("Frog")
    if frog ~= nil then
        if frog:getPositionY() < -100.0 then
            frog:setPosition(0.0, 3.0, 0.0)
        end
    end

    -- Tower damage demo
    local tower = scene:findNode("Tower")

    if isDestructible(tower) and not tower:isDestroyed() then
        -- Damage the tower slowly after 5 seconds.
        if elapsedTime > 5.0 then
            tryApplyDamage(tower, 10.0 * deltaTime)
        end

        -- Fully destroy the tower after 15 seconds.
        if elapsedTime > 15.0 then
            tryApplyDamage(tower, 9999.0)
            -- WARNING: LOUD
            -- Audio.playSound("resources/sounds/test.wav", 0.1)
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
end

return game