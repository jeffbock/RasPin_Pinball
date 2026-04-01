// pb3dutil — 3D model analysis and utility tool for RasPin Pinball
//
// Usage:
//   pb3dutil --info        <file.glb>  [options]
//   pb3dutil --list-clips  <file.glb>
//   pb3dutil --simplify-bones <file.glb> [--max-bones N] [--threshold F]
//   pb3dutil --help
//
// Commands:
//   --info           Print full model info: meshes, materials, textures, bones, clips
//   --list-clips     List available animation clips (name, duration, channel count)
//   --simplify-bones Analyse which bones could be removed based on weight threshold
//                    (informational; does not modify the file)
//
// Options:
//   --max-bones N    Maximum bones to keep (default: 160, matching PB3D_MAX_BONES)
//   --threshold F    Minimum total weight for a bone to be retained (default: 0.01)
//   --help           Print this help message

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons
// Attribution-NonCommercial 4.0 International License.

#include "../3rdparty/cgltf.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <cmath>
#include <algorithm>

// ============================================================================
// Helpers
// ============================================================================

static void printSeparator() {
    std::cout << "------------------------------------------------------------\n";
}

static std::string interpName(cgltf_interpolation_type t) {
    switch (t) {
        case cgltf_interpolation_type_linear:       return "LINEAR";
        case cgltf_interpolation_type_step:         return "STEP";
        case cgltf_interpolation_type_cubic_spline: return "CUBICSPLINE";
        default:                                     return "UNKNOWN";
    }
}

static std::string pathName(cgltf_animation_path_type t) {
    switch (t) {
        case cgltf_animation_path_type_translation: return "translation";
        case cgltf_animation_path_type_rotation:    return "rotation";
        case cgltf_animation_path_type_scale:       return "scale";
        case cgltf_animation_path_type_weights:     return "morph-weights";
        default:                                     return "unknown";
    }
}

static cgltf_data* loadGLB(const char* path) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success) {
        std::cerr << "Error: Failed to parse file '" << path
                  << "' (cgltf error " << (int)result << ")\n";
        return nullptr;
    }

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) {
        std::cerr << "Error: Failed to load buffers in '" << path
                  << "' (cgltf error " << (int)result << ")\n";
        cgltf_free(data);
        return nullptr;
    }

    return data;
}

// ============================================================================
// --info command
// ============================================================================

