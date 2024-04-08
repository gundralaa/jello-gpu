#pragma once

#include "includes.h"
#include "utils.hpp"
#include "constructs.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Lighting
{
    // create gpu buffers + vao
    // copy shader data to vertex buffer
    // get position data
    // compile shader
    // get light data
    // get camera data
    // calculate normals
    // calculate depth buffer

    public:
    
    // data
    uint32_t numPos = 0;
    uint32_t numFaces = 0;

    Face *faces = nullptr;
    glm::vec4 *norms = nullptr;
    glm::vec4 *pos = nullptr;
    
    // ids
    GLuint shaderProgramId = 0; 
    GLuint vboNorm = 0;
    GLuint vboVert = 0;
    GLuint vao = 0;
    GLuint ebo = 0;

    // uniform ids
    // vert
    GLint modelUniId;
    GLint viewProjUniId;
    // frag
    GLint lightPosUniId;
    GLint camPosUniId;
    GLint colorUniId;
    GLint lightIntensityUniId;

    bool wireframe = true;

    Lighting();
    ~Lighting();

    void initProgram();
    int createGlBuffers();
    int createCpuBuffers(uint32_t nv, uint32_t nf);
    void render();

    private:
    glm::vec4 lightCenter = glm::vec4(-1, -1, 10, 1);
    glm::vec4 lightDisp = glm::vec4(0, 0, 1, 0); 
    glm::vec4 lightPos = lightCenter + lightDisp; 
    
    enum AttrLoc {
        POSITION = 0,
        NORMAL = 1,
        TANGENT = 2,
        UV = 3,
    };

    const std::string vertShader = "./shaders/base.vert";
    const std::string fragShader = "./shaders/base.frag";
    std::string prelude = "#version 460\n";

    void calcNorms();
    void calcDepth();
    void getCamera();
    void getLights();
};