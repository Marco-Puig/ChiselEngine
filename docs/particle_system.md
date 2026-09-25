# Particle System API Documentation

The Particle System still needs a lot more work into it. However, you can create fire and smoke particles for now. 

## Create particles
```lua
local fire = scene:createFire("Fire", x, y, z)
local smoke = scene:createSmoke("Smoke", x, y, z)
```

## Control particles
```lua
fire:playParticles()
fire:stopParticles()
fire:clearParticles()
fire:setParticleEmissionRate(200.0)

if fire:isParticleSystemPlaying() then
    print("Fire is playing")
end
```

# Find and control later
```lua
local fire = scene:findNode("Fire")

if fire ~= nil then
    fire:stopParticles()
end
```