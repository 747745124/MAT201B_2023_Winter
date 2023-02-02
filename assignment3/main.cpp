#include "flock.hpp"
struct AnApp : App
{
    Parameter a_dist{"Align_distance", "a_dist", 1.5, 0, 2.0f};
    Parameter s_dist{"Separate_distance", "s_dist", 0.1, 0, 2.0f};
    Parameter c_dist{"Cohesion_distance", "c_dist", 0.1, 0, 2.0f};
    std::unique_ptr<Flock> flock = nullptr;
    Mesh cones{Mesh::POINTS};
    Mesh wireframe{Mesh::LINE_STRIP};
    Light light;
    std::vector<Material> materials = {};
    // Material material;

    void setWireFrame()
    {
        wireframe.vertex(-1, -1, -1);
        wireframe.vertex(1, -1, -1);
        wireframe.vertex(1, 1, -1);
        wireframe.vertex(-1, 1, -1);
        wireframe.vertex(-1, -1, -1);
        wireframe.vertex(-1, 1, -1);
        wireframe.vertex(-1, 1, 1);
        wireframe.vertex(-1, -1, 1);
        wireframe.vertex(-1, -1, -1);
        wireframe.vertex(-1, -1, 1);
        wireframe.vertex(1, -1, 1);
        wireframe.vertex(1, -1, -1);
        wireframe.vertex(1, -1, 1);
        wireframe.vertex(1, 1, 1);
        wireframe.vertex(1, 1, -1);
        wireframe.vertex(1, 1, 1);
        wireframe.vertex(-1, 1, 1);
    };

    void onCreate() override
    {
        flock.reset(new Flock(50, static_cast<std::function<Vec3f(void)>>(r)));

        Light::globalAmbient({0.2f, 1, 0.2f});
        light.ambient({0.1f, 0.1f, 0.5f});
        light.diffuse({1, 0, 0});
        light.specular({0, 1, 1});

        for (const auto _ : flock->get_vertices())
        {
            addCone(cones);
            Material material;
            material.ambient({0, 0.5, 0});
            material.diffuse({1, 1, 0});
            material.specular({0, 0, 1});
            material.shininess(0.5);
            materials.push_back(material);
        }
        // position.vertices() = flock->get_vertices();
        setWireFrame();
        nav().pos(0, 0, 5);
    }

    void onAnimate(double dt) override
    {
        flock->update_align_velocity(a_dist.get());
        flock->update_separate_velocity(s_dist.get());
        flock->update_cohesion_velocity(c_dist.get());
        flock->update_border_velocity();
        flock->update_position(dt);
        flock->make_anomaly(dt);
        light.pos(1.f * cos(2 * al_steady_time()), 0.5f,
                  1.f * sin(2 * al_steady_time()));
    }

    bool onKeyDown(Keyboard const &k) override
    {
        if (k.key() == k.ENTER)
        {
            flock.reset(new Flock(50, static_cast<std::function<Vec3f(void)>>(r)));
        }
    }

    void onDraw(Graphics &g) override
    {
        g.clear(1.0);
        gl::depthTesting(true);
        gl::blending(true);
        // enable lighting
        g.lighting(true);
        g.light(light);

        for (int i = 0; i < flock->get_vertices().size(); i++)
        {
            g.pushMatrix();
            g.translate(flock->get_vertices()[i]);
            g.rotate(Quatd::getRotationTo(Vec3f(0, 0, 1), flock->get_velocities()[i]));
            g.scale(0.06f);
            g.material(materials[i]);
            g.draw(cones);
            g.popMatrix();
        };

        g.pointSize(1);
        g.meshColor();
        g.draw(wireframe);
    }

    void onInit() override
    {
        auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
        auto &gui = GUIdomain->newGUI();
        gui.add(a_dist);
        gui.add(c_dist);
        gui.add(s_dist);
    }
};

int main()
{
    AnApp app;
    app.start();
    return 0;
}