static int cmdInfo(const char* path) {
    cgltf_data* data = loadGLB(path);
    if (!data) return 1;

    printSeparator();
    std::cout << "FILE: " << path << "\n";
    printSeparator();

    // --- Scenes & nodes ---
    std::cout << "Scenes   : " << data->scenes_count << "\n";
    std::cout << "Nodes    : " << data->nodes_count   << "\n";

    // --- Meshes & primitives ---
    size_t totalPrims = 0;
    bool hasSkinData  = false;
    for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
        totalPrims += data->meshes[mi].primitives_count;
        for (cgltf_size pi = 0; pi < data->meshes[mi].primitives_count; pi++) {
            const cgltf_primitive* prim = &data->meshes[mi].primitives[pi];
            for (cgltf_size ai = 0; ai < prim->attributes_count; ai++) {
                if (prim->attributes[ai].type == cgltf_attribute_type_joints ||
                    prim->attributes[ai].type == cgltf_attribute_type_weights) {
                    hasSkinData = true;
                }
            }
        }
    }
    std::cout << "Meshes   : " << data->meshes_count   << "  (" << totalPrims << " primitives)\n";
    std::cout << "Materials: " << data->materials_count << "\n";
    std::cout << "Textures : " << data->textures_count  << "\n";
    std::cout << "Images   : " << data->images_count    << "\n";
    std::cout << "Has skin data: " << (hasSkinData ? "YES" : "NO") << "\n";

    // --- Mesh detail ---
    if (data->meshes_count > 0) {
        std::cout << "\nMesh detail:\n";
        for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
            const cgltf_mesh* mesh = &data->meshes[mi];
            std::cout << "  Mesh[" << mi << "]: \"" << (mesh->name ? mesh->name : "(unnamed)") << "\"\n";
            for (cgltf_size pi = 0; pi < mesh->primitives_count; pi++) {
                const cgltf_primitive* prim = &mesh->primitives[pi];
                size_t vertCount = 0;
                size_t indexCount = prim->indices ? prim->indices->count : 0;
                std::vector<std::string> attrs;
                for (cgltf_size ai = 0; ai < prim->attributes_count; ai++) {
                    const auto& a = prim->attributes[ai];
                    if (a.type == cgltf_attribute_type_position) vertCount = a.data->count;
                    std::string aname;
                    switch (a.type) {
                        case cgltf_attribute_type_position:  aname = "POSITION";   break;
                        case cgltf_attribute_type_normal:    aname = "NORMAL";     break;
                        case cgltf_attribute_type_tangent:   aname = "TANGENT";    break;
                        case cgltf_attribute_type_texcoord:  aname = "TEXCOORD_0"; break;
                        case cgltf_attribute_type_color:     aname = "COLOR_0";    break;
                        case cgltf_attribute_type_joints:    aname = "JOINTS_0";   break;
                        case cgltf_attribute_type_weights:   aname = "WEIGHTS_0";  break;
                        default:                              aname = "OTHER";      break;
                    }
                    attrs.push_back(aname);
                }
                std::cout << "    Prim[" << pi << "]: " << vertCount << " verts, "
                          << indexCount << " indices, attrs:";
                for (const auto& a : attrs) std::cout << " " << a;
                if (prim->material) {
                    std::cout << ", material: \""
                              << (prim->material->name ? prim->material->name : "(unnamed)")
                              << "\"";
                }
                std::cout << "\n";
            }
        }
    }

    // --- Skins / Skeleton ---
    if (data->skins_count > 0) {
        std::cout << "\nSkins: " << data->skins_count << "\n";
        for (cgltf_size si = 0; si < data->skins_count; si++) {
            const cgltf_skin* skin = &data->skins[si];
            std::cout << "  Skin[" << si << "]: \"" << (skin->name ? skin->name : "(unnamed)")
                      << "\"  " << skin->joints_count << " joints\n";
            if (skin->joints_count <= 32) {
                // Print bone hierarchy for small skeletons
                std::map<const cgltf_node*, int> nodeIdx;
                for (cgltf_size ji = 0; ji < skin->joints_count; ji++)
                    nodeIdx[skin->joints[ji]] = (int)ji;
                for (cgltf_size ji = 0; ji < skin->joints_count; ji++) {
                    const cgltf_node* jn = skin->joints[ji];
                    int parentIdx = -1;
                    if (jn->parent) {
                        auto it = nodeIdx.find(jn->parent);
                        if (it != nodeIdx.end()) parentIdx = it->second;
                    }
                    std::cout << "    Joint[" << ji << "]: \""
                              << (jn->name ? jn->name : "(unnamed)")
                              << "\"  parent: " << parentIdx << "\n";
                }
            } else {
                std::cout << "    (bone list truncated - " << skin->joints_count << " joints)\n";
            }
        }
    } else {
        std::cout << "\nSkins    : none\n";
    }

    // --- Animations ---
    if (data->animations_count > 0) {
        std::cout << "\nAnimations: " << data->animations_count << "\n";
        for (cgltf_size ai = 0; ai < data->animations_count; ai++) {
            const cgltf_animation* anim = &data->animations[ai];
            // Compute duration
            float duration = 0.0f;
            for (cgltf_size ci = 0; ci < anim->samplers_count; ci++) {
                if (anim->samplers[ci].input) {
                    float t = 0.0f;
                    cgltf_size last = anim->samplers[ci].input->count;
                    if (last > 0) {
                        cgltf_accessor_read_float(anim->samplers[ci].input, last - 1, &t, 1);
                        if (t > duration) duration = t;
                    }
                }
            }
            std::cout << "  Clip[" << ai << "]: \"" << (anim->name ? anim->name : "(unnamed)")
                      << "\"  duration: " << std::fixed << std::setprecision(3)
                      << duration << "s  channels: " << anim->channels_count << "\n";
        }
    } else {
        std::cout << "\nAnimations: none\n";
    }

    cgltf_free(data);
    printSeparator();
    return 0;
}

// ============================================================================
// --list-clips command
// ============================================================================

