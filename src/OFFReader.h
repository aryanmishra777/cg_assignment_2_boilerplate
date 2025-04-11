#ifndef OFF_READER_H
#define OFF_READER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

// Define Vector3f structure for normals
typedef struct Vector3f {
    float x, y, z;
} Vector3f;

// Vertex structure with position, normal, and incident triangle count
typedef struct Vt {
    float x, y, z;          // Position
    Vector3f normal;        // Normal vector
    int numIcidentTri;      // Number of incident triangles/faces
} Vertex;

// Polygon structure with number of sides and vertex indices
typedef struct Pgn {
    int noSides;            // Number of sides/vertices
    int *v;                 // Array of vertex indices
} Polygon;

// OffModel structure to hold the entire model
typedef struct offmodel {
    Vertex *vertices;       // Array of vertices
    Polygon *polygons;      // Array of polygons
    int numberOfVertices;   // Number of vertices
    int numberOfPolygons;   // Number of polygons
    float minX, minY, minZ; // Bounding box minima
    float maxX, maxY, maxZ; // Bounding box maxima
    float extent;           // Maximum extent of the model
} OffModel;

/**
 * Reads an OFF file and constructs an OffModel.
 * @param OffFile Path to the OFF file
 * @return Pointer to the constructed OffModel, or NULL on failure
 */
OffModel* readOffFile(char* OffFile) {
    FILE* input;
    char line[256]; // Buffer for reading lines (adjust size if needed)
    int noEdges;    // Number of edges (not used but read from header)
    int i, j;
    float x, y, z;  // Vertex coordinates
    int nv, np;     // Number of vertices and polygons
    OffModel* model;

    // Open the file
    input = fopen(OffFile, "r");
    if (!input) {
        printf("Failed to open file: %s\n", OffFile);
        return NULL;
    }

    // Read the header line
    if (fgets(line, sizeof(line), input) == NULL) {
        printf("Failed to read OFF header\n");
        fclose(input);
        return NULL;
    }

    char type[10];
    if (sscanf(line, "%s", type) != 1 || strcmp(type, "OFF") != 0) {
        printf("Not an OFF file: %s\n", line);
        fclose(input);
        return NULL;
    }

    // Read vertex, face, and edge counts
    while (fgets(line, sizeof(line), input)) {
        char* ptr = line;
        while (*ptr == ' ' || *ptr == '\t') ptr++; // Skip leading whitespace
        if (*ptr == '#' || *ptr == '\n') continue; // Skip comments and empty lines
        if (sscanf(ptr, "%d %d %d", &nv, &np, &noEdges) == 3) break;
    }
    if (feof(input)) {
        printf("Failed to read vertex, face, edge counts\n");
        fclose(input);
        return NULL;
    }

    // Validate counts
    if (nv <= 0 || nv > 1000000 || np <= 0 || np > 2000000) {
        printf("Invalid vertex or polygon counts: %d vertices, %d polygons\n", nv, np);
        fclose(input);
        return NULL;
    }

    // Allocate the model
    model = (OffModel*)malloc(sizeof(OffModel));
    if (!model) {
        printf("Failed to allocate model\n");
        fclose(input);
        return NULL;
    }
    model->numberOfVertices = nv;
    model->numberOfPolygons = np;

    // Initialize bounding box
    model->minX = model->minY = model->minZ = FLT_MAX;
    model->maxX = model->maxY = model->maxZ = -FLT_MAX;

    // Allocate vertices array
    model->vertices = (Vertex*)malloc(nv * sizeof(Vertex));
    if (!model->vertices) {
        printf("Failed to allocate vertices\n");
        free(model);
        fclose(input);
        return NULL;
    }

    // Allocate polygons array
    model->polygons = (Polygon*)malloc(np * sizeof(Polygon));
    if (!model->polygons) {
        printf("Failed to allocate polygons\n");
        free(model->vertices);
        free(model);
        fclose(input);
        return NULL;
    }

    // Read vertices
    for (i = 0; i < nv; i++) {
        while (fgets(line, sizeof(line), input)) {
            char* ptr = line;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            if (*ptr == '#' || *ptr == '\n') continue;
            if (sscanf(ptr, "%f %f %f", &x, &y, &z) == 3) {
                model->vertices[i].x = x;
                model->vertices[i].y = y;
                model->vertices[i].z = z;
                model->vertices[i].numIcidentTri = 0;
                model->vertices[i].normal.x = 0.0f;
                model->vertices[i].normal.y = 0.0f;
                model->vertices[i].normal.z = 0.0f;

                // Update bounding box
                if (x < model->minX) model->minX = x;
                if (x > model->maxX) model->maxX = x;
                if (y < model->minY) model->minY = y;
                if (y > model->maxY) model->maxY = y;
                if (z < model->minZ) model->minZ = z;
                if (z > model->maxZ) model->maxZ = z;
                break;
            }
        }
        if (feof(input)) {
            printf("Failed to read vertex %d\n", i);
            for (j = 0; j < i; j++) {
                // No polygon vertex arrays allocated yet
            }
            free(model->vertices);
            free(model->polygons);
            free(model);
            fclose(input);
            return NULL;
        }
    }

    // Read faces
    for (i = 0; i < np; i++) {
        while (fgets(line, sizeof(line), input)) {
            char* ptr = line;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            if (*ptr == '#' || *ptr == '\n') continue;

            // Parse the face line
            char* token = strtok(ptr, " \t");
            if (token == NULL) continue;
            int n;
            if (sscanf(token, "%d", &n) != 1) continue;
            int values[100]; // Assumes max sides < 100
            values[0] = n;
            int count = 1;
            while ((token = strtok(NULL, " \t")) != NULL && count < 100) {
                if (sscanf(token, "%d", &values[count]) == 1) {
                    count++;
                } else {
                    break;
                }
            }
            if (count == n + 1) {
                model->polygons[i].noSides = n;
                model->polygons[i].v = (int*)malloc(n * sizeof(int));
                if (!model->polygons[i].v) {
                    printf("Failed to allocate polygon vertices\n");
                    for (j = 0; j < i; j++) {
                        free(model->polygons[j].v);
                    }
                    free(model->vertices);
                    free(model->polygons);
                    free(model);
                    fclose(input);
                    return NULL;
                }
                for (j = 0; j < n; j++) {
                    model->polygons[i].v[j] = values[j + 1];
                    if (values[j + 1] < 0 || values[j + 1] >= nv) {
                        printf("Invalid vertex index %d in polygon %d\n", values[j + 1], i);
                        for (int k = 0; k <= i; k++) {
                            free(model->polygons[k].v);
                        }
                        free(model->vertices);
                        free(model->polygons);
                        free(model);
                        fclose(input);
                        return NULL;
                    }
                }
                break;
            } else {
                printf("Invalid face line %d: expected %d indices, got %d\n", i, n, count - 1);
                for (j = 0; j < i; j++) {
                    free(model->polygons[j].v);
                }
                free(model->vertices);
                free(model->polygons);
                free(model);
                fclose(input);
                return NULL;
            }
        }
        if (feof(input)) {
            printf("Failed to read face %d\n", i);
            for (j = 0; j < i; j++) {
                free(model->polygons[j].v);
            }
            free(model->vertices);
            free(model->polygons);
            free(model);
            fclose(input);
            return NULL;
        }
    }

    // Calculate model extent
    float extentX = model->maxX - model->minX;
    float extentY = model->maxY - model->minY;
    float extentZ = model->maxZ - model->minZ;
    model->extent = extentX;
    if (extentY > model->extent) model->extent = extentY;
    if (extentZ > model->extent) model->extent = extentZ;
    if (model->extent <= 0.0f) model->extent = 1.0f;

    fclose(input);
    return model;
}

