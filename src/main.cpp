#include "../glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui includes
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"

#include <iostream>
#include <string>
#include <vector>

#include "shader.h"
#include "camera.h"
#include "mesh.h"
#include "explosion_effect.h" // Add this include

// Window settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Lighting
std::vector<Light> lights;
bool depthColoring = false;
float explodeFactor = 0.0f;
bool explodeAnimation = false;
float explodeDirection = 1.0f; // 1.0f for expanding, -1.0f for contracting

// OFF model for explosion effect
OffModel* offModel = nullptr;

// Rotation settings
float rotationAngle = 0.0f;
glm::vec3 rotationAxis(1.0f, 0.0f, 0.0f); // Default to X axis
bool autoRotate = true;
bool customRotationAxis = false;
float customAxisParams[3] = {1.0f, 1.0f, 1.0f}; // For arbitrary rotation axis
float rotationSpeed = 30.0f; // Degrees per second

// ImGui control
bool showImGuiWindow = true;
bool captureMouse = true;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(int argc, char* argv[]) {
    // Check if mesh file is provided
    std::string meshFilename;
    if (argc > 1) {
        meshFilename = argv[1];
    } else {
        meshFilename = "models/1grm.off"; // Default mesh if none provided
        std::cout << "No mesh file provided. Using default: " << meshFilename << std::endl;
        std::cout << "Usage: " << argv[0] << " <mesh_file.off>" << std::endl;
    }

    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Mesh Viewer", NULL, NULL);
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

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Build and compile shaders
    Shader shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    // Load the mesh from OFF file
    std::cout << "Loading mesh: " << meshFilename << std::endl;
    Mesh mesh(meshFilename);
    mesh.setupMesh();
    std::cout << "Mesh loaded with " << mesh.vertices.size() << " vertices and " 
              << mesh.indices.size() / 3 << " triangles" << std::endl;

    // Get access to the underlying OFF model for explosion effects
    offModel = mesh.getOffModel();
    if (offModel) {
        // Initialize explosion data
        initializeExplosion(offModel);
    }

    // Setup lights
    lights.push_back(Light(
        glm::vec3(1.2f, 1.0f, 2.0f),
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        true
    ));
    
    lights.push_back(Light(
        glm::vec3(-1.2f, 1.0f, 0.0f),
        glm::vec3(0.1f, 0.1f, 0.1f),
        glm::vec3(0.25f, 0.25f, 0.25f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        false
    ));
    
    lights.push_back(Light(
        glm::vec3(0.0f, -1.0f, -1.0f),
        glm::vec3(0.05f, 0.05f, 0.05f),
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::vec3(0.7f, 0.7f, 0.7f),
        false
    ));
    
    // Print controls
    std::cout << "\n=== Controls ===\n";
    std::cout << "WASD: Move camera\n";
    std::cout << "QE: Move camera up/down\n";
    std::cout << "Mouse: Look around\n";
    std::cout << "B: Toggle explode animation\n";
    std::cout << "R: Toggle auto-rotation\n";
    std::cout << "Space: Change rotation axis\n";
    std::cout << "Tab: Toggle ImGui window/mouse capture\n";
    std::cout << "ESC: Exit\n";

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);

        // Update rotation angle if auto-rotate is enabled
        if (autoRotate) {
            rotationAngle += rotationSpeed * deltaTime; // Use rotationSpeed instead of hardcoded 30.0f
            if (rotationAngle > 360.0f) rotationAngle -= 360.0f;
        }
        
        // Update explosion effect if animation is active
        if (explodeAnimation) {
            explodeFactor += explodeDirection * deltaTime;
            if (explodeFactor > 1.0f) {
                explodeFactor = 1.0f;
                explodeDirection = -1.0f;
            }
            else if (explodeFactor < 0.0f) {
                explodeFactor = 0.0f;
                explodeDirection = 1.0f;
                explodeAnimation = false;
            }
            
            // Apply explosion effect to the model
            if (offModel) {
                updateExplosion(offModel, explodeFactor);
            }
        }

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui panel for light and rotation controls
        if (showImGuiWindow) {
            ImGui::Begin("Controls");
            
            // General settings
            if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Depth-based Coloring", &depthColoring);
                if (ImGui::Button("Explode View")) {
                    explodeAnimation = true;
                    explodeDirection = explodeFactor > 0.5f ? -1.0f : 1.0f;
                }
                ImGui::SameLine();
                if (ImGui::SliderFloat("Explode Factor", &explodeFactor, 0.0f, 1.0f)) {
                    // Apply explosion effect when slider is moved
                    if (offModel) {
                        updateExplosion(offModel, explodeFactor);
                    }
                }
            }
            
            // Rotation settings
            if (ImGui::CollapsingHeader("Rotation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Auto Rotate", &autoRotate);
                
                // Radio buttons for standard axes
                static int rotationAxisOption = 0; // 0=X, 1=Y, 2=Z, 3=Custom
                bool axisChanged = false;
                
                axisChanged |= ImGui::RadioButton("X Axis", &rotationAxisOption, 0);
                ImGui::SameLine();
                axisChanged |= ImGui::RadioButton("Y Axis", &rotationAxisOption, 1);
                ImGui::SameLine();
                axisChanged |= ImGui::RadioButton("Z Axis", &rotationAxisOption, 2);
                axisChanged |= ImGui::RadioButton("Custom Axis", &rotationAxisOption, 3);
                
                if (rotationAxisOption == 3) {
                    // Custom axis controls
                    ImGui::InputFloat3("Custom Axis Vector", customAxisParams);
                    if (ImGui::Button("Apply Custom Axis")) {
                        glm::vec3 newAxis(customAxisParams[0], customAxisParams[1], customAxisParams[2]);
                        if (glm::length(newAxis) > 0.001f) { // Ensure non-zero vector
                            rotationAxis = glm::normalize(newAxis);
                        }
                    }
                } else if (axisChanged) {
                    // Set rotation axis based on selected standard axis
                    switch (rotationAxisOption) {
                        case 0: rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f); break; // X axis
                        case 1: rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); break; // Y axis
                        case 2: rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f); break; // Z axis
                    }
                }
                
                // Display current rotation axis
                ImGui::Text("Current Axis: (%.2f, %.2f, %.2f)", 
                    rotationAxis.x, rotationAxis.y, rotationAxis.z);
                ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 100.0f);
            }
            
            ImGui::Separator();
            
            // Light controls
            for (int i = 0; i < lights.size(); i++) {
                std::string lightLabel = "Light " + std::to_string(i + 1);
                if (ImGui::CollapsingHeader(lightLabel.c_str())) {
                    std::string enableId = "Enabled##" + std::to_string(i);
                    ImGui::Checkbox(enableId.c_str(), &lights[i].enabled);
                    
                    std::string posId = "Position##" + std::to_string(i);
                    ImGui::DragFloat3(posId.c_str(), &lights[i].position.x, 0.1f);
                    
                    std::string ambId = "Ambient##" + std::to_string(i);
                    ImGui::ColorEdit3(ambId.c_str(), &lights[i].ambient.x);
                    
                    std::string diffId = "Diffuse##" + std::to_string(i);
                    ImGui::ColorEdit3(diffId.c_str(), &lights[i].diffuse.x);
                    
                    std::string specId = "Specular##" + std::to_string(i);
                    ImGui::ColorEdit3(specId.c_str(), &lights[i].specular.x);
                }
            }
            
            ImGui::End();
        }

        // Activate shader
        shader.use();

        // Pass object color to the shader
        shader.setVec3("objectColor", glm::vec3(0.8f, 0.8f, 0.8f));
        shader.setFloat("shininess", 32.0f);
        shader.setVec3("viewPos", camera.Position);
        shader.setBool("useDepthColor", depthColoring);
        shader.setFloat("minDepth", 0.1f);
        shader.setFloat("maxDepth", 10.0f);
        
        // Pass light properties
        for (unsigned int i = 0; i < lights.size(); i++) {
            std::string lightIndex = "lights[" + std::to_string(i) + "]";
            shader.setVec3(lightIndex + ".position", lights[i].position);
            shader.setVec3(lightIndex + ".ambient", lights[i].ambient);
            shader.setVec3(lightIndex + ".diffuse", lights[i].diffuse);
            shader.setVec3(lightIndex + ".specular", lights[i].specular);
            shader.setBool(lightIndex + ".enabled", lights[i].enabled);
        }
        
        // Pass explode factor
        shader.setFloat("explodeFactor", explodeFactor);

        // View/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // Model transformation
        glm::mat4 model = mesh.getModelMatrix(rotationAngle, rotationAxis);
        shader.setMat4("model", model);

        // Render the mesh
        mesh.Draw(shader);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Reset explosion and clean up explosion data
    if (offModel) {
        resetExplosion(offModel);
        cleanupExplosionData(offModel);
    }
    cleanupAllExplosionData();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwTerminate();
    return 0;
}

