#ifndef MESH_H
#define MESH_H

#include "../glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>
#include "shader.h"
#include "OFFReader.h"

struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 faceCenter; // For explode effect
};

class Mesh {
public:
    // Mesh data
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 centerOfMass;
    float boundingSphereRadius;
    OffModel* offModel;

    // Constructor - loads mesh from OFF file
    Mesh(const std::string& filename) {
        // Use OFFReader to load the mesh
        offModel = readOffFile(const_cast<char*>(filename.c_str()));
        if (!offModel) {
            throw std::runtime_error("Failed to load OFF file: " + filename);
        }
        
        // Process vertices
        for (int i = 0; i < offModel->numberOfVertices; i++) {
            MeshVertex vertex;
            vertex.position = glm::vec3(
                offModel->vertices[i].x,
                offModel->vertices[i].y,
                offModel->vertices[i].z
            );
            vertex.normal = glm::vec3(0.0f); // Will calculate later
            vertices.push_back(vertex);
        }
        
        // Process faces and create indices
        for (int i = 0; i < offModel->numberOfPolygons; i++) {
            Polygon& polygon = offModel->polygons[i];
            
            // Triangulate polygons (assuming convex polygons)
            for (int j = 1; j < polygon.noSides - 1; j++) {
                indices.push_back(polygon.v[0]);
                indices.push_back(polygon.v[j]);
                indices.push_back(polygon.v[j + 1]);
            }
        }
        
        calculateNormals();
        calculateFaceCenters();
        calculateCenterAndRadius();
    }
    
    // Destructor - cleanup
    ~Mesh() {
        if (offModel) {
            FreeOffModel(offModel);
        }
        
        // Clean up OpenGL resources
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    // Sets up the mesh data in the buffers
    void setupMesh() {
        // Create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Bind buffers
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Fill buffer with vertex data
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MeshVertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Vertex position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)0);
        
        // Vertex normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, normal));
        
        // Face center attribute (for explosion effect)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, faceCenter));

        // Unbind
        glBindVertexArray(0);
    }
    
    // Renders the mesh
    void Draw(Shader &shader) {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    // Get model matrix that centers and scales the mesh to fit view
    glm::mat4 getModelMatrix(float rotationAngle, glm::vec3 rotationAxis) {
        glm::mat4 model = glm::mat4(1.0f);
        
        // Center the mesh
        model = glm::translate(model, -centerOfMass);
        
        // Scale to fit viewing area
        float scaleFactor = 1.0f / boundingSphereRadius;
        model = glm::scale(model, glm::vec3(scaleFactor));
        
        // Apply rotation
        model = glm::rotate(model, glm::radians(rotationAngle), rotationAxis);
        
        return model;
    }
    // Add this method to your Mesh class definition
    void updateBuffers() {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
        // Update only the position data in the buffer
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_DYNAMIC_DRAW);
    
        glBindVertexArray(0);
    }

    // Get underlying OffModel for explosion effects
    OffModel* getOffModel() const {
        return offModel;
    }

private:
    // Render data
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    // Calculate vertex normals
    void calculateNormals() {
        // Initialize all normals to zero
        for (auto& vertex : vertices) {
            vertex.normal = glm::vec3(0.0f);
        }
        
        // For each face (triangle), calculate normal and add to connected vertices
        for (size_t i = 0; i < indices.size(); i += 3) {
            unsigned int idx1 = indices[i];
            unsigned int idx2 = indices[i+1];
            unsigned int idx3 = indices[i+2];
            
            glm::vec3 v1 = vertices[idx1].position;
            glm::vec3 v2 = vertices[idx2].position;
            glm::vec3 v3 = vertices[idx3].position;
            
            // Calculate face normal using cross product
            glm::vec3 edge1 = v2 - v1;
            glm::vec3 edge2 = v3 - v1;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));
            
            // Add face normal to all vertices of this face
            vertices[idx1].normal += faceNormal;
            vertices[idx2].normal += faceNormal;
            vertices[idx3].normal += faceNormal;
            
            // Update the OffModel normals too (for potential reuse)
            offModel->vertices[idx1].numIcidentTri++;
            offModel->vertices[idx2].numIcidentTri++;
            offModel->vertices[idx3].numIcidentTri++;
        }
        
        // Normalize all vertex normals
        for (size_t i = 0; i < vertices.size(); i++) {
            if (glm::length(vertices[i].normal) > 0.0001f) {
                vertices[i].normal = glm::normalize(vertices[i].normal);
                
                // Update the OffModel normals too
                offModel->vertices[i].normal.x = vertices[i].normal.x;
                offModel->vertices[i].normal.y = vertices[i].normal.y;
                offModel->vertices[i].normal.z = vertices[i].normal.z;
            }
        }
    }
    
    // Calculate face centers for explosion effect
    void calculateFaceCenters() {
        // For each triangle
        for (size_t i = 0; i < indices.size(); i += 3) {
            unsigned int idx1 = indices[i];
            unsigned int idx2 = indices[i+1];
            unsigned int idx3 = indices[i+2];
            
            glm::vec3 center = (vertices[idx1].position + vertices[idx2].position + vertices[idx3].position) / 3.0f;
            
            // Store face center for each vertex of this triangle
            vertices[idx1].faceCenter = center;
            vertices[idx2].faceCenter = center;
            vertices[idx3].faceCenter = center;
        }
    }
    
    // Calculate center of mass and bounding sphere radius
    void calculateCenterAndRadius() {
        // Use the info already computed by OFFReader
        centerOfMass = glm::vec3(
            (offModel->minX + offModel->maxX) / 2.0f,
            (offModel->minY + offModel->maxY) / 2.0f,
            (offModel->minZ + offModel->maxZ) / 2.0f
        );
        
        // boundingSphereRadius is half the extent
        boundingSphereRadius = offModel->extent / 2.0f;
    }
};

// Light structure
struct Light {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    bool enabled;
    
    Light(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, bool enabled) 
        : position(position), ambient(ambient), diffuse(diffuse), specular(specular), enabled(enabled) {}
};

#endif // MESH_H