#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Scene/Node.h"
#include "Scene/AnimationLibrary.h"
#include <vector>
#include <string>
#include <map>

class GLBLoader {
public:
    static MeshNode* loadGLB(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_CalcTangentSpace | 
            aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            return nullptr;
        }

        // 1. Load the root node
        MeshNode* root = new MeshNode(scene->mRootNode->mName.C_Str());
        
        // 2. Recursively process the node hierarchy (Simplified)
        processNode(scene->mRootNode, scene, root);

        // 3. Load Animations into the Library
        for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
            aiAnimation* aiAnim = scene->mAnimations[i];
            AnimationClip clip;
            clip.name = aiAnim->mName.C_Str();
            clip.duration = (float)aiAnim->mDuration;

            for (unsigned int j = 0; j < aiAnim->mNumChannels; j++) {
                aiNodeAnim* channel = aiAnim->mChannels[j];
                AnimationTrack track;
                track.nodeName = channel->mNodeName.C_Str();

                for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) {
                    Keyframe kf;
                    kf.time = (float)channel->mPositionKeys[k].mTime;
                    kf.position = glm::vec3(channel->mPositionKeys[k].mValue.x, channel->mPositionKeys[k].mValue.y, channel->mPositionKeys[k].mValue.z);
                    kf.rotation = glm::quat(channel->mRotationKeys[k].mValue.w, channel->mRotationKeys[k].mValue.x, channel->mRotationKeys[k].mValue.y, channel->mRotationKeys[k].mValue.z);
                    kf.scale = glm::vec3(channel->mScalingKeys[k].mValue.x, channel->mScalingKeys[k].mValue.y, channel->mScalingKeys[k].mValue.z);
                    track.keyframes.push_back(kf);
                }
                clip.tracks.push_back(track);
            }
            AnimationLibrary::getInstance().addClip(clip);
        }

        return root;
    }

private:
    static void processNode(aiNode* node, const aiScene* scene, MeshNode* parent) {
        // Recursively load children and attach them as Nodes...
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, parent);
        }
    }
};
