#include "simulator.hpp"

Simulator::Simulator() {}

Simulator::~Simulator()
{
    delete light;
    if (GPU_data.springs)
        free(GPU_data.springs);

    if (GPU_data.planes.faces)
        free(GPU_data.planes.faces);

    if (GPU_data.planes.vertices)
        free(GPU_data.planes.vertices);

    if (GPU_data.planes.normals)
        free(GPU_data.planes.normals);

    if (GPU_data.planes.colors)
        free(GPU_data.planes.colors);

    if (GPU_data.spheres.faces)
        free(GPU_data.spheres.faces);

    if (GPU_data.spheres.vertices)
        free(GPU_data.spheres.vertices);

    if (GPU_data.spheres.normals)
        free(GPU_data.spheres.normals);

    if (GPU_data.spheres.colors)
        free(GPU_data.spheres.colors);

    // Release buffers
    glDeleteBuffers(10, (GLuint *)&buffers);

    glfwTerminate();
}

int Simulator::init()
{
    int errorCode;

    // Initialize glfw
    errorCode = initGL();
    if (errorCode)
        return errorCode;

    // Construct cube
    errorCode = constructCube();
    if (errorCode)
        return errorCode;

    // Construct scene
    errorCode = constructScene();
    if (errorCode)
        return errorCode;

    // Load Shaders
    errorCode = loadShaders();
    if (errorCode)
        return errorCode;

    // Load Data onto GPU
    errorCode = makeBuffers();
    if (errorCode)
        return errorCode;

    return 0;
}

int Simulator::run()
{
    // Clear screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Add gravity
    glUseProgram(programIDs.gravity);
    glDispatchCompute(GPU_data.jello.position_count, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Apply springs
    glUseProgram(programIDs.springs);
    for (GLuint i = 0; i < 8; i++)
    {
        glUniform1ui(0, i);
        glDispatchCompute(scene_config.jello.block_width * scene_config.jello.block_height * scene_config.jello.block_depth, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Apply forces
    glUseProgram(programIDs.integrate);
    glDispatchCompute(GPU_data.jello.position_count, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Collide
    glUseProgram(programIDs.collide);
    glDispatchCompute(GPU_data.jello.position_count, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Apply corrections
    glUseProgram(programIDs.correct);
    glDispatchCompute(GPU_data.jello.position_count, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Render
    updateNormals();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.positions);
    glBindBuffer(GL_ARRAY_BUFFER, buffers.vertices);
    glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_ARRAY_BUFFER, 0, 0, sizeof(glm::vec4) * GPU_data.jello.position_count);
    glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * GPU_data.jello.position_count, sizeof(glm::vec4) * GPU_data.jello.position_count);
    glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_ARRAY_BUFFER, 0, 2 * sizeof(glm::vec4) * GPU_data.jello.position_count, sizeof(glm::vec4) * GPU_data.jello.position_count);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glUseProgram(programIDs.render);

    glUniform1f(0, scene_config.light_position.x);
    glUniform1f(1, scene_config.light_position.y);
    glUniform1f(2, scene_config.light_position.z);
    glUniform1f(3, scene_config.light_position.w);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.faces);
    glDrawElements(GL_TRIANGLES, sizeof(Face) * (GPU_data.jello.face_count + GPU_data.planes.face_count + GPU_data.spheres.face_count), GL_UNSIGNED_INT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Update window
    glfwSwapBuffers(window);
    glfwPollEvents();

    getErrors("Run");

    return 0;
}

void Simulator::updateNormals()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.positions);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * GPU_data.jello.position_count, GPU_data.jello.positions);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    for (int i = 0; i < GPU_data.jello.position_count * 3; i++)
    {
        GPU_data.jello.normals[i] = glm::vec4(0.0, 0.0, 0.0, 0.0);
    }

    for (int i = 0; i < GPU_data.jello.face_count; i++)
    {
        Face face = GPU_data.jello.faces[i];
        glm::vec4 v1 = GPU_data.jello.positions[face.index1 % GPU_data.jello.position_count];
        glm::vec4 v2 = GPU_data.jello.positions[face.index2 % GPU_data.jello.position_count];
        glm::vec4 v3 = GPU_data.jello.positions[face.index3 % GPU_data.jello.position_count];

        glm::vec3 e1 = glm::vec3(v3 - v1);
        glm::vec3 e2 = glm::vec3(v2 - v1);

        glm::vec4 normal = glm::vec4(glm::cross(e1, e2), 0.0f);

        GPU_data.jello.normals[face.index1] += normal;
        GPU_data.jello.normals[face.index2] += normal;
        GPU_data.jello.normals[face.index3] += normal;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffers.normals);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * GPU_data.jello.normal_count, GPU_data.jello.normals);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool Simulator::running() const
{
    return glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window);
}

