local game = {}

function game.onStart(scene, animator)
    local light = scene:createDirectionalLight("Sun")
    light:setColor(1.0, 0.9, 0.8)
    light:setPosition(0.0, 4.0, 0.0)

    Engine.setSkybox("resources/skybox.jpg")

    local floor = scene:loadMesh("resources/plane.glb", "Floor")
    floor:setPosition(0.0, 0.0, 0.0)
    Engine.addRigidBody(floor, "static", "box", 0.8, 0.0)

    local frog = scene:loadMesh("resources/frog.glb", "Frog")
    if frog ~= nil then
        frog:setPosition(0.0, 3.0, 0.0)
        Engine.addRigidBody(frog, "dynamic", "convex", 0.6, 0.1)
        animator:procedural(frog, "y", "positive", "rotation", 1.0)
    end
end

function game.onUpdate(deltaTime, scene, animator)
    local frog = scene:findNode("Frog")
    if frog ~= nil and frog:getPositionY() < -100.0 then
        frog:setPosition(0.0, 3.0, 0.0)
    end
end

return game
