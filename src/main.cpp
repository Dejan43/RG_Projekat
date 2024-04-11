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

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(char const * path);
void renderQuad();
void renderCube();
// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 1.0f;
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
    vector<bool> used = {false,false,false,false,false,false,false,false};
    vector<float> rot = {0,0,0,0,0,0,0,0};
    int last = 0;
    float timer = 0;
    bool cleared = true;
    bool move = false;
    int now;
    bool reset = true;
};
GameState gameState;


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
    //stbi_set_flip_vertically_on_load(true);

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
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader shader("resources/shaders/bloom.vs", "resources/shaders/bloom.fs");
    Shader shaderLight("resources/shaders/bloom.vs", "resources/shaders/light_box.fs");
    Shader shaderBlur("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader hdrShader("resources/shaders/hdr.vs", "resources/shaders/hdr.fs");


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

    // skybox
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // making card objects
    float vertices[] = {
            -0.05f, 0.5f, 0.25f, 1.0f, 0.0f,
            -0.05f, 0.5f, -0.25f, 1.0f, 1.0f,
            -0.05f, -0.5f, -0.25f, 0.0f, 1.0f,
            -0.05f, -0.5f, -0.25f, 0.0f, 1.0f,
            -0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, 0.5f, 0.25f, 1.0f, 0.0f,

            -0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, -0.25f, 0.0f, 0.0f,
            -0.05f, 0.5f, -0.25f, 0.0f, 0.0f,
            -0.05f, -0.5f, -0.25f, 0.0f, 0.0f,

            -0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, 0.5f, 0.25f, 0.0f, 0.0f,

            0.05f, 0.5f, 0.25f, 1.0f, 0.0f,
            0.05f, 0.5f, -0.25f, 1.0f, 1.0f,
            0.05f, -0.5f, -0.25f, 0.0f, 1.0f,
            0.05f, -0.5f, -0.25f, 0.0f, 1.0f,
            0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, 0.5f, 0.25f, 1.0f, 0.0f,

            -0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            0.05f, -0.5f, 0.25f, 0.0f, 0.0f,
            -0.05f, -0.5f, -0.25f, 0.0f, 0.0f,
            -0.05f, -0.5f, 0.25f, 0.0f, 0.0f,

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
    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    stbi_set_flip_vertically_on_load(false);
    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/back.jpg")
            };
    unsigned int cubemapTexture = loadCubemap(faces);
    stbi_set_flip_vertically_on_load(true);
    //cards VAO
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
    // making victory transparent box
    float victoryvertices[] = {


            -0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
            0.5f, 0.0f, -1.0f, 0.0f, 1.0f,
            0.5f, 0.0f, -1.0f, 0.0f, 1.0f,
            -0.5f, 0.0f, -1.0f, 0.0f, 0.0f,
            -0.5f, 0.0f, 1.0f, 1.0f, 0.0f


    };
    // victory transparent box VAO
    unsigned int VBO1, VAO1;

    glGenVertexArrays(1, &VAO1);
    glGenBuffers(1, &VBO1);

    glBindVertexArray(VAO1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(victoryvertices), victoryvertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    //HDR
    // configure (floating point) framebuffers
    // ---------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    // load textures
    // -------------
    unsigned int texture1 = loadTexture(FileSystem::getPath("resources/textures/card.jpg").c_str());
    unsigned int texture2 = loadTexture(FileSystem::getPath("resources/textures/python.png").c_str());
    unsigned int texture3 = loadTexture(FileSystem::getPath("resources/textures/c++.png").c_str());
    // load and create a texture
    // -------------------------
    unsigned int texture4 = loadTexture(FileSystem::getPath("resources/textures/haskell.png").c_str());;
    unsigned int texture5 = loadTexture(FileSystem::getPath("resources/textures/java.png").c_str());;
    unsigned int texture6 = loadTexture(FileSystem::getPath("resources/textures/victory.png").c_str());;



    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    blendShader.use();
    blendShader.setInt("texture1", 0);
    blendShader.setInt("texture2", 1);
    blendShader.setInt("texture3", 2);
    blendShader.setInt("texture4", 3);
    blendShader.setInt("texture5", 4);
    blendingShader.use();
    blendingShader.setInt("texture6", 5);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    //initialize point light
    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(1.75,16.55,7.3);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.f;
    pointLight.linear = 0.045f;
    pointLight.quadratic = 0.012f;

    DirLight& dirLight = programState->dirLight;
    // lighting info
    // -------------
    // positions
    std::vector<glm::vec3> lightPositions;
    lightPositions.push_back(glm::vec3(3.66,6.9,7.f));
    lightPositions.push_back(glm::vec3(1.75,16.55,7.3));

    // colors
    std::vector<glm::vec3> lightColors;
    lightColors.push_back(glm::vec3(11.0f,   5.0f,  1.0f));
    lightColors.push_back(glm::vec3(8.0f,   8.0f,  4.0f));

    std::vector<glm::vec3> lightScales;
    lightScales.push_back(glm::vec3(0.18f));
    lightScales.push_back(glm::vec3(1.02f,0.15,1.55f));


    // shader configuration
    // --------------------
    ourShader.use();
    ourShader.setInt("diffuseTexture", 0);
    shaderBlur.use();
    shaderBlur.setInt("image", 0);
    hdrShader.use();
    hdrShader.setInt("scene", 0);
    hdrShader.setInt("bloomBlur", 1);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // used for game logic
    vector<int> newOrder(8, -1);
    glm::vec3 cubePosition2[8];

    // render loop
    // -----------
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

        // 1. render scene into floating point framebuffer
        // -----------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        dirLight.ambient = glm::vec3(0.02, 0.02, 0.02);
        dirLight.diffuse = glm::vec3(0.25, 0.25, 0.25);
        dirLight.specular = glm::vec3(0.20, 0.20, 0.20);
        dirLight.direction = glm::vec3(-0.1,-0.5, 1.0);

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.color",normalize(lightColors[1]));
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
        ourShader.setVec3("spotLight.position", glm::vec3(3.66,6.9,7.f));
        ourShader.setVec3("spotLight.direction", glm::vec3(-0.2,-1,-0.01));
        ourShader.setVec3("spotLight.color",normalize(lightColors[0]));
        ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant", 1.f);
        ourShader.setFloat("spotLight.linear", 0.09);
        ourShader.setFloat("spotLight.quadratic", 0.039);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(28.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(36.0f)));
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
        model = glm::translate(model,glm::vec3(4.6,3.54,7.f));
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
        model = glm::scale(model, glm::vec3(0.4f,0.4f,0.4f));
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
        if(gameState.reset) {
            srand(time(0));
            for (int i = 0; i < 8; i++) {
                newOrder[i] = -1;
            }
            for (int i = 0; i < 8; i++) {
                int x = rand() % 8;
                while (std::find(newOrder.begin(), newOrder.end(), x) != newOrder.end()) {
                    x = rand() % 8;
                }
                newOrder[i] = x;
            }
            for (int i = 0; i < 8; i++) {
                cubePosition2[i] = cubePositions[newOrder[i]];
                gameState.rot[i] = 0;
                gameState.used[i] = false;
                gameState.number = 0;
                gameState.pickedCount = 0;
                gameState.card = -1;
            }
            gameState.reset = false;
        }
        auto it = find(newOrder.begin(), newOrder.end(),gameState.card);
        if(gameState.cleared){
            gameState.now = it - newOrder.begin();
            if(gameState.card != -1 && !gameState.used[gameState.now ] && gameState.pickedCount < 4) {

                gameState.number++;

                gameState.used[gameState.now ] = true;
                gameState.rot[gameState.now ] = 180.f;
                if (gameState.number == 1) {
                    gameState.last = gameState.now;
                }
                if (gameState.number == 2) {
                    gameState.number = 0;
                     if (gameState.now  / 2 == gameState.last / 2) {
                         gameState.move = true;
                     }
                     else{
                         gameState.used[gameState.last] = false;
                         gameState.used[gameState.now ] = false;
                     }
                     gameState.timer = glfwGetTime();
                     gameState.cleared = false;
                     gameState.card = -1;
                }
            }
        }
        if(glfwGetTime() - gameState.timer > 1 && !gameState.cleared) {

            if(gameState.move){
                cubePosition2[gameState.now] = cardPicked[gameState.pickedCount  * 2];
                cubePosition2[gameState.last] = cardPicked[gameState.pickedCount*2 + 1];
                gameState.pickedCount += 1;
                gameState.move = false;
            } else {
                gameState.rot[gameState.last] = 0;
                gameState.rot[gameState.now] = 0;
            }
            gameState.cleared = true;
        }
        // --------------------------------------------------
        // render cards
        glEnable(GL_CULL_FACE);
        glBindVertexArray(VAO);
        int pair = 0;
        bool drawVictory = true;
        for (unsigned int i = 0; i < 8; i++){
            glCullFace(GL_FRONT);
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
            model = glm::rotate(model, glm::radians(gameState.rot[i]), glm::vec3(0.0f, 1.0f, 0.0f));
            if(gameState.rot[i])
                glCullFace(GL_BACK);

            model = glm::scale(model, glm::vec3(0.7f,0.7f,0.7f));

            blendShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            glCullFace(GL_FRONT);

            side = true;
            blendShader.setBool("side", side);
            glDrawArrays(GL_TRIANGLES, 6, 36);

            if(!gameState.used[i]){
                drawVictory = false;
            }
        }
        glDisable(GL_CULL_FACE);

        if(drawVictory) {
            blendingShader.use();
            blendingShader.setMat4("projection", projection);
            blendingShader.setMat4("view", view);
            glBindVertexArray(VAO1);
            glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
            model = glm::translate(model, glm::vec3(2.15,3.74,6.6));

            model = glm::scale(model, glm::vec3(1.5f,1.5f,1.5f));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        // finally show all the light sources as bright cubes
        shaderLight.use();
        shaderLight.setMat4("projection", projection);
        shaderLight.setMat4("view", view);
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(lightPositions[i]));
            model = glm::rotate(model, (float)glm::radians(-10.f),glm::vec3(0,1,0));
            model = glm::scale(model, glm::vec3(lightScales[i]));
            shaderLight.setMat4("model", model);
            shaderLight.setVec3("lightColor", lightColors[i]);
            renderCube();
        }

        // Draw skybox
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


        // 2. blur bright fragments with two-pass Gaussian Blur
        // --------------------------------------------------
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        shaderBlur.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            shaderBlur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        hdrShader.setInt("bloom", bloom);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();

        std::cout << "bloom: " << (bloom ? "on" : "off") << "| exposure: " << exposure << std::endl;



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
    glDeleteVertexArrays(1, &skyboxVAO);

    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &skyboxVBO);

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
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(TABLE_VIEW, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.01f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        exposure += 0.01f;
    }
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

    if (key == GLFW_KEY_R && action == GLFW_PRESS) gameState.reset = true;
    // --------------------------------------------------
}
//utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