int Simulator::initGL()
{
    if (!glfwInit())
        return -10;

    glfwWindowHint(GLFW_SAMPLES, 4);
    window = glfwCreateWindow(window_config.width, window_config.height, window_config.title.c_str(), NULL, NULL);

    if (!window)
        return -11;

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -12;

    const GLubyte *version = glGetString(GL_VERSION);
    printf("OpenGL Version: %s\n", version);

    return 0;
}

int Simulator::makeCpuBuffers()
{
    GPU_data.position_count = cube_config.masses_x * cube_config.masses_y * cube_config.masses_z;
    GPU_data.spring_count = cube_config.block_width * cube_config.block_height * cube_config.block_depth * cube_config.block_length * cube_config.block_length * cube_config.block_length * 12;    
    GPU_data.face_count = ((cube_config.masses_x - 1) * (cube_config.masses_y - 1) + (cube_config.masses_y - 1) * (cube_config.masses_z - 1) + (cube_config.masses_z - 1) * (cube_config.masses_x - 1)) * 4; 
    
    // create lighting buffers 
    if (light->createCpuBuffers(GPU_data.position_count, GPU_data.face_count)) return 1;
    
    // buffer allocation  
    GPU_data.positions = light->pos;
    if (!GPU_data.positions) return 1;
    
    GPU_data.faces = light->faces;
    if (!GPU_data.faces) return 1;

    GPU_data.springs = (Spring *)malloc(sizeof(Spring) * GPU_data.spring_count);
    if (!GPU_data.springs) return 1;

    return 0;
}

