#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"
#include "al/math/al_Random.hpp"

using namespace al;
using namespace std;

Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }
Vec3f r_limiter_circle(float radius)
{
    while (true)
    {
        float x = rnd::uniformS();
        float y = rnd::uniformS();
        if ((x * x + y * y) > abs(1 * radius))
        {
            return Vec3f(x, y, 0.f);
        }
    }
};

RGB c() { return RGB(rnd::uniform(), rnd::uniform(), rnd::uniform()); }
al::Vec3f operator*(double f, const al::Vec3f &v)
{
    return al::Vec3f(v.x * f, v.y * f, v.z * f);
};

template <class T, class N>
inline T lerp_color(const T &input, const T &target, N amt)
{
    return input * (1.f - amt) + target * amt;
}

namespace Common
{
    constexpr float G = 6.67408;
    // calculate gravity force
    constexpr float F_G(float m1, float m2, float r, float g)
    {
        return g * m1 * m2 / (r * r * 1e18);
    };

    constexpr float DIST_LIMIT = 1e-2;

};

struct AnApp : App
{
    Parameter drag{"drag_factor", "", 1e-6, 0, 0.1};
    Parameter max_force{"max_force_log", "", 10, 1, 50};
    Parameter G_const{"Gravity Constant -11", "", Common::G, 1.f, 10000.f};
    Parameter time_step{"time_step", "", 5.f, 0.1f, 100.f};
    ParameterBool is_asymmetrical{"asymmetric force", "", 0, 0, 1.0f};

    Mesh position{Mesh::POINTS};
    std::vector<Vec3f> velocity;
    std::vector<double> masses;
    bitset<10000> is_collided;
    const int N = 1000;

    void onCreate() override
    {

        // set up sun parameters
        position.vertex(1, 0, 0);
        position.color(1, 0, 0);
        velocity.push_back(Vec3f(-1e-6f, 1e-6f, 0.f));
        masses.push_back(2e15);

        // set up planet parameters
        for (int i = 0; i < 10; i++)
        {
            position.vertex(r() * 10);
            position.color(0, 0, 1);
            velocity.push_back(Vec3f(-1e-6, 1e-6, 0));
            masses.push_back(1e10);
        }

        // set up satellite parameters
        for (int i = 0; i < 50; i++)
        {
            position.vertex(r() * 10);
            position.color(0, 0, 1);
            velocity.push_back(Vec3f(-1e-6, 1e-6, 0));
            masses.push_back(1e6);
        }

        for (int i = 0; i < N; i++)
        {
            position.vertex(r_limiter_circle(rnd::uniformS()));
            position.color(c());
            velocity.push_back(r() * 1e-10);

            // in 1e16
            masses.push_back(5 + rnd::uniformS());
        }

        nav().pos(0, 0, 5);
    }

    float accum_t = 0.f;

    void onAnimate(double dt) override
    {
        // calculate the force of gravity...
        for (int i = 0; i < velocity.size(); i++)
        {
            for (int j = 0; j < velocity.size(); j++)
            {
                if (i != j)
                {
                    auto gforce = pow(10.f, max_force.get());
                    // calculate the force of gravity, default mass is 1
                    // Gravity force is limited by max_force, default is 1e6
                    // gravity force is negligeable if the distance is too far or the mass is too small
                    Vec3f r = position.vertices()[j] - position.vertices()[i];
                    float r_mag = r.mag();

                    if (r_mag < Common::DIST_LIMIT)
                    {
                        is_collided.set(i);
                        is_collided.set(j);
                    }

                    Vec3f F = min(Common::F_G(masses[i], masses[j], r_mag, G_const.get()), gforce) * r.normalize();
                    velocity[i] += dt * (F / masses[i]) * (((i > j) && is_asymmetrical.get()) ? 0.5f : 1.f);
                }
            }
        }

        int collided_count = is_collided.count();
        float collided_percentage = collided_count / float(masses.size());
        Color center_color(collided_percentage);

        // update drag force
        for (int i = 0; i < velocity.size(); ++i)
        {
            velocity[i] += -velocity[i] * drag.get() * dt;
        }

        // update position
        for (int i = 0; i < position.vertices().size(); ++i)
        {
            if (i != 0 && is_collided.test(i))
            {
                position.vertices()[i] = lerp(position.vertices()[i], Vec3f(0.f, 0.f, 0.f), 0.05f);
                position.colors()[i] = lerp_color(position.colors()[i], center_color, 0.01f);
            }
            else
                position.vertices()[i] += velocity[i] * dt * time_step.get();
        }

        position.vertices()[0] = Vec3f(cos(accum_t), sin(accum_t), 0.f);

        accum_t += dt;
        if (accum_t > 6.28f)
            accum_t -= 6.28f;
    }

    bool onKeyDown(Keyboard const &k) override
    {
        for (int i = 0; i < position.vertices().size(); ++i)
        {
            position.vertices()[i] = r_limiter_circle(rnd::uniformS());
            velocity[i] = r() * 1e-10;
        }

        position.vertices()[0] = Vec3f(1, 0, 0);
        velocity[0] = Vec3f(-1e-6, 1e-6, 0);

        // set up planet parameters
        for (int i = 0; i < 10; i++)
        {
            position.vertices()[i + 1] = r() * 10;
            velocity[i] = (Vec3f(-1e-6, 1e-6, 0));
        }

        // set up satellite parameters
        for (int i = 0; i < 50; i++)
        {
            position.vertices()[i + 11] = r() * 10;
            velocity[i] = (Vec3f(-1e-6, 1e-6, 0));
        }

        is_collided.reset();

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