// Process all input
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Only process camera movement if mouse is captured
    if (captureMouse) {
        // Camera movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime);
    }
}

// Process key presses
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_B:
                explodeAnimation = true;
                explodeDirection = explodeFactor > 0.5f ? -1.0f : 1.0f;
                std::cout << "Explode animation: " << (explodeAnimation ? "ON" : "OFF") << std::endl;
                break;
            case GLFW_KEY_R:
                autoRotate = !autoRotate;
                std::cout << "Auto-rotation: " << (autoRotate ? "ON" : "OFF") << std::endl;
                break;
            case GLFW_KEY_SPACE:
                // Change rotation axis
                if (rotationAxis == glm::vec3(0.0f, 1.0f, 0.0f))
                    rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
                else if (rotationAxis == glm::vec3(1.0f, 0.0f, 0.0f))
                    rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
                else
                    rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
                std::cout << "Rotation axis: " << rotationAxis.x << ", " 
                          << rotationAxis.y << ", " << rotationAxis.z << std::endl;
                break;
            case GLFW_KEY_TAB:
                // Toggle ImGui window and mouse capture
                showImGuiWindow = !showImGuiWindow;
                captureMouse = !showImGuiWindow;
                
                if (captureMouse) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
                std::cout << "ImGui window: " << (showImGuiWindow ? "SHOWN" : "HIDDEN") << std::endl;
                std::cout << "Mouse capture: " << (captureMouse ? "ON" : "OFF") << std::endl;
                break;
            case GLFW_KEY_N: // Add a key for resetting explosion
                if (offModel) {
                    resetExplosion(offModel);
                    explodeFactor = 0.0f;
                }
                std::cout << "Explosion reset" << std::endl;
                break;
        }
    }
}

// Callback for window resize
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Callback for mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Skip if ImGui is using the mouse
    if (!captureMouse)
        return;
        
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top

    lastX = xpos;
    lastY = ypos;

    // Adjust sensitivity - you can tune this value
    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Update camera angles
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Callback for scroll wheel
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (captureMouse)
        camera.ProcessMouseScroll(yoffset);
}