int Simulator::constructCube()
{
    GPU_data.jello.position_count = scene_config.jello.masses_x * scene_config.jello.masses_y * scene_config.jello.masses_z;
    GPU_data.jello.positions = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.jello.position_count);
    GPU_data.jello.normal_count = GPU_data.jello.position_count * 3;
    GPU_data.jello.normals = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.jello.normal_count);
    GPU_data.jello.color_count = GPU_data.jello.position_count * 3;
    GPU_data.jello.colors = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.jello.color_count);
    GPU_data.jello.face_count = ((scene_config.jello.masses_x - 1) * (scene_config.jello.masses_y - 1) + (scene_config.jello.masses_y - 1) * (scene_config.jello.masses_z - 1) + (scene_config.jello.masses_z - 1) * (scene_config.jello.masses_x - 1)) * 4;
    GPU_data.jello.faces = (Face *)malloc(sizeof(Face) * GPU_data.jello.face_count);
    GPU_data.jello.spring_count = scene_config.jello.block_width * scene_config.jello.block_height * scene_config.jello.block_depth * scene_config.jello.block_length * scene_config.jello.block_length * scene_config.jello.block_length * 12;
    GPU_data.jello.springs = (Spring *)malloc(sizeof(Spring) * GPU_data.jello.spring_count);

    for (int i = 0; i < GPU_data.jello.spring_count; i++)
    {
        GPU_data.jello.springs[i] = Spring(0, 1, 0, 1);
    }

    if (!scene_config.jello.sphere)
    {
        float spring_lengths[12]{
            scene_config.jello.width / (scene_config.jello.masses_x - 1),
            scene_config.jello.height / (scene_config.jello.masses_y - 1),
            scene_config.jello.depth / (scene_config.jello.masses_z - 1),
            sqrt(scene_config.jello.width * scene_config.jello.width / ((scene_config.jello.masses_x - 1) * (scene_config.jello.masses_x - 1)) + scene_config.jello.height * scene_config.jello.height / ((scene_config.jello.masses_y - 1) * (scene_config.jello.masses_y - 1))),
            sqrt(scene_config.jello.width * scene_config.jello.width / ((scene_config.jello.masses_x - 1) * (scene_config.jello.masses_x - 1)) + scene_config.jello.depth * scene_config.jello.depth / ((scene_config.jello.masses_z - 1) * (scene_config.jello.masses_z - 1))),
            sqrt(scene_config.jello.height * scene_config.jello.height / ((scene_config.jello.masses_y - 1) * (scene_config.jello.masses_y - 1)) + scene_config.jello.depth * scene_config.jello.depth / ((scene_config.jello.masses_z - 1) * (scene_config.jello.masses_z - 1))),
        };

        for (int z = 0; z < scene_config.jello.masses_z; z++)
        {
            for (int y = 0; y < scene_config.jello.masses_y; y++)
            {
                for (int x = 0; x < scene_config.jello.masses_x; x++)
                {
                    GPU_data.jello.positions[getPositionIndex(x, y, z)] = glm::vec4(x * scene_config.jello.width / (scene_config.jello.masses_x - 1) + scene_config.jello.x - scene_config.jello.width/2,
                                                                                   y * scene_config.jello.height / (scene_config.jello.masses_y - 1) + scene_config.jello.y - scene_config.jello.height/2,
                                                                                   z * scene_config.jello.depth / (scene_config.jello.masses_z - 1) + scene_config.jello.z - scene_config.jello.depth/2, 1.0);

                    GPU_data.jello.colors[getPositionIndex(x, y, z)] = glm::vec4(0.3, 1.0, 0.3, 1.0);
                    GPU_data.jello.colors[getPositionIndex(x, y, z) + GPU_data.jello.position_count] = glm::vec4(0.3, 1.0, 0.3, 1.0);
                    GPU_data.jello.colors[getPositionIndex(x, y, z) + GPU_data.jello.position_count * 2] = glm::vec4(0.3, 1.0, 0.3, 1.0);

                    unsigned spring_index = 12 * ((x % scene_config.jello.block_radius) + scene_config.jello.block_radius * ((y % scene_config.jello.block_radius) + scene_config.jello.block_radius * ((z % scene_config.jello.block_radius) + scene_config.jello.block_radius * (((x / scene_config.jello.block_radius) % 2) + 2 * (((y / scene_config.jello.block_radius) % 2) + 2 * (((z / scene_config.jello.block_radius) % 2) + 2 * ((x / scene_config.jello.block_length) + scene_config.jello.block_width * ((y / scene_config.jello.block_length) + scene_config.jello.block_height * ((z / scene_config.jello.block_length))))))))));

                    // (x, y, z) springs to: (x-1, y, z), (x, y-1, z), (x, y, z-1), (x-1, y-1, z), (x-1, y, z-1), (x, y-1, z-1), (x+1, y-1, z), (x-1, y, z+1), (x, y+1, z-1), (x-2, y, z), (x, y-2, z), (x, y, z-2)
                    if (x - 1 >= 0)
                    {
                        // (x-1, y, z) #0
                        GPU_data.jello.springs[spring_index] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 1, y, z), 1, spring_lengths[0]);
                        if (x - 2 >= 0)
                        {
                            // (x-2, y, z) #9
                            GPU_data.jello.springs[spring_index + 9] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 2, y, z), 3, spring_lengths[0] * 2);
                        }
                        if (z - 1 >= 0)
                        {
                            // (x-1, y, z-1) #4
                            GPU_data.jello.springs[spring_index + 4] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 1, y, z - 1), 2, spring_lengths[3]);
                        }
                        else if (z + 1 <= scene_config.jello.masses_z)
                        {
                            // (x-1, y, z+1) #7
                            GPU_data.jello.springs[spring_index + 7] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 1, y, z + 1), 2, spring_lengths[3]);
                        }
                    }
                    if (y - 1 >= 0)
                    {
                        // (x, y-1, z) #1
                        GPU_data.jello.springs[spring_index + 1] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y - 1, z), 1, spring_lengths[1]);
                        if (y - 2 >= 0)
                        {
                            // (x, y-2, z) #10
                            GPU_data.jello.springs[spring_index + 10] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y - 2, z), 3, spring_lengths[1] * 2);
                        }
                        if (x - 1 >= 0)
                        {
                            // (x-1, y-1, z) #3
                            GPU_data.jello.springs[spring_index + 3] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 1, y - 1, z), 2, spring_lengths[4]);
                        }
                        else if (x + 1 <= scene_config.jello.masses_x)
                        {
                            // (x+1, y-1, z) #6
                            GPU_data.jello.springs[spring_index + 6] = Spring(getPositionIndex(x, y, z), getPositionIndex(x + 1, y - 1, z), 2, spring_lengths[4]);
                        }
                    }
                    if (z - 1 >= 0)
                    {
                        // (x, y, z-1) #2
                        GPU_data.jello.springs[spring_index + 2] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y, z - 1), 1, spring_lengths[2]);
                        if (z - 2 >= 0)
                        {
                            // (x, y, z-2) #11
                            GPU_data.jello.springs[spring_index + 11] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y, z - 2), 3, spring_lengths[2] * 2);
                        }
                        if (y - 1 >= 0)
                        {
                            // (x, y-1, z-1) #5
                            GPU_data.jello.springs[spring_index + 5] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y - 1, z - 1), 2, spring_lengths[5]);
                        }
                        else if (y + 1 < scene_config.jello.masses_y)
                        {
                            // (x, y+1, z-1) #8
                            GPU_data.jello.springs[spring_index + 8] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y + 1, z - 1), 2, spring_lengths[5]);
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (int z = 0; z < scene_config.jello.masses_z; z++)
        {
            for (int y = 0; y < scene_config.jello.masses_y; y++)
            {
                for (int x = 0; x < scene_config.jello.masses_x; x++)
                {
                    glm::vec3 relative_coordinates = glm::vec3((((float)x - (float)(scene_config.jello.masses_x - 1) / 2)),
                                                               (((float)y - (float)(scene_config.jello.masses_y - 1) / 2)),
                                                               (((float)z - (float)(scene_config.jello.masses_z - 1) / 2)));
                    float scale = std::max(std::max(abs((float)x/(scene_config.jello.masses_x - 1) - 0.5f), abs((float)y/(scene_config.jello.masses_y - 1) - 0.5f)), abs((float)z/(scene_config.jello.masses_z - 1) - 0.5f));
                    relative_coordinates = glm::normalize(relative_coordinates) * scale;
                    GPU_data.jello.positions[getPositionIndex(x, y, z)] = glm::vec4(relative_coordinates.x * scene_config.jello.width + scene_config.jello.x,
                                                                                   relative_coordinates.y * scene_config.jello.height + scene_config.jello.y,
                                                                                   relative_coordinates.z * scene_config.jello.depth + scene_config.jello.z, 1.0);

                    GPU_data.jello.colors[getPositionIndex(x, y, z)] = glm::vec4(0.3, 1.0, 0.3, 1.0);
                    GPU_data.jello.colors[getPositionIndex(x, y, z) + GPU_data.jello.position_count] = glm::vec4(0.3, 1.0, 0.3, 1.0);
                    GPU_data.jello.colors[getPositionIndex(x, y, z) + GPU_data.jello.position_count * 2] = glm::vec4(0.3, 1.0, 0.3, 1.0);
                }
            }
        }
        for (int z = 0; z < scene_config.jello.masses_z; z++)
        {
            for (int y = 0; y < scene_config.jello.masses_y; y++)
            {
                for (int x = 0; x < scene_config.jello.masses_x; x++)
                {
                    unsigned spring_index = 12 * ((x % scene_config.jello.block_radius) + scene_config.jello.block_radius * ((y % scene_config.jello.block_radius) + scene_config.jello.block_radius * ((z % scene_config.jello.block_radius) + scene_config.jello.block_radius * (((x / scene_config.jello.block_radius) % 2) + 2 * (((y / scene_config.jello.block_radius) % 2) + 2 * (((z / scene_config.jello.block_radius) % 2) + 2 * ((x / scene_config.jello.block_length) + scene_config.jello.block_width * ((y / scene_config.jello.block_length) + scene_config.jello.block_height * ((z / scene_config.jello.block_length))))))))));

                    // (x, y, z) springs to: (x-1, y, z), (x, y-1, z), (x, y, z-1), (x-1, y-1, z), (x-1, y, z-1), (x, y-1, z-1), (x+1, y-1, z), (x-1, y, z+1), (x, y+1, z-1), (x-2, y, z), (x, y-2, z), (x, y, z-2)
                    if (x - 1 >= 0)
                    {
                        // (x-1, y, z) #0
                        GPU_data.jello.springs[spring_index] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 1, y, z), 1, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x - 1, y, z)]));
                        if (x - 2 >= 0)
                        {
                            // (x-2, y, z) #9
                            GPU_data.jello.springs[spring_index + 9] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 2, y, z), 3, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x - 2, y, z)]));
                        }
                        if (z - 1 >= 0)
                        {
                            // (x-1, y, z-1) #4
                            GPU_data.jello.springs[spring_index + 4] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 1, y, z - 1), 2, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x - 1, y, z - 1)]));
                        }
                        else if (z + 1 <= scene_config.jello.masses_z)
                        {
                            // (x-1, y, z+1) #7
                            GPU_data.jello.springs[spring_index + 7] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 1, y, z + 1), 2, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x - 1, y, z + 1)]));
                        }
                    }
                    if (y - 1 >= 0)
                    {
                        // (x, y-1, z) #1
                        GPU_data.jello.springs[spring_index + 1] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y - 1, z), 1, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x, y - 1, z)]));
                        if (y - 2 >= 0)
                        {
                            // (x, y-2, z) #10
                            GPU_data.jello.springs[spring_index + 10] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y - 2, z), 3, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x, y - 2, z)]));
                        }
                        if (x - 1 >= 0)
                        {
                            // (x-1, y-1, z) #3
                            GPU_data.jello.springs[spring_index + 3] = Spring(getPositionIndex(x, y, z), getPositionIndex(x - 1, y - 1, z), 2, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x - 1, y - 1, z)]));
                        }
                        else if (x + 1 <= scene_config.jello.masses_x)
                        {
                            // (x+1, y-1, z) #6
                            GPU_data.jello.springs[spring_index + 6] = Spring(getPositionIndex(x, y, z), getPositionIndex(x + 1, y - 1, z), 2, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x + 1, y - 1, z)]));
                        }
                    }
                    if (z - 1 >= 0)
                    {
                        // (x, y, z-1) #2
                        GPU_data.jello.springs[spring_index + 2] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y, z - 1), 1, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x, y, z - 1)]));
                        if (z - 2 >= 0)
                        {
                            // (x, y, z-2) #11
                            GPU_data.jello.springs[spring_index + 11] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y, z - 2), 3, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x, y, z - 2)]));
                        }
                        if (y - 1 >= 0)
                        {
                            // (x, y-1, z-1) #5
                            GPU_data.jello.springs[spring_index + 5] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y - 1, z - 1), 2, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x, y - 1, z - 1)]));
                        }
                        else if (y + 1 < scene_config.jello.masses_y)
                        {
                            // (x, y+1, z-1) #8
                            GPU_data.jello.springs[spring_index + 8] = Spring(getPositionIndex(x, y, z), getPositionIndex(x, y + 1, z - 1), 2, glm::distance(GPU_data.jello.positions[getPositionIndex(x, y, z)], GPU_data.jello.positions[getPositionIndex(x, y + 1, z - 1)]));
                        }
                    }
                }
            }
        }
    }

    unsigned i = 0;
    unsigned l = 0;
    for (unsigned y = 0; y < scene_config.jello.masses_y - 1; y++)
    {
        for (unsigned x = 0; x < scene_config.jello.masses_x - 1; x++)
        {
            GPU_data.jello.faces[i++] = Face(getPositionIndex(x, y, 0) + l, getPositionIndex(x + 1, y, 0) + l, getPositionIndex(x + 1, y + 1, 0) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(x, y, 0) + l, getPositionIndex(x + 1, y + 1, 0) + l, getPositionIndex(x, y + 1, 0) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(x, y, scene_config.jello.masses_z - 1) + l, getPositionIndex(x + 1, y + 1, scene_config.jello.masses_z - 1) + l, getPositionIndex(x + 1, y, scene_config.jello.masses_z - 1) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(x, y, scene_config.jello.masses_z - 1) + l, getPositionIndex(x, y + 1, scene_config.jello.masses_z - 1) + l, getPositionIndex(x + 1, y + 1, scene_config.jello.masses_z - 1) + l);
        }
    }
    l = GPU_data.jello.position_count;
    for (unsigned z = 0; z < scene_config.jello.masses_z - 1; z++)
    {
        for (unsigned x = 0; x < scene_config.jello.masses_x - 1; x++)
        {
            GPU_data.jello.faces[i++] = Face(getPositionIndex(x, 0, z) + l, getPositionIndex(x, 0, z + 1) + l, getPositionIndex(x + 1, 0, z + 1) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(x, 0, z) + l, getPositionIndex(x + 1, 0, z + 1) + l, getPositionIndex(x + 1, 0, z) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(x, scene_config.jello.masses_y - 1, z) + l, getPositionIndex(x + 1, scene_config.jello.masses_y - 1, z + 1) + l, getPositionIndex(x, scene_config.jello.masses_y - 1, z + 1) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(x, scene_config.jello.masses_y - 1, z) + l, getPositionIndex(x + 1, scene_config.jello.masses_y - 1, z) + l, getPositionIndex(x + 1, scene_config.jello.masses_y - 1, z + 1) + l);
        }
    }
    l = GPU_data.jello.position_count * 2;
    for (unsigned z = 0; z < scene_config.jello.masses_z - 1; z++)
    {
        for (unsigned y = 0; y < scene_config.jello.masses_y - 1; y++)
        {
            GPU_data.jello.faces[i++] = Face(getPositionIndex(0, y, z) + l, getPositionIndex(0, y + 1, z) + l, getPositionIndex(0, y + 1, z + 1) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(0, y, z) + l, getPositionIndex(0, y + 1, z + 1) + l, getPositionIndex(0, y, z + 1) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(scene_config.jello.masses_z - 1, y, z) + l, getPositionIndex(scene_config.jello.masses_z - 1, y + 1, z + 1), getPositionIndex(scene_config.jello.masses_z - 1, y + 1, z) + l);
            GPU_data.jello.faces[i++] = Face(getPositionIndex(scene_config.jello.masses_z - 1, y, z) + l, getPositionIndex(scene_config.jello.masses_z - 1, y, z + 1), getPositionIndex(scene_config.jello.masses_z - 1, y + 1, z + 1) + l);
        }
    }

    return 0;
}

