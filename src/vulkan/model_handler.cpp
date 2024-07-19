#include "model_handler.h"

void ModelHandler::add_model(Reference<Model> model, const Transform& transform)
{

}

void ModelHandler::remove_model(EntityId entity)
{

}

std::vector<RenderTask> DynamicModelHandler::get_render_tasks()
{
    std::vector<RenderTask> renderTasks;

}

void DynamicModelHandler::awake(EntityId entity)
{
    const auto& transform = m_ecs->get_component<Transform>(entity);
    auto& model = m_ecs->get_component<DynamicModel>(entity);

    ModelHandler::add_model(&model, transform);
}

void DynamicModelHandler::update(float dt)
{
    for (auto& modelInfo : m_models)
    {
        const auto& transform = m_ecs->get_component<Transform>(modelInfo.entity);
        modelInfo.transform = transform;
    }
}

void DynamicModelHandler::remove(EntityId entity)
{
    ModelHandler::remove_model(entity);
}

const char * DynamicModelHandler::type_name() { return "DynamicModelHandler"; }