static int cmdListClips(const char* path) {
    cgltf_data* data = loadGLB(path);
    if (!data) return 1;

    if (data->animations_count == 0) {
        std::cout << "No animation clips found in: " << path << "\n";
        cgltf_free(data);
        return 0;
    }

    std::cout << "Animation clips in: " << path << "\n";
    printSeparator();
    std::cout << std::left
              << std::setw(4)  << "Idx"
              << std::setw(32) << "Name"
              << std::setw(12) << "Duration"
              << std::setw(10) << "Channels"
              << "Interpolations\n";
    printSeparator();

    for (cgltf_size ai = 0; ai < data->animations_count; ai++) {
        const cgltf_animation* anim = &data->animations[ai];

        float duration = 0.0f;
        std::set<std::string> interpTypes;
        for (cgltf_size si = 0; si < anim->samplers_count; si++) {
            const cgltf_animation_sampler* smp = &anim->samplers[si];
            interpTypes.insert(interpName(smp->interpolation));
            if (smp->input) {
                cgltf_size last = smp->input->count;
                if (last > 0) {
                    float t = 0.0f;
                    cgltf_accessor_read_float(smp->input, last - 1, &t, 1);
                    if (t > duration) duration = t;
                }
            }
        }

        std::string interpStr;
        for (const auto& s : interpTypes) {
            if (!interpStr.empty()) interpStr += "/";
            interpStr += s;
        }

        std::string clipName = anim->name ? anim->name : "(unnamed)";
        if (clipName.size() > 30) clipName = clipName.substr(0, 27) + "...";

        std::cout << std::left
                  << std::setw(4)  << ai
                  << std::setw(32) << clipName
                  << std::setw(12) << (std::to_string((int)(duration * 1000)) + "ms")
                  << std::setw(10) << anim->channels_count
                  << interpStr << "\n";

        // Per-channel detail
        for (cgltf_size ci = 0; ci < anim->channels_count; ci++) {
            const cgltf_animation_channel* ch = &anim->channels[ci];
            if (!ch->target_node) continue;
            std::cout << "       "
                      << pathName(ch->target_path) << " -> \""
                      << (ch->target_node->name ? ch->target_node->name : "(unnamed)")
                      << "\"  " << interpName(ch->sampler->interpolation) << "  "
                      << (ch->sampler->input ? ch->sampler->input->count : 0) << " keyframes\n";
        }
    }

    cgltf_free(data);
    printSeparator();
    return 0;
}

// ============================================================================
// --simplify-bones command
// ============================================================================

