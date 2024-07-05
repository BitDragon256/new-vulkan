#include <cmath>
#include <fstream>

#include <nve.h>

struct Bucket
{
    Bucket():
        pressure{ 0.f }, velocity(0.f), num{ 0 }
    {}
    float pressure;
    Vector2 velocity;
    size_t num;
};

class EulerFluid
{
public:

    REF(ECSManager) m_ecs;

    size_t m_sizeX;
    size_t m_sizeY;
    [[nodiscard]] size_t total_size() const
    {
        return m_sizeX * m_sizeY;
    }

    Vector2 m_displaySize{ 3.f, 3.f };

    int m_divergenceSolverSteps = 50;

    float m_density { 1.0 };
    float m_dynamicViscosity { 1.0 };

    Bucket m_fakeBucket;
    std::vector<Bucket> m_buckets;
    std::vector<Bucket> m_oldBuckets;
    [[nodiscard]] size_t pos_to_index(const size_t x, const size_t y) const
    {
        return x + m_sizeY * y;
    }
    [[nodiscard]] size_t pos_to_index(const Vector2 pos) const
    {
        return pos_to_index(std::floor(pos.x), std::floor(pos.y));
    }
    [[nodiscard]] bool out_of_bounds(const Vector2 pos) const
    {
        return pos.x < 0 || pos.y < 0 || pos.x >= static_cast<float>(m_sizeX) || pos.y >= static_cast<float>(m_sizeY);
    }
    Bucket& bucket_at(const Vector2 pos)
    {
        if (out_of_bounds(pos))
        {
            m_fakeBucket = Bucket{};
            return m_fakeBucket;
        }
        return m_buckets[pos_to_index(pos)];
    }
    Bucket& old_bucket_at(const Vector2 pos)
    {
        if (out_of_bounds(pos))
        {
            m_fakeBucket = Bucket{};
            return m_fakeBucket;
        }
        return m_oldBuckets[pos_to_index(pos)];
    }
    static void do_around(const std::function<void(const Vector2 delta)>& func)
    {
        Vector2 delta;
        for (delta.x = -1; delta.x <= 1; delta.x += 1)
            for (delta.y = -1; delta.y <= 1; delta.y += 1)
                if (delta != Vector2{0.f})
                    func(delta);
    }
    static void do_around_cross(const std::function<void(const Vector2 delta)>& func)
    {
        Vector2 delta;
        for (delta.x = -1; delta.x <= 1; delta.x += 1)
            for (delta.y = -1; delta.y <= 1; delta.y += 1)
                if ((delta.x == 0 || delta.y == 0) && delta != VECTOR2_ZERO)
                    func(delta);
    }
    Vector2 gradient(const Vector2 pos, const std::function<float(const Bucket& bucket)>& access)
    {
        return Vector2 {
            access(old_bucket_at(pos + VECTOR2_RIGHT)) - access(old_bucket_at(pos + VECTOR2_LEFT)),
            access(old_bucket_at(pos + VECTOR2_UP)) - access(old_bucket_at(pos + VECTOR2_DOWN))
        } / 2.f;
    }
    std::vector<EntityId> m_models;

    void start()
    {
        m_models.resize(total_size());
        for (size_t x = 0; x < m_sizeX; x++)
        {
            for (size_t y = 0; y < m_sizeY; y++)
            {
                const size_t index = pos_to_index(x, y);
                const EntityId id = m_ecs->m_renderer->create_default_model(DefaultModel::Cube);
                m_models[index] = id;
                auto& model = m_ecs->get_component<DynamicModel>(id);
                model.set_fragment_shader("fragments/lamb_wmat.frag.spv");
                // model.m_children.front().material->m_diffuse.r = 255.f;
                auto& transform = m_ecs->get_component<Transform>(id);
                transform.position.x = static_cast<float>(x) / static_cast<float>(m_sizeX) * m_displaySize.x;
                transform.position.y = static_cast<float>(y) / static_cast<float>(m_sizeY) * m_displaySize.y;
                transform.scale.x = m_displaySize.x / static_cast<float>(m_sizeX) * 0.9f;
                transform.scale.y = m_displaySize.y / static_cast<float>(m_sizeY) * 0.9f;
            }
        }
        m_buckets.resize(total_size());
    }
    void update(float dt)
    {
        m_oldBuckets = m_buckets;

        // navier stokes for velocity change
        // navier_stokes(dt);

        // apply
        m_oldBuckets = m_buckets;

        // get pressure gradient with new velocities
        apply_pressure_gradient(dt);

        // divergence invariant
        // divergence_invariant();

        // visualize
        for (size_t i = 0; i < total_size(); i++)
        {
            // m_ecs->get_component<Transform>(m_models[i]).position.z = glm::length(m_buckets[i].velocity);
            m_ecs->get_component<Transform>(m_models[i]).position.z = m_buckets[i].pressure;
        }
    }

private:
    Vector2 convective_change(const Vector2 pos)
    {
        const auto vel = bucket_at(pos).velocity;
        return Vector2 {
            glm::dot(vel, gradient(pos, [](const Bucket& bucket){ return bucket.velocity.x; })),
            glm::dot(vel, gradient(pos, [](const Bucket& bucket){ return bucket.velocity.y; }))
        };
    }
    Vector2 pressure_gradient(const Vector2 pos)
    {
        return gradient(pos, [](const Bucket& bucket){ return bucket.pressure; });
    }
    Vector2 laplace_velocity(const Vector2 pos)
    {
        return Vector2 {
            ( old_bucket_at(pos).velocity.x - old_bucket_at(pos + VECTOR2_LEFT).velocity.x ) - ( old_bucket_at(pos + VECTOR2_RIGHT).velocity.x - old_bucket_at(pos).velocity.x ),
            ( old_bucket_at(pos).velocity.y - old_bucket_at(pos + VECTOR2_DOWN).velocity.y ) - ( old_bucket_at(pos + VECTOR2_UP).velocity.y - old_bucket_at(pos).velocity.y )
        };
    }