int Simulator::constructScene()
{
    GPU_data.planes.vertex_count = scene_config.planes_count * 3;
    GPU_data.planes.vertices = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.planes.vertex_count);
    GPU_data.planes.normal_count = scene_config.planes_count * 3;
    GPU_data.planes.normals = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.planes.normal_count);
    GPU_data.planes.color_count = scene_config.planes_count * 3;
    GPU_data.planes.colors = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.planes.color_count);
    GPU_data.planes.face_count = scene_config.planes_count;
    GPU_data.planes.faces = (Face *)malloc(sizeof(Face) * GPU_data.planes.face_count);

    unsigned l = GPU_data.jello.vertex_count;
    for (unsigned i = 0; i < scene_config.planes_count; i++)
    {
        glm::vec4 basis1{0, scene_config.planes[i].z, -scene_config.planes[i].y, 0},
            basis2{scene_config.planes[i].y, -scene_config.planes[i].x, 0, 0},
            offset{scene_config.planes[i].x * scene_config.planes[i].w, scene_config.planes[i].y * scene_config.planes[i].w, scene_config.planes[i].z * scene_config.planes[i].w, 0},
            normal{scene_config.planes[i].x, scene_config.planes[i].y, scene_config.planes[i].z, 0};

        GPU_data.planes.vertices[i * 3] = basis1 * 100.0f + offset;
        GPU_data.planes.vertices[i * 3 + 1] = basis1 * -100.0f + basis2 * 1000.0f + offset;
        GPU_data.planes.vertices[i * 3 + 2] = basis1 * -100.0f + basis2 * -100.0f + offset;

        GPU_data.planes.normals[i * 3] = normal;
        GPU_data.planes.normals[i * 3 + 1] = normal;
        GPU_data.planes.normals[i * 3 + 2] = normal;

        GPU_data.planes.colors[i * 3] = glm::vec4(1.0, 1.0, 1.0, 1.0);
        GPU_data.planes.colors[i * 3 + 1] = glm::vec4(1.0, 1.0, 1.0, 1.0);
        GPU_data.planes.colors[i * 3 + 2] = glm::vec4(1.0, 1.0, 1.0, 1.0);

        GPU_data.planes.faces[i] = Face(l + i * 3, l + i * 3 + 1, l + i * 3 + 2);
    }

    GPU_data.spheres.vertex_count = scene_config.spheres_count * scene_config.sphere_precision * scene_config.sphere_precision * scene_config.sphere_precision;
    GPU_data.spheres.vertices = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.spheres.vertex_count);
    GPU_data.spheres.normal_count = scene_config.spheres_count * scene_config.sphere_precision * scene_config.sphere_precision * scene_config.sphere_precision;
    GPU_data.spheres.normals = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.spheres.normal_count);
    GPU_data.spheres.color_count = scene_config.spheres_count * scene_config.sphere_precision * scene_config.sphere_precision * scene_config.sphere_precision;
    GPU_data.spheres.colors = (glm::vec4 *)malloc(sizeof(glm::vec4) * GPU_data.spheres.color_count);
    GPU_data.spheres.face_count = scene_config.spheres_count * (scene_config.sphere_precision - 1) * (scene_config.sphere_precision - 1) * 12;
    GPU_data.spheres.faces = (Face *)malloc(sizeof(Face) * GPU_data.spheres.face_count);

    unsigned i = 0;
    l += GPU_data.planes.vertex_count;
    for (unsigned j = 0; j < scene_config.spheres_count; j++)
    {
        for (unsigned z = 0; z < scene_config.sphere_precision; z++)
        {
            for (unsigned y = 0; y < scene_config.sphere_precision; y++)
            {
                for (unsigned x = 0; x < scene_config.sphere_precision; x++)
                {
                    glm::vec3 relative_coordinates = glm::normalize(glm::vec3((float)x - (float)(scene_config.sphere_precision - 1) / 2,
                                                                              (float)y - (float)(scene_config.sphere_precision - 1) / 2,
                                                                              (float)z - (float)(scene_config.sphere_precision - 1) / 2));
                    GPU_data.spheres.vertices[getSphereIndex(x, y, z, j)] = glm::vec4(relative_coordinates.x * scene_config.spheres[j].w + scene_config.spheres[j].x,
                                                                                      relative_coordinates.y * scene_config.spheres[j].w + scene_config.spheres[j].y,
                                                                                      relative_coordinates.z * scene_config.spheres[j].w + scene_config.spheres[j].z, 1.0);
                    GPU_data.spheres.normals[getSphereIndex(x, y, z, j)] = glm::vec4(relative_coordinates.x, relative_coordinates.y, relative_coordinates.z, 0.0);
                    GPU_data.spheres.colors[getSphereIndex(x, y, z, j)] = glm::vec4(1.0, 1.0, 1.0, 1.0);
                }
            }
        }
        for (unsigned y = 0; y < scene_config.sphere_precision - 1; y++)
        {
            for (unsigned x = 0; x < scene_config.sphere_precision - 1; x++)
            {
                unsigned index = x + scene_config.sphere_precision * y;
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(x, y, 0, j) + l, getSphereIndex(x + 1, y, 0, j) + l, getSphereIndex(x + 1, y + 1, 0, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(x, y, 0, j) + l, getSphereIndex(x + 1, y + 1, 0, j) + l, getSphereIndex(x, y + 1, 0, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(x, y, scene_config.sphere_precision - 1, j) + l, getSphereIndex(x + 1, y + 1, scene_config.sphere_precision - 1, j) + l, getSphereIndex(x + 1, y, scene_config.sphere_precision - 1, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(x, y, scene_config.sphere_precision - 1, j) + l, getSphereIndex(x, y + 1, scene_config.sphere_precision - 1, j) + l, getSphereIndex(x + 1, y + 1, scene_config.sphere_precision - 1, j) + l);
            }
        }
        for (unsigned z = 0; z < scene_config.sphere_precision - 1; z++)
        {
            for (unsigned x = 0; x < scene_config.sphere_precision - 1; x++)
            {
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(x, 0, z, j) + l, getSphereIndex(x, 0, z + 1, j) + l, getSphereIndex(x + 1, 0, z + 1, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(x, 0, z, j) + l, getSphereIndex(x + 1, 0, z + 1, j) + l, getSphereIndex(x + 1, 0, z, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(x, scene_config.sphere_precision - 1, z, j) + l, getSphereIndex(x + 1, scene_config.sphere_precision - 1, z + 1, j) + l, getSphereIndex(x, scene_config.sphere_precision - 1, z + 1, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(x, scene_config.sphere_precision - 1, z, j) + l, getSphereIndex(x + 1, scene_config.sphere_precision - 1, z, j) + l, getSphereIndex(x + 1, scene_config.sphere_precision - 1, z + 1, j) + l);
            }
        }
        for (unsigned z = 0; z < scene_config.sphere_precision - 1; z++)
        {
            for (unsigned y = 0; y < scene_config.sphere_precision - 1; y++)
            {
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(0, y, z, j) + l, getSphereIndex(0, y + 1, z, j) + l, getSphereIndex(0, y + 1, z + 1, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(0, y, z, j) + l, getSphereIndex(0, y + 1, z + 1, j) + l, getSphereIndex(0, y, z + 1, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(scene_config.sphere_precision - 1, y, z, j) + l, getSphereIndex(scene_config.sphere_precision - 1, y + 1, z + 1, j) + l, getSphereIndex(scene_config.sphere_precision - 1, y + 1, z, j) + l);
                GPU_data.spheres.faces[i++] = Face(getSphereIndex(scene_config.sphere_precision - 1, y, z, j) + l, getSphereIndex(scene_config.sphere_precision - 1, y, z + 1, j) + l, getSphereIndex(scene_config.sphere_precision - 1, y + 1, z + 1, j) + l);
            }
        }
    }

    return 0;
}

int Simulator::loadShaders()
{
    char prelude[500];

    snprintf(prelude, 500,
             "#version 460\n#define NUM_POINTS %lu\n#define NUM_PLANES %lu\n#define NUM_SPHERES %lu\n#define BLOCK_SIZE %u\n",
             GPU_data.jello.position_count,
             sizeof(scene_config.planes) / sizeof(glm::vec4),
             sizeof(scene_config.spheres) / sizeof(glm::vec4),
             scene_config.jello.block_radius * scene_config.jello.block_radius * scene_config.jello.block_radius * 12);

    GLuint render;
    GLuint gravity;
    GLuint springs;
    GLuint integrate;
    GLuint collide;
    GLuint constrain;

    programIDs.render = glCreateProgram();
    programIDs.gravity = glCreateProgram();
    programIDs.springs = glCreateProgram();
    programIDs.integrate = glCreateProgram();
    programIDs.collide = glCreateProgram();
    programIDs.correct = glCreateProgram();

    loadShader(shader_config.vertex.c_str(), GL_VERTEX_SHADER, programIDs.render, prelude);
    loadShader(shader_config.fragment.c_str(), GL_FRAGMENT_SHADER, programIDs.render, prelude);
    loadShader(shader_config.gravity.c_str(), GL_COMPUTE_SHADER, programIDs.gravity, prelude);
    loadShader(shader_config.springs.c_str(), GL_COMPUTE_SHADER, programIDs.springs, prelude);
    loadShader(shader_config.integrate.c_str(), GL_COMPUTE_SHADER, programIDs.integrate, prelude);
    loadShader(shader_config.collide.c_str(), GL_COMPUTE_SHADER, programIDs.collide, prelude);
    loadShader(shader_config.correct.c_str(), GL_COMPUTE_SHADER, programIDs.correct, prelude);

    validateProgram(programIDs.render);
    validateProgram(programIDs.gravity);
    validateProgram(programIDs.springs);
    validateProgram(programIDs.integrate);
    validateProgram(programIDs.collide);
    validateProgram(programIDs.correct);

    glLinkProgram(programIDs.render);
    glLinkProgram(programIDs.gravity);
    glLinkProgram(programIDs.springs);
    glLinkProgram(programIDs.integrate);
    glLinkProgram(programIDs.collide);
    glLinkProgram(programIDs.correct);

    getErrors("Shaders");

    return 0;
}

int Simulator::makeBuffers()
{
    glGenBuffers(10, (GLuint *)&buffers);

    // SSBOs
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.positions);
    if (GPU_data.jello.position_count)
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * GPU_data.jello.position_count, GPU_data.jello.positions, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers.positions);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.last_positions);
    if (GPU_data.jello.position_count)
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * GPU_data.jello.position_count, GPU_data.jello.positions, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffers.last_positions);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.forces);
    if (GPU_data.jello.position_count)
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * GPU_data.jello.position_count, NULL, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers.forces);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.springs);
    if (GPU_data.jello.spring_count)
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Spring) * GPU_data.jello.spring_count, GPU_data.jello.springs, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buffers.springs);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.spheres);
    if (sizeof(scene_config.spheres))
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * sizeof(scene_config.spheres), scene_config.spheres, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buffers.spheres);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.planes);
    if (sizeof(scene_config.planes))
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * sizeof(scene_config.planes), scene_config.planes, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buffers.planes);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // VAOs
    glBindBuffer(GL_ARRAY_BUFFER, buffers.vertices);
    if (GPU_data.jello.vertex_count + GPU_data.planes.vertex_count + GPU_data.spheres.vertex_count)
    {
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * (GPU_data.jello.vertex_count + GPU_data.planes.vertex_count + GPU_data.spheres.vertex_count), NULL, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * GPU_data.jello.vertex_count, sizeof(glm::vec4) * GPU_data.planes.vertex_count, GPU_data.planes.vertices);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * (GPU_data.jello.vertex_count + GPU_data.planes.vertex_count), sizeof(glm::vec4) * GPU_data.spheres.vertex_count, GPU_data.spheres.vertices);
    }
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, buffers.normals);
    if (GPU_data.jello.normal_count + GPU_data.planes.normal_count + GPU_data.spheres.normal_count)
    {
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * (GPU_data.jello.normal_count + GPU_data.planes.normal_count + GPU_data.spheres.normal_count), NULL, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * GPU_data.jello.normal_count, sizeof(glm::vec4) * GPU_data.planes.normal_count, GPU_data.planes.normals);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * (GPU_data.jello.normal_count + GPU_data.planes.normal_count), sizeof(glm::vec4) * GPU_data.spheres.normal_count, GPU_data.spheres.normals);
    }
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, nullptr);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, buffers.colors);
    if (GPU_data.jello.color_count + GPU_data.planes.color_count + GPU_data.spheres.color_count)
    {
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * (GPU_data.jello.color_count + GPU_data.planes.color_count + GPU_data.spheres.color_count), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * GPU_data.jello.color_count, GPU_data.jello.colors);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * GPU_data.jello.color_count, sizeof(glm::vec4) * GPU_data.planes.color_count, GPU_data.planes.colors);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * (GPU_data.jello.color_count + GPU_data.planes.color_count), sizeof(glm::vec4) * GPU_data.spheres.color_count, GPU_data.spheres.colors);
    }
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, nullptr);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.faces);
    if (GPU_data.jello.face_count + GPU_data.planes.face_count + GPU_data.spheres.face_count)
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Face) * (GPU_data.jello.face_count + GPU_data.planes.face_count + GPU_data.spheres.face_count), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(Face) * GPU_data.jello.face_count, GPU_data.jello.faces);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Face) * GPU_data.jello.face_count, sizeof(Face) * GPU_data.planes.face_count, GPU_data.planes.faces);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Face) * (GPU_data.jello.face_count + GPU_data.planes.face_count), sizeof(Face) * GPU_data.spheres.face_count, GPU_data.spheres.faces);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    getErrors("Buffers");

    return 0;
}

inline unsigned Simulator::getPositionIndex(unsigned x, unsigned y, unsigned z) const
{
    return x + y * scene_config.jello.masses_x + z * scene_config.jello.masses_x * scene_config.jello.masses_y;
}

inline unsigned Simulator::getSphereIndex(unsigned x, unsigned y, unsigned z, unsigned i) const
{
    return x + y * scene_config.sphere_precision + z * scene_config.sphere_precision * scene_config.sphere_precision + i * scene_config.sphere_precision * scene_config.sphere_precision * scene_config.sphere_precision;
}