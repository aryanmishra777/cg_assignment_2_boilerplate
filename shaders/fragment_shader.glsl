#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float Depth;

uniform vec3 viewPos;
uniform bool useDepthColor;
uniform float minDepth;
uniform float maxDepth;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

#define NR_LIGHTS 3
uniform Light lights[NR_LIGHTS];

uniform vec3 objectColor;
uniform float shininess;

vec3 getDepthColor(float depth) {
    // Normalize depth to 0-1 range
    float normalizedDepth = clamp((depth - minDepth) / (maxDepth - minDepth), 0.0, 1.0);
    
    // Use 3 contrasting colors: red, green, blue
    // Divide the depth range into 3 segments
    if (normalizedDepth < 0.33) {
        // Transition from blue to green in first third
        float t = normalizedDepth / 0.33;
        return mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0), t);
    } else if (normalizedDepth < 0.67) {
        // Transition from green to red in middle third
        float t = (normalizedDepth - 0.33) / 0.34;
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), t);
    } else {
        // Keep red in last third, potentially making it brighter
        return mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 0.3, 0.0), (normalizedDepth - 0.67) / 0.33);
    }
}

void main() {
    // If depth coloring is enabled
    vec3 finalColor;
    if (useDepthColor) {
        finalColor = getDepthColor(Depth);
    } else {
        finalColor = objectColor;
    }
    
    // Ensure we have a normalized normal
    vec3 norm = normalize(Normal);
    vec3 result = vec3(0.0);
    
    for(int i = 0; i < NR_LIGHTS; i++) {
        if (!lights[i].enabled)
            continue;
            
        // Ambient
        vec3 ambient = lights[i].ambient * finalColor;
        
        // Diffuse
        vec3 lightDir = normalize(lights[i].position - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = lights[i].diffuse * diff * finalColor;
        
        // Specular (Blinn-Phong)
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
        vec3 specular = lights[i].specular * spec;
        
        result += ambient + diffuse + specular;
    }
    
    FragColor = vec4(result, 1.0);
}