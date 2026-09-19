#include "RenderSystem.h"
#include "scene/MeshNode.h"
#include "Platform/PhysicsSystem.h"
#include "scene/ArcRotateCamera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <limits>

DirectionalLight* RenderSystem::getDirectionalLight() const {
    for (Light* light : m_lights)
        if (auto* directional = dynamic_cast<DirectionalLight*>(light))
            return directional;
    return nullptr;
}

void RenderSystem::init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.08f, 0.1f, 0.14f, 1.0f);
    m_shader = std::make_unique<Shader>(
        "#version 450 core\nlayout(location=0) in vec3 aPosition;\nlayout(location=1) in vec3 aNormal;\nlayout(location=2) in vec2 aTexCoord;\nuniform mat4 uModel; uniform mat4 uView; uniform mat4 uProjection;\nout vec3 vNormal; out vec3 vWorldPosition; out vec2 vTexCoord;\nvoid main(){vec4 world=uModel*vec4(aPosition,1.0); vWorldPosition=world.xyz; vNormal=mat3(transpose(inverse(uModel)))*aNormal; vTexCoord=aTexCoord; gl_Position=uProjection*uView*world;}",
        "#version 450 core\nin vec3 vNormal; in vec3 vWorldPosition; in vec2 vTexCoord;\nuniform vec3 uLightDirection; uniform vec3 uLightColor; uniform vec3 uCameraPosition; uniform float uLightIntensity; uniform float uLightExposure;\nuniform vec4 uBaseColorFactor; uniform float uMetallicFactor; uniform float uRoughnessFactor;\nuniform bool uHasBaseColorTexture; uniform sampler2D uBaseColorTexture; uniform sampler2D uMetallicRoughnessTexture; uniform sampler2D uNormalTexture; uniform sampler2D uEmissiveTexture;\nuniform bool uHasMetallicRoughnessTexture; uniform bool uHasNormalTexture; uniform bool uHasEmissiveTexture; out vec4 FragColor;\nvoid main(){vec4 base=uBaseColorFactor; if(uHasBaseColorTexture) base*=texture(uBaseColorTexture,vTexCoord); vec3 n=normalize(vNormal); if(uHasNormalTexture){vec3 dp1=dFdx(vWorldPosition); vec3 dp2=dFdy(vWorldPosition); vec2 duv1=dFdx(vTexCoord); vec2 duv2=dFdy(vTexCoord); vec3 t=normalize(dp1*duv2.y-dp2*duv1.y); vec3 b=normalize(cross(n,t)); n=normalize(mat3(t,b,n)*(texture(uNormalTexture,vTexCoord).xyz*2.0-1.0));} vec3 l=normalize(-uLightDirection); vec3 v=normalize(uCameraPosition-vWorldPosition); vec3 h=normalize(l+v); float metallic=uMetallicFactor; float rough=uRoughnessFactor; if(uHasMetallicRoughnessTexture){vec4 mr=texture(uMetallicRoughnessTexture,vTexCoord); rough*=mr.g; metallic*=mr.b;} float diffuse=max(dot(n,l),0.0); rough=max(rough,0.04); float specular=pow(max(dot(n,h),0.0),mix(128.0,4.0,rough))*mix(0.04,0.96,metallic); vec3 lighting=(base.rgb*(0.08+diffuse*uLightColor)+specular*uLightColor)*uLightIntensity; vec3 color=vec3(1.0)-exp(-lighting*exp2(uLightExposure)); if(uHasEmissiveTexture) color+=texture(uEmissiveTexture,vTexCoord).rgb; FragColor=vec4(color,base.a);}");
    m_debugShader = std::make_unique<Shader>(
        "#version 450 core\nlayout(location=0) in vec3 aPosition;\nuniform mat4 uView;\nuniform mat4 uProjection;\nvoid main(){gl_Position=uProjection*uView*vec4(aPosition,1.0);}",
        "#version 450 core\nout vec4 FragColor;\nvoid main(){FragColor=vec4(1.0,0.7,0.1,1.0);}");
    glGenVertexArrays(1, &m_debugVao);
    glGenBuffers(1, &m_debugVbo);
    m_skyboxShader = std::make_unique<Shader>(
        "#version 450 core\nlayout(location=0) in vec3 aPosition; out vec3 vDirection; uniform mat4 uView; uniform mat4 uProjection; void main(){vDirection=aPosition; vec4 p=uProjection*mat4(mat3(uView))*vec4(aPosition,1.0); gl_Position=p.xyww;}",
        "#version 450 core\nin vec3 vDirection; uniform sampler2D uEnvironment; out vec4 FragColor; const float PI=3.14159265359; void main(){vec3 d=normalize(vDirection); float u=atan(d.z,d.x)/(2.0*PI)+0.5; float v=1.0-(asin(clamp(d.y,-1.0,1.0))/PI+0.5); FragColor=vec4(texture(uEnvironment,vec2(u,v)).rgb,1.0);}");
    const float cube[] = {
        -1,-1,-1, 1,-1,-1, 1,1,-1, 1,1,-1, -1,1,-1, -1,-1,-1,
        -1,-1,1, 1,-1,1, 1,1,1, 1,1,1, -1,1,1, -1,-1,1,
        -1,1,1, 1,1,1, 1,1,-1, 1,1,-1, -1,1,-1, -1,1,1,
        -1,-1,1, 1,-1,1, 1,-1,-1, 1,-1,-1, -1,-1,-1, -1,-1,1,
        1,-1,1, 1,1,1, 1,1,-1, 1,1,-1, 1,-1,-1, 1,-1,1,
        -1,-1,-1, -1,1,-1, -1,1,1, -1,1,1, -1,-1,1, -1,-1,-1};
    glGenVertexArrays(1, &m_skyboxVao);
    glGenBuffers(1, &m_skyboxVbo);
    glBindVertexArray(m_skyboxVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void RenderSystem::render(Node* rootNode) {
    const glm::mat4 view = m_desktopCamera != nullptr
        ? m_desktopCamera->getViewMatrix()
        : glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f),
                      glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 proj = m_desktopCamera != nullptr
        ? m_desktopCamera->getProjectionMatrix()
        : glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    renderView(rootNode, view, proj, 0, 1280, 720);
}

