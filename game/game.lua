local game = {}

local frogRotationId = -1
local timeElapsed = 0.0

function game.onStart(scene, animator)
    local light = scene:createDirectionalLight("Sun")
    light:setIntensity(0.4)
    light:setColor(1.0, 0.9, 0.8)
    light:setPosition(0.0, 4.0, 0.0)

    Engine.setSkybox("resources/skybox.jpg")

    local floor = scene:loadMesh("resources/plane.glb", "Floor")
    floor:setPosition(0.0, 0.0, 0.0)
    Engine.addRigidBody(floor, "static", "convex", 0.8, 0.0)

    local frog = scene:loadMesh("resources/frog.glb", "Frog")
    if frog ~= nil then
        frog:setPosition(0.0, 3.0, 0.0)
        Engine.addRigidBody(frog, "dynamic", "convex", 0.6, 0.1)
        frogRotationId = animator:procedural(frog, "y", "positive", "rotation", 1.0)
    end
end

function game.onUpdate(deltaTime, scene, animator)
    timeElapsed = timeElapsed + deltaTime
    local frog = scene:findNode("Frog")
    
    if frog ~= nil then
        if frog:getPositionY() < -100.0 then
            frog:setPosition(0.0, 3.0, 0.0)
        end

        if timeElapsed > 5.0 and timeElapsed < 5.1 then
            animator:stopProcedural(frogRotationId)
        end
    end
end

return game