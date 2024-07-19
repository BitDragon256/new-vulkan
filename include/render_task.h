#pragma once
#ifndef RENDER_TASK_H
#define RENDER_TASK_H

#include "host_buffer.h"
#include "nve_types.h"
#include "shader.h"

// StaticGeometry has all transforms baked into the geometry, DynamicGeometry works with dynamic transforms
enum class GeometryType { StaticGeometry, DynamicGeometry };
enum class RenderType { Raytraced, Rasterized };

typedef struct RenderInstruction
{
    GeometryType geometryType;
    RenderType renderType;
    Shader shader;
} RenderInstruction;

typedef struct RenderTask
{
    HostBufferView<Vertex> vertices;
    HostBufferView<Index> indices;
    HostBufferView<Transform> instances;

    RenderInstruction instruction;
} RenderTask;

#endif //RENDER_TASK_H
