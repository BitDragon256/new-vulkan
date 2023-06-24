#include <stdio.h>

#include "ecs.h"
#include "ecs.cpp"

struct FooComponentA
{
    float x, y;
};
struct FooComponentB
{
    int counter;
};

class FooSys : public System<FooComponentA, FooComponentB>
{
public:
    void update(float dt, EntityId entity, ECSManager& ecs) override
    {
        auto& fa = ecs.get_component<FooComponentA>(entity);
        auto& fb = ecs.get_component<FooComponentB>(entity);
        fa.x = entity;
        fa.y = dt;

        printf("FooComponentA: %f %f | FooComponentB: %i\n", fa.x, fa.y, fb.counter);

        fb.counter++;
    }
};

int main(int argc, char** argv)
{
    ECSManager ecs;
    ecs.register_system<FooSys>();

    std::vector<EntityId> entities(100);
    for (size_t i = 0; i < entities.size(); i++)
    {
        entities[i] = ecs.create_entity();
        ecs.add_component<FooComponentA>(entities[i]);
        ecs.add_component<FooComponentB>(entities[i]);
    }

    while (entities.size() > 1)
    {
        ecs.update_systems(0.1f);

        int random = rand() % entities.size();
        ecs.delete_entity(entities[random]);
        entities.erase(entities.begin() + random);

        random = rand() % entities.size();
        ecs.remove_component<FooComponentA>(entities[random]);
        entities.erase(entities.begin() + random);

        entities.push_back(ecs.create_entity());
        ecs.add_component<FooComponentA>(entities.back());
        ecs.add_component<FooComponentB>(entities.back());
    }

    return 0;
}