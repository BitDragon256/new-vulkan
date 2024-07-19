#pragma once
#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include "material.h"
#include "reference.h"
#include "render_task.h"
#include "ecs.h"
#include "mesh.h"

class Model
{
public:
    REF(Material) m_material;
protected:
    virtual bool can_deallocate() = 0;
};

// mutable, movable
class MutableModel : public Model
{
protected:
    bool can_deallocate() override;
};
// immutable, immovable
class StaticModel : public Model
{

protected:
    bool can_deallocate() override;
};
// immutable, movable
class DynamicModel : public Model
{

protected:
    bool can_deallocate() override;
};

typedef struct ModelInfo
{
    HostBufferView<Vertex> vertices;
    HostBufferView<Index> indices;
    HostBufferSingleView<Transform> transform;
    REF(Material) material;
    EntityId entity;
} ModelInfo;

class ModelHandler
{
public:

    virtual std::vector<RenderTask> get_render_tasks() = 0;

protected:

    // adds a model to the buffers
    void add_model(REF(Model) model, const Transform& transform);
    void remove_model(EntityId entity);

    HostBuffer<Vertex> m_hostVertices;
    HostBuffer<Index> m_hostIndices;
    HostBuffer<Transform> m_hostTransforms;

    std::vector<ModelInfo> m_models;

};

class DynamicModelHandler : public ModelHandler, public System<Transform, DynamicModel>
{
public:

    std::vector<RenderTask> get_render_tasks() override;

    void awake(EntityId entity) override;

    void update(float dt) override;

    void remove(EntityId entity) override;

    const char * type_name() override;
};

#endif //MODEL_HANDLER_H