void RenderSystem::renderView(Node* rootNode, const glm::mat4& view,
                              const glm::mat4& proj, unsigned int framebuffer,
                              int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderSkybox(view, proj);
    m_shader->use();
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", proj);
    glm::vec3 lightDirection(0.0f, -1.0f, 0.0f);
    glm::vec3 lightColor(1.0f);
    float lightIntensity = 1.0f;
    float lightExposure = 0.0f;
    if (!m_lights.empty()) {
        if (auto* light = dynamic_cast<DirectionalLight*>(m_lights.front())) {
            lightDirection = light->getDirection();
            lightColor = light->getColor();
            lightIntensity = light->getIntensity();
            lightExposure = light->getExposure();
        }
    }
    m_shader->setVec3("uLightDirection", lightDirection);
    m_shader->setVec3("uLightColor", lightColor);
    m_shader->setFloat("uLightIntensity", lightIntensity);
    m_shader->setFloat("uLightExposure", lightExposure);
    m_shader->setVec3("uCameraPosition", glm::vec3(glm::inverse(view)[3]));
    traverseAndRender(rootNode);
    renderCollisionDebug(view, proj);
}

void RenderSystem::renderSkybox(const glm::mat4& view, const glm::mat4& projection) {
    if (m_skyboxTexture == 0 && !m_skyboxPath.empty()) {
        int width = 0, height = 0, channels = 0;
        unsigned char* pixels = stbi_load(m_skyboxPath.c_str(), &width, &height,
                                          &channels, 3);
        if (pixels == nullptr)
            throw std::runtime_error("Failed to load skybox image: " + m_skyboxPath);
        glGenTextures(1, &m_skyboxTexture);
        glBindTexture(GL_TEXTURE_2D, m_skyboxTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        stbi_image_free(pixels);
    }
    if (m_skyboxTexture == 0)
        return;
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    m_skyboxShader->use();
    m_skyboxShader->setMat4("uView", view);
    m_skyboxShader->setMat4("uProjection", projection);
    m_skyboxShader->setInt("uEnvironment", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_skyboxTexture);
    const GLboolean cullingEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(m_skyboxVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    if (cullingEnabled)
        glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void RenderSystem::renderCollisionDebug(const glm::mat4& view, const glm::mat4& projection) {
#if defined(_CPPUNWIND)
    try {
#endif
    const std::vector<PhysicsDebugLine> lines = PhysicsSystem::getInstance().getDebugLines();
    if (lines.empty())
        return;
    if (m_debugShader == nullptr || m_debugVao == 0 || m_debugVbo == 0) {
        std::cerr << "[Render] Collision debug resources are unavailable\n";
        return;
    }
    if (lines.size() > static_cast<size_t>(std::numeric_limits<GLsizei>::max() / 2)) {
        std::cerr << "[Render] Collision debug geometry is too large; skipping\n";
        return;
    }
    std::vector<glm::vec3> vertices;
#if defined(_CPPUNWIND)
    try {
#endif
        vertices.reserve(lines.size() * 2);
        for (const PhysicsDebugLine& line : lines) {
            vertices.push_back(line.from);
            vertices.push_back(line.to);
        }
#if defined(_CPPUNWIND)
    } catch (const std::exception& error) {
        std::cerr << "[Render] Collision debug vertex allocation failed: "
                  << error.what() << '\n';
        return;
    }
#endif
    m_debugShader->use();
    m_debugShader->setMat4("uView", view);
    m_debugShader->setMat4("uProjection", projection);
    glBindVertexArray(m_debugVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_debugVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
                 vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
#if defined(_CPPUNWIND)
    } catch (const std::exception& error) {
        std::cerr << "[Render] Collision debug skipped after exception: "
                  << error.what() << '\n';
    } catch (...) {
        std::cerr << "[Render] Collision debug skipped after unknown exception\n";
    }
#endif
}

void RenderSystem::traverseAndRender(Node* node) {
    if (!node) return;

    glm::mat4 worldTransform = node->getWorldTransform();
    
    if (MeshNode* meshNode = dynamic_cast<MeshNode*>(node)) {
        m_shader->setMat4("uModel", worldTransform);
        const Material& material = meshNode->getMaterial();
        m_shader->setVec4("uBaseColorFactor", material.baseColorFactor);
        m_shader->setFloat("uMetallicFactor", material.metallicFactor);
        m_shader->setFloat("uRoughnessFactor", material.roughnessFactor);
        m_shader->setInt("uHasBaseColorTexture", material.hasBaseColorTexture() ? 1 : 0);
        m_shader->setInt("uHasMetallicRoughnessTexture", material.hasMetallicRoughnessTexture() ? 1 : 0);
        m_shader->setInt("uHasNormalTexture", material.hasNormalTexture() ? 1 : 0);
        m_shader->setInt("uHasEmissiveTexture", material.hasEmissiveTexture() ? 1 : 0);
        const GLuint textures[] = {material.baseColorTexture, material.metallicRoughnessTexture,
                                   material.normalTexture, material.emissiveTexture};
        for (int unit = 0; unit < 4; ++unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, textures[unit]);
        }
        m_shader->setInt("uBaseColorTexture", 0);
        m_shader->setInt("uMetallicRoughnessTexture", 1);
        m_shader->setInt("uNormalTexture", 2);
        m_shader->setInt("uEmissiveTexture", 3);
        glBindVertexArray(meshNode->getVAO());
        glDrawElements(GL_TRIANGLES, meshNode->getIndexCount(), GL_UNSIGNED_INT, nullptr);
    }

    for (const auto& child : node->getChildren())
        traverseAndRender(child.get());
}
