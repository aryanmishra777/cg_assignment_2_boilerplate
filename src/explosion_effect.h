#ifndef EXPLOSION_EFFECT_H
#define EXPLOSION_EFFECT_H

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <map>
#include "OFFReader.h"

// Storage for explosion data for OFF models
struct ExplodedVertexData {
    float originalX, originalY, originalZ;
};

// Map to store original vertex positions for each model
std::map<OffModel*, std::vector<ExplodedVertexData>> explodedModels;

// Initialize explosion data for a model
void initializeExplosion(OffModel* model) {
    if (!model) return;
    
    // Check if model already initialized
    if (explodedModels.find(model) != explodedModels.end()) {
        return; // Already initialized
    }
    
    // Create storage for original vertex positions
    std::vector<ExplodedVertexData> originalVertices;
    originalVertices.resize(model->numberOfVertices);
    
    // Store original positions
    for (int i = 0; i < model->numberOfVertices; i++) {
        originalVertices[i].originalX = model->vertices[i].x;
        originalVertices[i].originalY = model->vertices[i].y;
        originalVertices[i].originalZ = model->vertices[i].z;
    }
    
    // Store data for this model
    explodedModels[model] = originalVertices;
}

// Update model vertices for explosion effect
void updateExplosion(OffModel* model, float factor) {
    if (!model) return;
    
    // Check if model is initialized for explosion
    if (explodedModels.find(model) == explodedModels.end()) {
        initializeExplosion(model);
        return;
    }
    
    // Get original vertex data
    const std::vector<ExplodedVertexData>& originalVertices = explodedModels[model];
    
    // Calculate center of the model
    glm::vec3 center(0.0f);
    for (int i = 0; i < model->numberOfVertices; i++) {
        center.x += originalVertices[i].originalX;
        center.y += originalVertices[i].originalY;
        center.z += originalVertices[i].originalZ;
    }
    center /= static_cast<float>(model->numberOfVertices);
    
    // Apply explosion effect - move vertices away from center
    for (int i = 0; i < model->numberOfVertices; i++) {
        // Get original position
        float origX = originalVertices[i].originalX;
        float origY = originalVertices[i].originalY;
        float origZ = originalVertices[i].originalZ;
        
        // Calculate direction from center to vertex
        glm::vec3 direction(origX - center.x, origY - center.y, origZ - center.z);
        float distance = glm::length(direction);
        
        // Normalize direction (if not too small)
        if (distance > 0.0001f) {
            direction = glm::normalize(direction);
        } else {
            // Default direction if vertex is at center
            direction = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        
        // Apply explosion effect
        model->vertices[i].x = origX + direction.x * factor;
        model->vertices[i].y = origY + direction.y * factor;
        model->vertices[i].z = origZ + direction.z * factor;
    }
    
    // Recalculate normals after changing positions
    computeNormals(model);
}

// Reset model to original positions
void resetExplosion(OffModel* model) {
    if (!model) return;
    
    // Check if model is initialized for explosion
    if (explodedModels.find(model) == explodedModels.end()) {
        return; // No original data to reset to
    }
    
    // Get original vertex data
    const std::vector<ExplodedVertexData>& originalVertices = explodedModels[model];
    
    // Reset all vertices to original positions
    for (int i = 0; i < model->numberOfVertices; i++) {
        model->vertices[i].x = originalVertices[i].originalX;
        model->vertices[i].y = originalVertices[i].originalY;
        model->vertices[i].z = originalVertices[i].originalZ;
    }
    
    // Recalculate normals after resetting positions
    computeNormals(model);
}

// Clean up explosion data for a model
void cleanupExplosionData(OffModel* model) {
    if (explodedModels.find(model) != explodedModels.end()) {
        explodedModels.erase(model);
    }
}

// Clean up all explosion data
void cleanupAllExplosionData() {
    explodedModels.clear();
}

#endif // EXPLOSION_EFFECT_H