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

static void printHelp(const char* argv0) {
    std::cout << "pb3dutil - 3D model analysis and utility tool for RasPin Pinball\n\n"
              << "Usage:\n"
              << "  " << argv0 << " --info        <file.glb>\n"
              << "  " << argv0 << " --list-clips  <file.glb>\n"
              << "  " << argv0 << " --simplify-bones <file.glb> [--max-bones N] [--threshold F]\n"
              << "  " << argv0 << " --help\n\n"
              << "Commands:\n"
              << "  --info           Print full model info (meshes, materials, textures, bones, clips)\n"
              << "  --list-clips     List animation clips with name, duration, and channel count\n"
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
