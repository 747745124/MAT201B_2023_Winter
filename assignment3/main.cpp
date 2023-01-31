#include "flock.hpp"
struct AnApp : App
{
    Parameter a_dist{"Align_distance", "a_dist", 1.5, 0, 2.0f};
    Parameter s_dist{"Separate_distance", "s_dist", 0.1, 0, 2.0f};
    Parameter c_dist{"Cohesion_distance", "c_dist", 0.1, 0, 2.0f};
    std::unique_ptr<Flock> flock = nullptr;
    Mesh position{Mesh::POINTS};
    Mesh wireframe{Mesh::LINE_STRIP};

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
        flock.reset(new Flock(100, static_cast<std::function<Vec3f(void)>>(r)));
        position.vertices() = flock->get_vertices();
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
        position.vertices() = flock->get_vertices();
    }

    bool onKeyDown(Keyboard const &k) override
    {
        if (k.key() == k.ENTER)
            flock.reset(new Flock(100, static_cast<std::function<Vec3f(void)>>(r)));
    }

    void onDraw(Graphics &g) override
    {
        g.clear(0.25);
        g.pointSize(20);
        g.meshColor();
        g.draw(position);
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