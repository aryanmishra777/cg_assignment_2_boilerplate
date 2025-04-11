#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aFaceCenter;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float explodeFactor;

out vec3 FragPos;
out vec3 Normal;
out float Depth;

void main() {
    // Apply explode effect
    vec3 direction = aPos - aFaceCenter;
    vec3 explodedPos = aPos + direction * explodeFactor;
    
    FragPos = vec3(model * vec4(explodedPos, 1.0));
    
    // Normal matrix calculation - correctly transforms normals
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
    
    // Calculate depth for coloring
    vec4 viewPosition = view * vec4(FragPos, 1.0);
    Depth = -viewPosition.z; // Negate because view space z is negative toward the screen
}