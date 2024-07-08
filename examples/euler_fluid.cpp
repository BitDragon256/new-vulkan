#include <cmath>
#include <fstream>

#include <nve.h>

struct Bucket
{
    Bucket():
        pressure{ 0.f }, velocity(0.f), density(1.f)
    {}
    Vector2 velocity;
    float fluidity;
    float density;
    float pressure;
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
    float m_overrelaxationConstant = 1.9f;

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
    Bucket& fake_bucket()
    {
        m_fakeBucket = {};
        m_fakeBucket.fluidity = 0.f;
        return m_fakeBucket;
    }
    Bucket& bucket_at(const Vector2 pos)
    {
        if (out_of_bounds(pos))
        {
            return fake_bucket();
        }
        return m_buckets[pos_to_index(pos)];
    }
    Bucket& old_bucket_at(const Vector2 pos)
    {
        if (out_of_bounds(pos))
        {
            return fake_bucket();
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
                transform.position.x = (static_cast<float>(x) / static_cast<float>(m_sizeX) - 0.5f) * m_displaySize.x;
                transform.position.y = (static_cast<float>(y) / static_cast<float>(m_sizeY) - 0.5f) * m_displaySize.y;
                transform.scale.x = m_displaySize.x / static_cast<float>(m_sizeX) * 0.3f;
                transform.scale.y = m_displaySize.y / static_cast<float>(m_sizeY) * 0.9f;
            }
        }

        m_buckets.resize(total_size());
        for (auto& bucket : m_buckets)
        {
            bucket.fluidity = 1.f;
        }
    }
    void update(float dt)
    {
        m_oldBuckets = m_buckets;

        update_velocities(dt);

        for (size_t i = 0; i < m_divergenceSolverSteps; i++)
        {
            force_incompressibility(dt);

            m_oldBuckets = m_buckets;
        }

        advect(dt);

        // visualize
        for (size_t i = 0; i < total_size(); i++)
        {
            // m_ecs->get_component<Transform>(m_models[i]).position.z = glm::length(m_buckets[i].velocity);
            // m_ecs->get_component<Transform>(m_models[i]).position.z = m_buckets[i].pressure;
            // m_ecs->get_component<DynamicModel>(m_models[i]).m_children.front().material->m_diffuse.r = m_buckets[i].pressure / 12.f + 6.f;
            auto& transform = m_ecs->get_component<Transform>(m_models[i]);
            transform.rotation.euler(Vector3(0,0,std::atan2(m_buckets[i].velocity.x, m_buckets[i].velocity.y) * RAD_TO_DEG));
            transform.scale.y = (1.f - std::exp(static_cast<float>(-m_buckets[i].velocity.length()))) * 0.5f;
        }
    }

private:

    void update_velocities(const float dt)
    {
        for (auto& bucket : m_buckets)
        {
            bucket.velocity += VECTOR2_DOWN * 0.f * dt; // gravity
        }
    }
    void force_incompressibility(const float dt)
    {
        for (size_t x = 0; x < m_sizeX; x++)
        {
            for (size_t y = 0; y < m_sizeY; y++)
            {
                const auto pos = Vector2{ x, y };
                const auto& top = old_bucket_at(pos + VECTOR2_UP);
                const auto& right = old_bucket_at(pos + VECTOR2_RIGHT);
                const auto& here = old_bucket_at(pos);
                const auto flow = m_overrelaxationConstant * (right.velocity.x - here.velocity.x + top.velocity.y - here.velocity.y);

                // solidity
                const auto tops = top.fluidity;
                const auto bottoms = old_bucket_at(pos + VECTOR2_DOWN).fluidity;
                const auto rights = right.fluidity;
                const auto lefts = old_bucket_at(pos + VECTOR2_LEFT).fluidity;

                const auto solidity = rights + bottoms + rights + lefts;
                if (solidity == 0.f)
                    continue;

                bucket_at(pos).velocity.x += flow * lefts / solidity;
                bucket_at(pos + VECTOR2_UP).velocity.x -= flow * rights / solidity;
                bucket_at(pos).velocity.y += flow * bottoms / solidity;
                bucket_at(pos + VECTOR2_RIGHT).velocity.y -= flow * tops / solidity;

                bucket_at(pos).pressure += flow / solidity / dt;
            }
        }
    }

    void advect(const float dt)
    {
        for (size_t x = 0; x < m_sizeX; x++)
        {
            for (size_t y = 0; y < m_sizeY; y++)
            {
                const auto pos = Vector2{ x, y };

                const auto& here = old_bucket_at(pos);
                const auto& top = old_bucket_at(pos + VECTOR2_UP);
                const auto& left = old_bucket_at(pos + VECTOR2_LEFT);
                const auto& topLeft = old_bucket_at(pos + VECTOR2_LEFT + VECTOR2_UP);

                const auto thisV = (here.velocity + top.velocity + left.velocity + topLeft.velocity) / 4.f;
                const auto lastPos = pos + VECTOR2_UP * 0.5f + VECTOR2_LEFT * 0.5f - thisV * dt;

                const auto inter = Vector2{ lastPos.x - std::floor(lastPos.x), lastPos.y - std::floor(lastPos.y) };
                const auto invInter = Vector2{1.f} - inter;
                bucket_at(pos).velocity =
                    invInter.x * invInter.y * old_bucket_at(lastPos).velocity
                    + inter.x * invInter.y * old_bucket_at(lastPos + VECTOR2_RIGHT).velocity
                    + inter.x * inter.y * old_bucket_at(lastPos + VECTOR2_UP).velocity
                    + invInter.x * inter.y * old_bucket_at(lastPos + VECTOR2_UP + VECTOR2_RIGHT).velocity;
            }
        }
    }

};

int main()
{
    Renderer renderer;
    RenderConfig config = {};
    config.height = 1000;
    config.width = 1000;
    config.enableValidationLayers = false;
    renderer.init(config);
    renderer.active_camera().m_position = Vector3{0.f, 0.f, 2.f};
    renderer.active_camera().m_rotation = Vector3{0.f, 90.f, 0.f};
    renderer.active_camera().m_orthographic = true;

    EulerFluid fluid;
    fluid.m_sizeX = 20; fluid.m_sizeY = 20;
    fluid.m_divergenceSolverSteps = 5;
    fluid.m_displaySize = Vector2{ 5.f, 5.f };

    fluid.m_ecs = &renderer.m_ecs;

    logger::log("init fluid");

    fluid.start();

    logger::log("start");

    fluid.bucket_at(Vector2{ fluid.m_sizeX / 2,fluid.m_sizeY / 2 }).velocity = VECTOR2_RIGHT * .1f;
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
            fluid.update(0.001f);
        }
        else if (!keyPressed)
        {
            pressedButton = false;
        }

        // renderer.active_camera().m_position += Vector3 {
        //     static_cast<float>((renderer.get_key(GLFW_KEY_W) != 0) - (renderer.get_key(GLFW_KEY_S) != 0)),
        //     static_cast<float>((renderer.get_key(GLFW_KEY_D) != 0) - (renderer.get_key(GLFW_KEY_A) != 0)),
        //     static_cast<float>((renderer.get_key(GLFW_KEY_E) != 0) - (renderer.get_key(GLFW_KEY_Q) != 0))
        // } * 0.01f;
    }
    while (renderer.render() != NVE_RENDER_EXIT_SUCCESS);
}