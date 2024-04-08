#pragma once

#include "includes.h"
#include "utils.hpp"
#include "constructs.h"

#include <math.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

class Simulator
{
    // Constants
    const struct
    {
        unsigned width = 1000, height = 1000;
        std::string title = "Jello Simulator";
    } window_config;

    const struct
    {
        std::string vertex = "./shaders/base.vert";
        std::string fragment = "./shaders/diffuse.frag";
        std::string gravity = "./shaders/gravity.comp";
        std::string springs = "./shaders/springs.comp";
        std::string integrate = "./shaders/integrate.comp";
        std::string collide = "./shaders/collide.comp";
        std::string correct = "./shaders/correct.comp";
    } shader_config;

    const struct
    {
        struct
        {
            float x = 0, y = 0, z = 10;
            size_t masses_x = 8, masses_y = 8, masses_z = 8;
            float width = 2, height = 2, depth = 2;
            // block_radius must be at least 2
            unsigned block_radius = 2;
            unsigned block_length = block_radius * 2;
            unsigned block_width = std::ceil((float)masses_x / block_length);
            unsigned block_height = std::ceil((float)masses_y / block_length);
            unsigned block_depth = std::ceil((float)masses_z / block_length);
            bool sphere = false;
        } jello;

        glm::vec4 planes[1]{
            glm::vec4(0.31224989992, 0.95, 0, -3)}; // xyz = normal, xyz * w = point;
        size_t planes_count = sizeof(planes) / sizeof(glm::vec4);

        glm::vec4 spheres[1]{
            glm::vec4(0, -2, 10, 1)}; // xyz = center, w = radius
        size_t spheres_count = sizeof(spheres) / sizeof(glm::vec4);
        unsigned sphere_precision = 16;

        glm::vec4 light_position = glm::vec4(2, 0, 9, 0);
    } scene_config;

    // Info
    struct
    {
        GLuint positions;
        GLuint last_positions;
        GLuint forces;
        GLuint springs;
        GLuint planes;
        GLuint spheres;
        GLuint vertices;
        GLuint normals;
        GLuint colors;
        GLuint faces;
    } buffers;

    struct
    {
        GLuint render;
        GLuint gravity;
        GLuint springs;
        GLuint collide;
        GLuint integrate;
        GLuint correct;
    } programIDs;

    GLFWwindow *window;

    // Data
    struct
    {
        struct
        {
            glm::vec4 *positions = nullptr;
            size_t position_count = 0;
            glm::vec4 *normals = nullptr;
            glm::vec4 *colors = nullptr;
            union
            {
                size_t vertex_count = 0;
                size_t normal_count;
                size_t color_count;
            };
            Spring *springs = nullptr;
            size_t spring_count = 0;
            Face *faces = nullptr;
            size_t face_count = 0;
        } jello;

        struct
        {
            glm::vec4 *vertices = nullptr;
            glm::vec4 *normals = nullptr;
            glm::vec4 *colors = nullptr;
            union
            {
                size_t vertex_count = 0;
                size_t normal_count;
                size_t color_count;
            };

            Face *faces = nullptr;
            size_t face_count = 0;
        } planes;

        struct
        {
            glm::vec4 *vertices = nullptr;
            glm::vec4 *normals = nullptr;
            glm::vec4 *colors = nullptr;
            union
            {
                size_t vertex_count = 0;
                size_t normal_count;
                size_t color_count;
            };

            Face *faces = nullptr;
            size_t face_count = 0;
        } spheres;
    } GPU_data;

    // Functions
    int initGL();
    int constructCube();
    int constructScene();
    int loadShaders();
    int makeBuffers();

    void updateNormals();

    // Helper Functions
    inline unsigned getPositionIndex(unsigned x, unsigned y, unsigned z) const;
    inline unsigned getSphereIndex(unsigned x, unsigned y, unsigned z, unsigned i) const;

public:
    Simulator();
    ~Simulator();

    // Core Functionality
    int init();
    int run();
    bool running() const;
};