static int cmdSimplifyBones(const char* path, int maxBones, float threshold) {
    cgltf_data* data = loadGLB(path);
    if (!data) return 1;

    if (data->skins_count == 0) {
        std::cout << "No skeleton found in: " << path << "\n";
        cgltf_free(data);
        return 0;
    }

    std::cout << "Bone simplification analysis for: " << path << "\n";
    std::cout << "  max-bones  = " << maxBones  << "\n";
    std::cout << "  threshold  = " << threshold << " (minimum total weight per bone)\n";
    std::cout << "  skins      = " << data->skins_count << "\n";
    printSeparator();

    // Build unified node->globalBoneIndex map across all skins (same dedup logic as PB3D.cpp).
    // Also build per-skin local->global index remapping so per-mesh weight accumulation
    // translates skin-local joint indices to the unified global index correctly.
    std::map<const cgltf_node*, int>              nodeToGlobal;
    std::map<const cgltf_skin*, std::vector<int>> skinLocalToGlobal;
    std::vector<const cgltf_node*>                globalIndexToNode;  // reverse map for names

    for (cgltf_size si = 0; si < data->skins_count; si++) {
        const cgltf_skin* skin = &data->skins[si];
        std::vector<int>& l2g = skinLocalToGlobal[skin];
        l2g.resize(skin->joints_count, 0);
        for (cgltf_size ji = 0; ji < skin->joints_count; ji++) {
            const cgltf_node* jn = skin->joints[ji];
            auto it = nodeToGlobal.find(jn);
            if (it != nodeToGlobal.end()) {
                l2g[ji] = it->second;
            } else {
                int idx = (int)nodeToGlobal.size();
                nodeToGlobal[jn] = idx;
                globalIndexToNode.push_back(jn);
                l2g[ji] = idx;
            }
        }
    }

    int numBones = (int)globalIndexToNode.size();
    std::cout << "Total unique bones (across " << data->skins_count << " skin(s)): " << numBones << "\n";
    for (cgltf_size si = 0; si < data->skins_count; si++) {
        const cgltf_skin* skin = &data->skins[si];
        std::cout << "  Skin[" << si << "]: \"" << (skin->name ? skin->name : "(unnamed)")
                  << "\"  " << skin->joints_count << " joints\n";
    }

    // Build mesh->skin map so we know which skin's l2g to use per primitive
    std::map<const cgltf_mesh*, const cgltf_skin*> meshToSkin;
    for (cgltf_size ni = 0; ni < data->nodes_count; ni++) {
        const cgltf_node* nd = &data->nodes[ni];
        if (nd->mesh && nd->skin) meshToSkin[nd->mesh] = nd->skin;
    }

    // Accumulate total weight per global bone index across all skinned primitives
    std::vector<double> totalWeight((size_t)numBones, 0.0);
    size_t totalVertices = 0;

    for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
        const cgltf_mesh* mesh = &data->meshes[mi];
        const cgltf_skin* meshSkin = nullptr;
        {
            auto sit = meshToSkin.find(mesh);
            if (sit != meshToSkin.end()) meshSkin = sit->second;
        }
        const std::vector<int>* l2g = nullptr;
        if (meshSkin) {
            auto lit = skinLocalToGlobal.find(meshSkin);
            if (lit != skinLocalToGlobal.end()) l2g = &lit->second;
        }

        for (cgltf_size pi = 0; pi < mesh->primitives_count; pi++) {
            const cgltf_primitive* prim = &mesh->primitives[pi];
            const cgltf_accessor* jointsAcc  = nullptr;
            const cgltf_accessor* weightsAcc = nullptr;
            const cgltf_accessor* posAcc     = nullptr;

            for (cgltf_size ai = 0; ai < prim->attributes_count; ai++) {
                if (prim->attributes[ai].type == cgltf_attribute_type_joints)
                    jointsAcc = prim->attributes[ai].data;
                else if (prim->attributes[ai].type == cgltf_attribute_type_weights)
                    weightsAcc = prim->attributes[ai].data;
                else if (prim->attributes[ai].type == cgltf_attribute_type_position)
                    posAcc = prim->attributes[ai].data;
            }

            if (!jointsAcc || !weightsAcc || !posAcc) continue;

            size_t vCount = posAcc->count;
            totalVertices += vCount;

            for (cgltf_size vi = 0; vi < (cgltf_size)vCount; vi++) {
                float joints[4]  = {0,0,0,0};
                float weights[4] = {0,0,0,0};
                cgltf_accessor_read_float(jointsAcc,  vi, joints,  4);
                cgltf_accessor_read_float(weightsAcc, vi, weights, 4);
                for (int j = 0; j < 4; j++) {
                    int localIdx = (int)joints[j];
                    int globalIdx = (l2g && localIdx < (int)l2g->size())
                                    ? (*l2g)[localIdx] : localIdx;
                    if (globalIdx >= 0 && globalIdx < numBones) {
                        totalWeight[(size_t)globalIdx] += (double)weights[j];
                    }
                }
            }
        }
    }

    std::cout << "Total skinned vertices: " << totalVertices << "\n\n";

    // Report bones below/above threshold
    std::vector<int> bonesAbove, bonesBelow;
    for (int bi = 0; bi < numBones; bi++) {
        double normalised = (totalVertices > 0) ? totalWeight[(size_t)bi] / (double)totalVertices : 0.0;
        if (normalised >= (double)threshold)
            bonesAbove.push_back(bi);
        else
            bonesBelow.push_back(bi);
    }

    std::cout << "Bones with weight >= threshold (" << bonesAbove.size() << "):\n";
    for (int bi : bonesAbove) {
        const cgltf_node* jn = globalIndexToNode[(size_t)bi];
        double w = (totalVertices > 0) ? totalWeight[(size_t)bi] / (double)totalVertices : 0.0;
        std::cout << "  [" << std::setw(3) << bi << "] \""
                  << (jn->name ? jn->name : "(unnamed)")
                  << "\"  avg weight: " << std::fixed << std::setprecision(4) << w << "\n";
    }

    if (!bonesBelow.empty()) {
        std::cout << "\nBones below threshold - candidates for removal (" << bonesBelow.size() << "):\n";
        for (int bi : bonesBelow) {
            const cgltf_node* jn = globalIndexToNode[(size_t)bi];
            double w = (totalVertices > 0) ? totalWeight[(size_t)bi] / (double)totalVertices : 0.0;
            std::cout << "  [" << std::setw(3) << bi << "] \""
                      << (jn->name ? jn->name : "(unnamed)")
                      << "\"  avg weight: " << std::fixed << std::setprecision(4) << w << "\n";
        }
    } else {
        std::cout << "\nNo bones below threshold - no simplification recommended.\n";
    }

    // Report if unified bone count exceeds max-bones
    if (numBones > maxBones) {
        std::cout << "\nWARNING: Model has " << numBones << " unique bones but max-bones is " << maxBones
                  << ". Lowest-weight bones to cut:\n";
        std::vector<std::pair<double,int>> sorted;
        for (int bi = 0; bi < numBones; bi++)
            sorted.emplace_back(totalWeight[(size_t)bi], bi);
        std::sort(sorted.begin(), sorted.end());
        int toCut = numBones - maxBones;
        for (int k = 0; k < toCut; k++) {
            int bi = sorted[k].second;
            const cgltf_node* jn = globalIndexToNode[(size_t)bi];
            double w = (totalVertices > 0) ? sorted[k].first / (double)totalVertices : 0.0;
            std::cout << "  [" << std::setw(3) << bi << "] \""
                      << (jn->name ? jn->name : "(unnamed)")
                      << "\"  avg weight: " << std::fixed << std::setprecision(4) << w << "\n";
        }
    } else {
        std::cout << "\nBone count (" << numBones << ") is within max-bones limit (" << maxBones << ").\n";
    }

    cgltf_free(data);
    printSeparator();
    return 0;
}

