// Karl Yerkes
// 2023-01-19
// MAT 201B
// Starter Code for Alssignment 2
//

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"
#include "al/math/al_Random.hpp"

using namespace al;
using namespace std;

Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }
RGB c() { return RGB(rnd::uniform(), rnd::uniform(), rnd::uniform()); }

al::Vec3f operator*(double f, const al::Vec3f &v)
{
    return al::Vec3f(v.x * f, v.y * f, v.z * f);
};

namespace Common
{
    constexpr float G = 6.67408;
    // calculate gravity force
    constexpr float F_G(float m1, float m2, float r, float g)
    {
        return g * m1 * m2 / (r * r * 1e11);
    };

};

struct AnApp : App
{
    Parameter drag{"drag_factor", "", 0.01, 0, 0.1};
    Parameter max_force{"max_force", "", 1, 0, 1000.f};
    Parameter G_const{"Gravity Constant -11", "", Common::G, 1.f, 1000000.f};
    Parameter time_step{"time_step", "", 5.f, 0.1f, 100.f};
    ParameterBool is_asymmetrical{"asymmetric force", "", 0, 0, 1.0f};

    Mesh position{Mesh::POINTS};
    std::vector<Vec3f> velocity;

    void onCreate() override
    {
        for (int i = 0; i < 1000; i++)
        {
            position.vertex(r());
            position.color(c());
            velocity.push_back(r() * 0.01);
        }
        nav().pos(0, 0, 5);
    }

    void onAnimate(double dt) override
    {
        // calculate the force of gravity...
        for (int i = 0; i < velocity.size(); i++)
        {
            for (int j = 0; j < velocity.size(); j++)
            {
                if (i != j)
                {

                    auto gforce = max_force.get();
                    // calculate the force of gravity, default mass is 1
                    // Gravity force is limited by max_force, default is 1e6
                    // gravity force is negligeable if the distance is too far or the mass is too small
                    Vec3f r = position.vertices()[j] - position.vertices()[i];
                    float r_mag = r.mag();
                    Vec3f F = min(Common::F_G(1, 1, r_mag, G_const.get()), gforce) * r.normalize();
                    velocity[i] += dt * (F / 1.f) * (((i > j) && is_asymmetrical.get()) ? 0.1f : 1.f);
                }
            }
        }

        // semi-implicit Euler integration

        // update drag force
        for (int i = 0; i < velocity.size(); ++i)
        {
            velocity[i] += -velocity[i] * drag.get() * dt;
        }

        // update position
        for (int i = 0; i < position.vertices().size(); ++i)
        {
            position.vertices()[i] += velocity[i] * time_step.get() * dt;
        }
    }

    bool onKeyDown(Keyboard const &k) override
    {
        for (int i = 0; i < position.vertices().size(); ++i)
        {
            position.vertices()[i] = r();
            velocity[i] = r() * 0.01;
        }
        return true;
    }

    void onDraw(Graphics &g) override
    {
        g.clear(0.25);
        g.pointSize(20);
        g.meshColor();
        g.draw(position);
    }

    void onInit() override
    {
        auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
        auto &gui = GUIdomain->newGUI();
        gui.add(drag);
        gui.add(max_force);
        gui.add(G_const);
        gui.add(time_step);
        gui.add(is_asymmetrical);
    }
};

int main()
{
    AnApp app;
    app.start();
    return 0;
}