    void navier_stokes(const float dt)
    {
        for (size_t x = 0; x < m_sizeX; x++)
        {
            for (size_t y = 0; y < m_sizeY; y++)
            {
                const Vector2 pos { x, y };
                // logger::log("convective change", convective_change(pos));
                // logger::log("pressure gradient", pressure_gradient(pos));
                // logger::log("laplace velocity", laplace_velocity(pos));
                // logger::log("---------------------------------");
                const Vector2 dv = - convective_change(pos) - pressure_gradient(pos) / m_density + m_dynamicViscosity / m_density * laplace_velocity(pos);

                bucket_at(pos).velocity -= dv * dt; // +=
            }
        }
    }
    void apply_pressure_gradient(const float dt)
    {
        for (size_t x = 0; x < m_sizeX; x++)
        {
            for (size_t y = 0; y < m_sizeY; y++)
            {
                const Vector2 pos { x, y };
                const auto pressureGradient = pressure_gradient(pos);
                bucket_at(pos).pressure -= glm::length(pressureGradient) * dt;
                for (float i = 0; i <= 1; i += 1.f)
                {
                    for (float j = 0; j <= 1; j += 1.f)
                    {
                        auto delta = Vector2{ i * std::ceil(pressureGradient.x), j * std::ceil(pressureGradient.y) };
                        if (delta == VECTOR2_ZERO)
                            continue;
                        delta = glm::normalize(delta);
                        bucket_at(pos + delta).pressure += glm::dot(pressureGradient, delta) * dt;
                    }
                }
            }
        }
    }
    void divergence_invariant()
    {
        for (int i = 0; i < m_divergenceSolverSteps; i++)
        for (size_t x = 0; x < m_sizeX; x++)
        {
            for (size_t y = 0; y < m_sizeY; y++)
            {
                const Vector2 pos { x, y };
                const Vector2 outFlow = bucket_at(pos).velocity;
                auto inFlow = VECTOR2_ZERO;
                do_around_cross([&](const Vector2 delta) {
                    inFlow += std::max(0.f, glm::dot(bucket_at(pos + delta).velocity, delta)) * -delta;
                });

                const Vector2 realOutFlow = outFlow - inFlow;

                bucket_at(pos).velocity -= realOutFlow;
            }
        }
    }
};

int main()
{
    Renderer renderer;
    renderer.init({});
    renderer.active_camera().m_position = Vector3{0.f, 0.f, 5.f};
    renderer.active_camera().m_rotation = Vector3{0.f, 45.f, 0.f};

    EulerFluid fluid;
    fluid.m_sizeX = 5; fluid.m_sizeY = 5;
    fluid.m_divergenceSolverSteps = 10;
    fluid.m_displaySize = Vector2{ 3.f, 3.f };

    fluid.m_ecs = &renderer.m_ecs;

    logger::log("init fluid");

    fluid.start();

    logger::log("start");

    fluid.bucket_at(Vector2{ 2,2 }).pressure = .5f;
    bool pressedButton = false;

    // render loop
    do
    {
        if (false)
        {
            std::fstream logFile("C:/Users/Lenovo/Desktop/log.txt");
            logFile << Profiler::flush_buf();
            logFile.close();
        }

        if (const auto keyPressed = renderer.get_key(GLFW_KEY_SPACE); keyPressed && !pressedButton || renderer.get_key(GLFW_KEY_R))
        {
            pressedButton = true;
            fluid.update(0.01f);
        }
        else if (!keyPressed)
        {
            pressedButton = false;
        }

        renderer.active_camera().m_position += Vector3 {
            static_cast<float>((renderer.get_key(GLFW_KEY_W) != 0) - (renderer.get_key(GLFW_KEY_S) != 0)),
            static_cast<float>((renderer.get_key(GLFW_KEY_D) != 0) - (renderer.get_key(GLFW_KEY_A) != 0)),
            static_cast<float>((renderer.get_key(GLFW_KEY_E) != 0) - (renderer.get_key(GLFW_KEY_Q) != 0))
        } * 0.01f;
    }
    while (renderer.render() != NVE_RENDER_EXIT_SUCCESS);
}