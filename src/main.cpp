#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#define RAND_MAX 7
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

};
struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    DirLight dirLight;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

// --------------------------------------------------
// USED FOR MINI GAME
struct GameState {
    int card = -1;
    int number = 0;
    int pickedCount = 0;
};
GameState gameState;

vector<bool> used(8, false);
vector<float> rot(8, 0.0);
int last = 0;
float timer = 0;
bool cleared = true;

// --------------------------------------------------
ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader blendShader("resources/shaders/blend.vs", "resources/shaders/blend.fs");

    // load models
    // -----------
    Model Dog("resources/objects/Dog/scene.gltf");
    Dog.SetShaderTextureNamePrefix("material.");
    Model Tree("resources/objects/Tree/scene.gltf");
    Tree.SetShaderTextureNamePrefix("material.");
    Model Table("resources/objects/Table/round table Ultimate(free Final).obj");
    Table.SetShaderTextureNamePrefix("material.");
    Model Chair("resources/objects/Chair/Rocking_chair_SF.obj");
    Chair.SetShaderTextureNamePrefix("material.");
    Model Lamp("resources/objects/Lamp/StreetLamp.obj");
    Lamp.SetShaderTextureNamePrefix("material.");
    Model Moon("resources/objects/Moon/Moon.obj");
    Moon.SetShaderTextureNamePrefix("material.");
    Model DeskLamp("resources/objects/DeskLamp/scene.gltf");
    DeskLamp.SetShaderTextureNamePrefix("material.");
    // making card objects
    float vertices[] = {
            -0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, -0.25f, 0.0f, 0.0f,
            -0.05f, 0.5f, -0.25f, 0.0f, 0.0f,
            -0.05f, -0.5f, -0.25f, 0.0f, 0.0f,

            -0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, 0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, -0.5f, 0.25f, 0.0f, 0.0f,

            -0.05f, 0.5f, 0.25f, 1.0f, 0.0f,
            -0.05f, 0.5f, -0.25f, 1.0f, 1.0f,
            -0.05f, -0.5f, -0.25f, 0.0f, 1.0f,
            -0.05f, -0.5f, -0.25f, 0.0f, 1.0f,
            -0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, 0.5f, 0.25f, 1.0f, 0.0f,

            0.05f, 0.5f, 0.25f, 1.0f, 0.0f,
            0.05f, 0.5f, -0.25f, 1.0f, 1.0f,
            0.05f, -0.5f, -0.25f, 0.0f, 1.0f,
            0.05f, -0.5f, -0.25f, 0.0f, 1.0f,
            0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, 0.25f, 1.0f, 0.0f,

            -0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, -0.5f, -0.25f, 0.0f, 0.0f,

            -0.05f, 0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, 0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, 0.5f, -0.25f, 0.0f, 0.0f
    };
    // world space positions of our cubes
    glm::vec3 cubePositions[] = {
            glm::vec3(2.8f, 3.6f, 5.4f),
            glm::vec3(2.8f, 3.6f, 6.2f),
            glm::vec3(2.8f, 3.6f, 7.0f),
            glm::vec3(2.8f, 3.6f, 7.8),
            glm::vec3(1.5f, 3.6f, 5.4),
            glm::vec3(1.5f, 3.6f, 6.2),
            glm::vec3(1.50f, 3.6f, 7.0),
            glm::vec3(1.5f, 3.6f, 7.8),
    };
    // --------------------------------------------------
    // USED FOR MINI GAME
    glm::vec3 cardPicked[] = {
            glm::vec3( 3.5f,  3.6f,  8.8f),
            glm::vec3( 3.5,  3.71f, 8.8f),
            glm::vec3(3.5, 3.82f, 8.8f),
            glm::vec3(3.5, 3.93f, 8.8f),
            glm::vec3( 3.5f, 4.04f, 8.8f),
            glm::vec3(3.5f,  4.15f, 8.8f),
            glm::vec3( 3.5f, 4.26f, 8.8f),
            glm::vec3( 3.5f,  4.37f, 8.8f),
    };
    //----------------------------------------------------

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // load and create a texture
    // -------------------------
    unsigned int texture1, texture2, texture3, texture4, texture5, texture6;
    // texture 1
    // ---------
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load(FileSystem::getPath("resources/textures/container.jpg").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    // texture 2
    // ---------
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    data = stbi_load(FileSystem::getPath("resources/textures/awesomeface.png").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    // texture 3
    // ---------
    glGenTextures(1, &texture3);
    glBindTexture(GL_TEXTURE_2D, texture3);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    data = stbi_load(FileSystem::getPath("resources/textures/c++.png").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
// texture 4
    // ---------
    glGenTextures(1, &texture4);
    glBindTexture(GL_TEXTURE_2D, texture4);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    data = stbi_load(FileSystem::getPath("resources/textures/haskell.png").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
// texture 5
    // ---------
    glGenTextures(1, &texture5);
    glBindTexture(GL_TEXTURE_2D, texture5);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    data = stbi_load(FileSystem::getPath("resources/textures/java.png").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
// texture 6
    // ---------
    glGenTextures(1, &texture6);
    glBindTexture(GL_TEXTURE_2D, texture6);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    data = stbi_load(FileSystem::getPath("resources/textures/python.png").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    blendShader.use();
    blendShader.setInt("texture1", 0);
    blendShader.setInt("texture2", 1);
    blendShader.setInt("texture3", 2);
    blendShader.setInt("texture4", 3);
    blendShader.setInt("texture5", 4);
    blendShader.setInt("texture6", 5);

    //initialize point light
    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(0.0,9,13.0);
    pointLight.ambient = glm::vec3(0.11, 0.11, 0.11);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.4f;
    pointLight.linear = 0.009f;
    pointLight.quadratic = 0.0032f;

     DirLight& dirLight = programState->dirLight;

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // render loop
    // -----------
    srand(time(0));
    vector<int> newOrder(8,-1);
    for(int i = 0; i < 8; i++){
        int x = rand() % 8;
        while(std::find(newOrder.begin(), newOrder.end(), x) != newOrder.end()){
            x = rand()%8;
        }
        newOrder[i] = x;
    }
    glm::vec3 cubePosition2[8];
    for(int i = 0; i < 8;i++){
        cubePosition2[i] = cubePositions[newOrder[i]];
    }
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);
        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // don't forget to enable shader before setting uniforms
        ourShader.use();
        dirLight.ambient = glm::vec3(0.1, 0.1, 0.1);
        dirLight.diffuse = glm::vec3(0.7, 0.7, 0.7);
        dirLight.specular = glm::vec3(1.0, 1.0, 1.0);
        dirLight.direction = glm::vec3(-0.2,-0.5, 0.0);

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("dirLight.position", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // spotLight
        ourShader.setVec3("spotLight.position", glm::vec3(3.66,7.8,7.4));
        ourShader.setVec3("spotLight.direction", glm::vec3(-0.2,-1,-0.01));
        ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.09);
        ourShader.setFloat("spotLight.quadratic", 0.032);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(25.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(30.0f)));
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);

        //Dog

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(9.0,0.0,0.0));
        model = glm::rotate(model, (float)glm::radians(-45.f),glm::vec3(0.0,1,0.0));
        model = glm::scale(model, glm::vec3(0.5f,0.5f,0.5f));
        ourShader.setMat4("model", model);
        Dog.Draw(ourShader);

        //Tree

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0,0.0,0.0));
        model = glm::rotate(model, (float)glm::radians(-90.f),glm::vec3(1.0,0,0.0));
        model = glm::scale(model, glm::vec3(0.25f,0.25f,0.25f));
        ourShader.setMat4("model", model);
        Tree.Draw(ourShader);

        //Table

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(3.0,0.0,7.0));
        model = glm::scale(model, glm::vec3(2.5f,2.5f,2.5f));
        ourShader.setMat4("model", model);
        Table.Draw(ourShader);

        //Chair

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-3.0,0.0,7.0));
        model = glm::rotate(model, (float)glm::radians(sin((float)glfwGetTime())* 15),glm::vec3(0.0,0,1.0));
        model = glm::rotate(model, (float)glm::radians(90.f),glm::vec3(0.0,1,0.0));
        model = glm::scale(model, glm::vec3(0.8f,0.8f,0.8f));
        ourShader.setMat4("model", model);
        Chair.Draw(ourShader);

        //Lamp

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0,-1,17.0));
        model = glm::rotate(model, (float)glm::radians(80.f),glm::vec3(0.0,1,0.0));
        model = glm::scale(model, glm::vec3(1.35f,1.35f,1.35f));
        ourShader.setMat4("model", model);
        Lamp.Draw(ourShader);

        //Desk Lamp

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(4.6,3.54,7.4));
        model = glm::rotate(model, (float)glm::radians(-90.f),glm::vec3(1.0,0.0,0.0));
        model = glm::rotate(model, (float)glm::radians(180.f),glm::vec3(0.0,0.0,1.0));
        model = glm::scale(model, glm::vec3(0.081f,0.081f,0.081f));
        ourShader.setMat4("model", model);
        DeskLamp.Draw(ourShader);

        //Moon

        dirLight.ambient = glm::vec3(1, 1, 1);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setFloat("material.shininess", 512.0f);

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(10.0,20.0,-40.0));
        model = glm::scale(model, glm::vec3(1.f,1.f,1.f));
        ourShader.setMat4("model", model);
        Moon.Draw(ourShader);



        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texture3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, texture4);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, texture5);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, texture6);
        blendShader.use();
        blendShader.setMat4("projection", projection);
        blendShader.setMat4("view", view);

        // --------------------------------------------------
        // USED FOR MINI GAME
        //
        auto it = find(newOrder.begin(), newOrder.end(),gameState.card);
        int i = it - newOrder.begin();
        if(gameState.card != -1 && !used[i] && gameState.pickedCount < 4 && cleared) {

            gameState.number++;

            used[i] = true;
            rot[i] = 180.f;
            if (gameState.number == 1) {
                last = i;
            }
             if (gameState.number == 2) {
                 gameState.number = 0;
                 if (i / 2 == last / 2) {
                     cubePosition2[i] = cardPicked[gameState.pickedCount  * 2];
                     cubePosition2[last] = cardPicked[gameState.pickedCount*2 + 1];
                     gameState.pickedCount += 1;
                 }
                 else{
                     timer = glfwGetTime();
                     cleared = false;
                     used[last] = false;
                     used[i] = false;
                     gameState.card = -1;
                 }
             }
        }
        if(glfwGetTime() - timer > 1 && !cleared) {
            for (int i = 0; i < 8; i++) {
                rot[i] = 0;
            }
            cleared = true;
        }
        // --------------------------------------------------

        // render cards
        glBindVertexArray(VAO);
        int pair = 0;
        for (unsigned int i = 0; i < 8; i++){
            bool side = false;
            if (i%2 == 0)
                pair++;
            blendShader.setBool("side", side);
            blendShader.setInt("pair", pair);

            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
            model = glm::translate(model, cubePosition2[i]);
            float angle = 90.0f;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rot[i]), glm::vec3(0.0f, 1.0f, 0.0f));

            model = glm::scale(model, glm::vec3(0.7f,0.7f,0.7f));

            blendShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 18);

            side = true;
            blendShader.setBool("side", side);
            glDrawArrays(GL_TRIANGLES, 18, 36);
        }
        
        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    //Movment
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        deltaTime *=5;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(R_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(R_RIGHT, deltaTime);

}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    // --------------------------------------------------
    // USED FOR MINI GAME
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) gameState.card = 0;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) gameState.card = 1;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) gameState.card = 2;
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) gameState.card = 3;
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) gameState.card = 4;
    if (key == GLFW_KEY_6 && action == GLFW_PRESS) gameState.card = 5;
    if (key == GLFW_KEY_7 && action == GLFW_PRESS) gameState.card = 6;
    if (key == GLFW_KEY_8 && action == GLFW_PRESS) gameState.card = 7;
    // --------------------------------------------------
}