/**
 * Computes vertex normals for the model based on face normals.
 * Uses the first three vertices for faces with 3 or more sides.
 * @param model Pointer to the OffModel
 */
void computeNormals(OffModel* model) {
    if (!model) return;

    // Reset normals and counts
    for (int i = 0; i < model->numberOfVertices; i++) {
        model->vertices[i].normal.x = 0.0f;
        model->vertices[i].normal.y = 0.0f;
        model->vertices[i].normal.z = 0.0f;
        model->vertices[i].numIcidentTri = 0;
    }

    // Calculate face normals and accumulate to vertices
    for (int i = 0; i < model->numberOfPolygons; i++) {
        if (model->polygons[i].noSides < 3) continue;

        // Use first three vertices to compute face normal
        int v1 = model->polygons[i].v[0];
        int v2 = model->polygons[i].v[1];
        int v3 = model->polygons[i].v[2];

        float ax = model->vertices[v2].x - model->vertices[v1].x;
        float ay = model->vertices[v2].y - model->vertices[v1].y;
        float az = model->vertices[v2].z - model->vertices[v1].z;

        float bx = model->vertices[v3].x - model->vertices[v1].x;
        float by = model->vertices[v3].y - model->vertices[v1].y;
        float bz = model->vertices[v3].z - model->vertices[v1].z;

        // Cross product
        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;

        // Normalize face normal
        float len = sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 0.00001f) {
            nx /= len;
            ny /= len;
            nz /= len;
        }

        // Add normal to all vertices of the face
        for (int j = 0; j < model->polygons[i].noSides; j++) {
            int v = model->polygons[i].v[j];
            model->vertices[v].normal.x += nx;
            model->vertices[v].normal.y += ny;
            model->vertices[v].normal.z += nz;
            model->vertices[v].numIcidentTri++;
        }
    }

    // Normalize vertex normals
    for (int i = 0; i < model->numberOfVertices; i++) {
        if (model->vertices[i].numIcidentTri > 0) {
            float len = sqrt(
                model->vertices[i].normal.x * model->vertices[i].normal.x +
                model->vertices[i].normal.y * model->vertices[i].normal.y +
                model->vertices[i].normal.z * model->vertices[i].normal.z
            );
            if (len > 0.00001f) {
                model->vertices[i].normal.x /= len;
                model->vertices[i].normal.y /= len;
                model->vertices[i].normal.z /= len;
            }
        }
    }
}

/**
 * Frees the memory allocated for an OffModel.
 * @param model Pointer to the OffModel to free
 * @return 1 on success, 0 if model is NULL
 */
int FreeOffModel(OffModel* model) {
    int i;
    if (model == NULL) return 0;

    if (model->vertices) {
        free(model->vertices);
    }

    if (model->polygons) {
        for (i = 0; i < model->numberOfPolygons; i++) {
            if (model->polygons[i].v) {
                free(model->polygons[i].v);
            }
        }
        free(model->polygons);
    }

    free(model);
    return 1;
}

#endif // OFF_READER_H