// ============================================================================
// Help
// ============================================================================

// ============================================================================
// --dump-bones command
// Builds the same unified bone array as PB3D.cpp and prints:
//   - For every bone: global idx, skin source, name, parent global idx, restT, restR
//   - For root bones (parentIndex==-1): non-joint parent node TRS, IBM translation col,
//     and the product [restLocalTRS × IBM] to check if it is identity at bind pose.
// ============================================================================
static void mat4TRS(float tx, float ty, float tz,
                    float qx, float qy, float qz, float qw,
                    float sx, float sy, float sz,
                    float m[16]) {
    m[0]  = sx*(1 - 2*(qy*qy+qz*qz));
    m[1]  = sx*(2*(qx*qy+qw*qz));
    m[2]  = sx*(2*(qx*qz-qw*qy));
    m[3]  = 0.f;
    m[4]  = sy*(2*(qx*qy-qw*qz));
    m[5]  = sy*(1 - 2*(qx*qx+qz*qz));
    m[6]  = sy*(2*(qy*qz+qw*qx));
    m[7]  = 0.f;
    m[8]  = sz*(2*(qx*qz+qw*qy));
    m[9]  = sz*(2*(qy*qz-qw*qx));
    m[10] = sz*(1 - 2*(qx*qx+qy*qy));
    m[11] = 0.f;
    m[12] = tx; m[13] = ty; m[14] = tz; m[15] = 1.f;
}
static void mat4Mul(const float A[16], const float B[16], float C[16]) {
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++) {
            float s = 0.f;
            for (int k = 0; k < 4; k++) s += A[k*4+row] * B[col*4+k];
            C[col*4+row] = s;
        }
}
static bool mat4IsIdentity(const float m[16], float eps = 0.001f) {
    for (int i = 0; i < 16; i++) {
        float expected = ((i%5) == 0) ? 1.f : 0.f;
        if (fabsf(m[i]-expected) > eps) return false;
    }
    return true;
}
static int cmdDumpBones(const char* path) {
    cgltf_data* data = loadGLB(path);
    if (!data) return 1;

    // Build unified nodeToGlobalBone map identical to PB3D.cpp logic
    std::map<const cgltf_node*, int>              nodeToGlobal;
    std::map<const cgltf_skin*, std::vector<int>> skinL2G;
    for (cgltf_size si = 0; si < data->skins_count; si++) {
        const cgltf_skin* skin = &data->skins[si];
        auto& l2g = skinL2G[skin];
        l2g.resize(skin->joints_count, 0);
        for (cgltf_size ji = 0; ji < skin->joints_count; ji++) {
            const cgltf_node* jn = skin->joints[ji];
            auto it = nodeToGlobal.find(jn);
            if (it != nodeToGlobal.end()) {
                l2g[ji] = it->second;
            } else {
                int g = (int)nodeToGlobal.size();
                nodeToGlobal[jn] = g;
                l2g[ji] = g;
            }
        }
    }

    // Per-bone data storage
    struct BoneInfo {
        std::string name;
        int         skinSrc   = -1;
        int         parentIdx = -1;
        float       restT[3]  = {0,0,0};
        float       restR[4]  = {0,0,0,1};
        float       restS[3]  = {1,1,1};
        float       ibm[16]   = {};
        bool        ibmPresent = false;
        // For root bones: non-joint parent
        bool            hasNonJointParent = false;
        std::string     parentNodeName;
        const cgltf_node* parentNodePtr = nullptr;  // for cgltf_node_transform_world call
        float       parentT[3] = {0,0,0};
        float       parentR[4] = {0,0,0,1};
        float       parentS[3] = {1,1,1};
        bool        parentHasT = false, parentHasR = false, parentHasS = false;
    };
    int totalBones = (int)nodeToGlobal.size();
    std::vector<BoneInfo> bones(totalBones);

    for (cgltf_size si = 0; si < data->skins_count; si++) {
        const cgltf_skin* skin = &data->skins[si];
        auto& l2g = skinL2G.at(skin);

        // Read IBM buffer
        std::vector<float> ibmBuf;
        if (skin->inverse_bind_matrices) {
            cgltf_size cnt = skin->inverse_bind_matrices->count;
            ibmBuf.resize(cnt * 16, 0.0f);
            for (cgltf_size ji = 0; ji < cnt; ji++)
                cgltf_accessor_read_float(skin->inverse_bind_matrices, ji, ibmBuf.data() + ji*16, 16);
        }

        for (cgltf_size ji = 0; ji < skin->joints_count; ji++) {
            int gi = l2g[ji];
            if (gi < 0 || gi >= totalBones) continue;
            BoneInfo& b = bones[gi];
            if (!b.name.empty()) continue;  // already filled

            const cgltf_node* jn = skin->joints[ji];
            b.name = jn->name ? jn->name : ("bone_" + std::to_string(gi));
            b.skinSrc = (int)si;

            // Parent index
            if (jn->parent) {
                auto pit = nodeToGlobal.find(jn->parent);
                if (pit != nodeToGlobal.end()) {
                    b.parentIdx = pit->second;
                } else {
                    // Non-joint parent node
                    b.hasNonJointParent = true;
                    const cgltf_node* pn = jn->parent;
                    b.parentNodePtr  = pn;
                    b.parentNodeName = pn->name ? pn->name : "?";
                    b.parentHasT = pn->has_translation;
                    b.parentHasR = pn->has_rotation;
                    b.parentHasS = pn->has_scale;
                    if (pn->has_translation) { b.parentT[0]=pn->translation[0]; b.parentT[1]=pn->translation[1]; b.parentT[2]=pn->translation[2]; }
                    if (pn->has_rotation)    { b.parentR[0]=pn->rotation[0]; b.parentR[1]=pn->rotation[1]; b.parentR[2]=pn->rotation[2]; b.parentR[3]=pn->rotation[3]; }
                    if (pn->has_scale)       { b.parentS[0]=pn->scale[0]; b.parentS[1]=pn->scale[1]; b.parentS[2]=pn->scale[2]; }
                }
            }

            if (jn->has_translation) { b.restT[0]=jn->translation[0]; b.restT[1]=jn->translation[1]; b.restT[2]=jn->translation[2]; }
            if (jn->has_rotation)    { b.restR[0]=jn->rotation[0]; b.restR[1]=jn->rotation[1]; b.restR[2]=jn->rotation[2]; b.restR[3]=jn->rotation[3]; }
            if (jn->has_scale)       { b.restS[0]=jn->scale[0]; b.restS[1]=jn->scale[1]; b.restS[2]=jn->scale[2]; }

            if (!ibmBuf.empty() && ji < ibmBuf.size()/16) {
                memcpy(b.ibm, ibmBuf.data() + ji*16, 64);
                b.ibmPresent = true;
            } else {
                // Identity IBM
                memset(b.ibm, 0, 64);
                b.ibm[0]=b.ibm[5]=b.ibm[10]=b.ibm[15]=1.f;
                b.ibmPresent = false;
            }
        }
    }

    printSeparator();
    std::cout << "Bone dump: " << path << "  (" << totalBones << " unified bones)\n";
    printSeparator();

    // Print root bones first with full detail
    std::cout << "\n--- ROOT BONES (parentIdx == -1) ---\n";
    for (int gi = 0; gi < totalBones; gi++) {
        const auto& b = bones[gi];
        if (b.parentIdx >= 0) continue;
        std::cout << "\nBone[" << gi << "] skin[" << b.skinSrc << "] \"" << b.name << "\"\n";
        std::cout << "  restT: " << b.restT[0] << ", " << b.restT[1] << ", " << b.restT[2] << "\n";
        std::cout << "  restR: " << b.restR[0] << ", " << b.restR[1] << ", " << b.restR[2] << ", " << b.restR[3] << "\n";
        std::cout << "  restS: " << b.restS[0] << ", " << b.restS[1] << ", " << b.restS[2] << "\n";
        if (b.hasNonJointParent) {
            std::cout << "  parentNode: \"" << b.parentNodeName << "\"\n";
            std::cout << "  parentT: " << b.parentT[0] << ", " << b.parentT[1] << ", " << b.parentT[2]
                      << (b.parentHasT ? "" : " (default)") << "\n";
            std::cout << "  parentR: " << b.parentR[0] << ", " << b.parentR[1] << ", " << b.parentR[2] << ", " << b.parentR[3]
                      << (b.parentHasR ? "" : " (default)") << "\n";
            std::cout << "  parentS: " << b.parentS[0] << ", " << b.parentS[1] << ", " << b.parentS[2]
                      << (b.parentHasS ? "" : " (default)") << "\n";
        } else {
            std::cout << "  (no parent node at all)\n";
        }
        if (b.ibmPresent) {
            std::cout << "  IBM col3 (translation): " << b.ibm[12] << ", " << b.ibm[13] << ", " << b.ibm[14] << "\n";
            // Compute restTRS × IBM and check if identity
            float trs[16], prod[16];
            mat4TRS(b.restT[0], b.restT[1], b.restT[2],
                    b.restR[0], b.restR[1], b.restR[2], b.restR[3],
                    b.restS[0], b.restS[1], b.restS[2], trs);
            mat4Mul(trs, b.ibm, prod);
            std::cout << "  restTRS×IBM: T=[" << prod[12] << "," << prod[13] << "," << prod[14] << "]"
                      << "  diag=[" << prod[0] << "," << prod[5] << "," << prod[10] << "," << prod[15] << "]"
                      << (mat4IsIdentity(prod) ? "  --> IDENTITY (ok)" : "  --> NOT identity (armature offset missing!)") << "\n";
            // KEY VERIFICATION: compute cgltf_node_transform_world(parentNode) × restTRS × IBM
            // This is exactly what PB3D.cpp computes as worldMatrix×IBM at bind pose.
            // If = identity, the parentOffsetMatrix=cgltf_node_transform_world fix is correct.
            if (b.parentNodePtr) {
                float parentWorld[16];
                cgltf_node_transform_world(b.parentNodePtr, parentWorld);
                float worldMat[16], skinMat[16];
                mat4Mul(parentWorld, trs, worldMat);   // worldMatrix = parentWorld × restTRS
                mat4Mul(worldMat, b.ibm, skinMat);     // skinMatrix  = worldMatrix × IBM
                // Print full parentWorld matrix
                std::cout << "  parentWorld (full matrix, row-major display):\n";
                for (int r = 0; r < 4; r++) {
                    std::cout << "    [" << parentWorld[r] << ", " << parentWorld[4+r]
                              << ", " << parentWorld[8+r] << ", " << parentWorld[12+r] << "]\n";
                }
                // Print full IBM matrix
                std::cout << "  IBM (full matrix, row-major display):\n";
                for (int r = 0; r < 4; r++) {
                    std::cout << "    [" << b.ibm[r] << ", " << b.ibm[4+r]
                              << ", " << b.ibm[8+r] << ", " << b.ibm[12+r] << "]\n";
                }
                // Print column magnitudes of parentWorld 3×3 block (reveal scale)
                float cMag0 = sqrtf(parentWorld[0]*parentWorld[0]+parentWorld[1]*parentWorld[1]+parentWorld[2]*parentWorld[2]);
                float cMag1 = sqrtf(parentWorld[4]*parentWorld[4]+parentWorld[5]*parentWorld[5]+parentWorld[6]*parentWorld[6]);
                float cMag2 = sqrtf(parentWorld[8]*parentWorld[8]+parentWorld[9]*parentWorld[9]+parentWorld[10]*parentWorld[10]);
                std::cout << "  parentWorld col-magnitudes: " << cMag0 << ", " << cMag1 << ", " << cMag2 << "\n";
                // Check if parent node has any grandparent
                if (b.parentNodePtr->parent) {
                    const cgltf_node* gp = b.parentNodePtr->parent;
                    std::cout << "  grandparent: \"" << (gp->name ? gp->name : "?") << "\""
                              << "  has_matrix=" << (gp->has_matrix ? "true" : "false")
                              << "  has_T=" << (gp->has_translation ? "true" : "false") << "\n";
                    if (gp->has_translation)
                        std::cout << "    gpT=[" << gp->translation[0] << "," << gp->translation[1] << "," << gp->translation[2] << "]\n";
                    if (gp->has_rotation)
                        std::cout << "    gpR=[" << gp->rotation[0] << "," << gp->rotation[1] << "," << gp->rotation[2] << "," << gp->rotation[3] << "]\n";
                    if (gp->has_scale)
                        std::cout << "    gpS=[" << gp->scale[0] << "," << gp->scale[1] << "," << gp->scale[2] << "]\n";
                } else {
                    std::cout << "  parentNode has no grandparent (is at scene root)\n";
                }
                std::cout << "  parentWorld×restTRS×IBM: T=[" << skinMat[12] << "," << skinMat[13] << "," << skinMat[14] << "]"
                          << "  diag=[" << skinMat[0] << "," << skinMat[5] << "," << skinMat[10] << "," << skinMat[15] << "]\n";
                std::cout << "  --> " << (mat4IsIdentity(skinMat) ? "IDENTITY (fix is correct!)" : "NOT identity (mismatch in coordinate spaces!)") << "\n";
            }
        }
    }

    // Summary: show all bones with parent info
    std::cout << "\n--- ALL BONES ---\n";
    std::cout << std::left
              << std::setw(6)  << "GIdx"
              << std::setw(5)  << "Skin"
              << std::setw(6)  << "Par"
              << std::setw(36) << "Name"
              << "RestTranslation\n";
    printSeparator();
    for (int gi = 0; gi < totalBones; gi++) {
        const auto& b = bones[gi];
        std::cout << std::left
                  << std::setw(6) << gi
                  << std::setw(5) << b.skinSrc
                  << std::setw(6) << b.parentIdx
                  << std::setw(36) << b.name.substr(0, 35)
                  << std::fixed << std::setprecision(3)
                  << b.restT[0] << ", " << b.restT[1] << ", " << b.restT[2] << "\n";
    }

    cgltf_free(data);
    printSeparator();
    return 0;
}

static void printHelp(const char* argv0) {
    std::cout << "pb3dutil - 3D model analysis and utility tool for RasPin Pinball\n\n"
              << "Usage:\n"
              << "  " << argv0 << " --info        <file.glb>\n"
              << "  " << argv0 << " --list-clips  <file.glb>\n"
              << "  " << argv0 << " --dump-bones  <file.glb>\n"
              << "  " << argv0 << " --simplify-bones <file.glb> [--max-bones N] [--threshold F]\n"
              << "  " << argv0 << " --help\n\n"
              << "Commands:\n"
              << "  --info           Print full model info (meshes, materials, textures, bones, clips)\n"
              << "  --list-clips     List animation clips with name, duration, and channel count\n"
              << "  --dump-bones     Dump unified bone hierarchy and check bind-pose correctness\n"
              << "  --simplify-bones Analyse which bones are candidates for removal\n"
              << "                   --max-bones N   Maximum bone count target (default: 160)\n"
              << "                   --threshold F   Min normalised weight to keep bone (default: 0.01)\n"
              << "  --help           Print this help message\n\n"
              << "Supported formats: GLB (glTF 2.0 binary)\n";
}

// ============================================================================
// main
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "--help" || cmd == "-h") {
        printHelp(argv[0]);
        return 0;
    }

    if (cmd == "--info") {
        if (argc < 3) {
            std::cerr << "Error: --info requires a file path\n";
            return 1;
        }
        return cmdInfo(argv[2]);
    }

    if (cmd == "--list-clips") {
        if (argc < 3) {
            std::cerr << "Error: --list-clips requires a file path\n";
            return 1;
        }
        return cmdListClips(argv[2]);
    }

    if (cmd == "--dump-bones") {
        if (argc < 3) {
            std::cerr << "Error: --dump-bones requires a file path\n";
            return 1;
        }
        return cmdDumpBones(argv[2]);
    }

    if (cmd == "--simplify-bones") {
        if (argc < 3) {
            std::cerr << "Error: --simplify-bones requires a file path\n";
            return 1;
        }
        const char* filePath  = argv[2];
        int   maxBones  = 160;
        float threshold = 0.01f;

        for (int i = 3; i < argc; i++) {
            std::string opt = argv[i];
            if ((opt == "--max-bones") && i + 1 < argc) {
                try {
                    maxBones = std::stoi(argv[++i]);
                } catch (...) {
                    std::cerr << "Error: invalid value for --max-bones: " << argv[i] << "\n";
                    return 1;
                }
            } else if ((opt == "--threshold") && i + 1 < argc) {
                try {
                    threshold = std::stof(argv[++i]);
                } catch (...) {
                    std::cerr << "Error: invalid value for --threshold: " << argv[i] << "\n";
                    return 1;
                }
            }
        }

        return cmdSimplifyBones(filePath, maxBones, threshold);
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    printHelp(argv[0]);
    return 1;
}
