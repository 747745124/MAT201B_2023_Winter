// Karl Yerkes
// 2023-01-13
// MAT 201B
// Starter Code for Alssignment 1
//

#include "allolib/include/al/app/al_App.hpp"
#include "allolib/include/al/graphics/al_Image.hpp"
#include "allolib/include/al/io/al_File.hpp"
// #include <filesystem>

using namespace std;
using namespace al;

enum class TARGET
{
  ORIGINAL,
  CUBE,
  CYLINDER,
  CUSTOM
};

struct AnApp : al::App
{

  Mesh original, current, cube, cylinder, custom;
  Mesh wireframe;
  // wireframe.primitive(al::Mesh::TRIANGLE);
  TARGET target = TARGET::ORIGINAL;

  // EBO would be efficient
  void setWireFrame()
  {
    wireframe.vertex(0, 0, 0);
    wireframe.vertex(1, 0, 0);
    wireframe.vertex(1, 1, 0);
    wireframe.vertex(0, 1, 0);
    wireframe.vertex(0, 0, 0);
    wireframe.vertex(0, 1, 0);
    wireframe.vertex(0, 1, 1);
    wireframe.vertex(0, 0, 1);
    wireframe.vertex(0, 0, 0);
    wireframe.vertex(0, 0, 1);
    wireframe.vertex(1, 0, 1);
    wireframe.vertex(1, 0, 0);
    wireframe.vertex(1, 0, 1);
    wireframe.vertex(1, 1, 1);
    wireframe.vertex(1, 1, 0);
    wireframe.vertex(1, 1, 1);
    wireframe.vertex(0, 1, 1);
  }
  // hint: add more meshes here: cube, cylindar, custom
  void onCreate() override
  {
    setWireFrame();
    // note: File::currentPath() is going to be something like:
    //   /Users/foo/allolib_playground/code-folder/bin
    // and we will put an image file next to our .cpp in this folder
    //   /Users/foo/allolib_playground/code-folder
    // so we add the "../" to our image file name
    const std::string filename = File::currentPath() + "../wheel.png";
    cout << "loading: " << filename << endl;
    auto image = Image(filename);
    if (image.array().size() == 0)
    {
      cout << "failed to load image " << filename << endl;
      exit(1);
    }
    cout << "image size: " << image.width() << "x" << image.height() << endl;

    Vec3d mean_pos; // note: we will learn average pixel position
    Vec3d average_cube_pos;
    Vec3d average_cylinder_pos;
    Vec3d average_custom_pos;
    vector<std::pair<Color, Vec3d>> custom_vertices;

    // get the RGB of each pixel in the image
    for (int column = 0; column < image.width(); ++column)
    {
      for (int row = 0; row < image.height(); ++row)
      {
        // here's how to look up a pixel in an image
        auto pixel = image.at(column, row);

        // pixel has .r .g .b and .a ~ each is an 8-bit unsigned integer
        // we need a float on (0, 1) so we divide by 255.0
        Color color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);

        // here we normalize the row and column while maintaining the
        // correct aspect ratio by dividing by width
        Vec3d position(1.0 * column / image.width(),
                       1.0 - 1.0 * row / image.width(), 0.0);

        mean_pos += position; // we will learn the average pixel position
        average_cube_pos += Vec3d(color.r, color.g, color.b);

        // add data to a mesh
        original.vertex(position);
        original.color(color);

        // add data to a mesh
        current.vertex(position);
        current.color(color);

        // hint: configure more meshes here
        // transform the vertices to a RGB cube
        Vec3d cubeVertex = Vec3d(color.r, color.g, color.b);
        cube.vertex(cubeVertex);
        cube.color(color);
        // transform the vertices to a HSV cylinder

        HSV hsv(color);
        // assume the cynlinder sit at (0,0,0)
        Vec3d cylinderVertex = Vec3d(hsv.s * cos(hsv.h * 2 * M_PI), hsv.v, hsv.s * sin(hsv.h * 2 * M_PI));
        cylinder.vertex(cylinderVertex);
        cylinder.color(color);
        average_cylinder_pos += cylinderVertex;

        // transform the vertices to your custom arrangement
        custom_vertices.push_back(std::make_pair(color, position));
      }
    }

    // Transform the vertices to a custom arrangement
    // sort the vertices by hue
    std::sort(custom_vertices.begin(), custom_vertices.end(), [](const std::pair<Color, Vec3d> &a, const std::pair<Color, Vec3d> &b)
              {
      HSV hsv_a(a.first);
      HSV hsv_b(b.first);
      return hsv_a.s < hsv_b.s; });

    // modify the position to a spiral
    // r(t) = 1/t
    // x(t) = r(t)cos(t)
    // y(t) = r(t)sin(t)
    // z(t) = t
    for (int i = 0; i < custom_vertices.size(); i++)
    {

      double t = static_cast<double>(i) * 20 * M_PI / custom_vertices.size();
      double r = 1.0f / t;
      // add a clamp
      if (r > 100.0f)
        r = 100.0f;
      Vec3d position = Vec3d(r * cos(t), r * sin(t), t);
      custom_vertices[i].second = position;
    }

    // add the vertices to the mesh
    for (auto &v : custom_vertices)
    {
      custom.vertex(v.second);
      custom.color(v.first);
      average_custom_pos += v.second;
    }

    // here we center the image by subtracting the average position
    mean_pos /= original.vertices().size();
    average_cube_pos /= cube.vertices().size();
    average_cylinder_pos /= cylinder.vertices().size();
    average_custom_pos /= custom.vertices().size();

    for (auto &v : original.vertices())
      v -= mean_pos;
    for (auto &v : current.vertices())
      v -= mean_pos;
    for (auto &v : cube.vertices())
      v -= average_cube_pos;
    for (auto &v : wireframe.vertices())
      v -= average_cube_pos;
    for (auto &v : cylinder.vertices())
      v -= average_cylinder_pos;
    for (auto &v : custom.vertices())
      v -= average_custom_pos;

#ifdef DEBUG
    for (auto &v : custom_vertices)
    {
      cout << v.second << endl;
    }
#endif

    // configure the meshs to render as points
    wireframe.primitive(Mesh::LINE_STRIP);
    original.primitive(Mesh::POINTS);
    current.primitive(Mesh::POINTS);
    cube.primitive(Mesh::POINTS);
    cylinder.primitive(Mesh::POINTS);
    custom.primitive(Mesh::POINTS);

    // set the viewer position 7 units back
    nav().pos(0, 0, 4);
  }

  void lerpHelper(const Mesh &&target, const float amount)
  {
    for (int i = 0; i < current.vertices().size(); i++)
    {
      this->current.vertices()[i].lerp(target.vertices()[i], amount);
    };
  };

  void onAnimate(double dt) override
  {
    switch (target)
    {
    case TARGET::ORIGINAL:
    {
      // note: animate changes to the CURRENT mesh using linear interpolation
      lerpHelper(std::move(original), 0.1f);
      break;
    }
    case TARGET::CUBE:
    {
      lerpHelper(std::move(cube), 0.1f);
      break;
    }
    case TARGET::CYLINDER:
    {
      lerpHelper(std::move(cylinder), 0.1f);
      break;
    }
    case TARGET::CUSTOM:
    {
      lerpHelper(std::move(custom), 0.1f);
      break;
    }
    default:
      break;
    }
  }

  bool onKeyDown(Keyboard const &k) override
  {
    switch (k.key())
    {
    case '1':
    {
      target = TARGET::ORIGINAL;
      break;
      // note: trigger transition back to original vertex positions
    }

    case '2':
    {
      target = TARGET::CUBE;
      break;
      // note: trigger transition toward an RGB cube
    }

    case '3':
    {
      target = TARGET::CYLINDER;
      break;
      // note: trigger transition toward an HSV cylinder
    }

    case '4':
    {
      target = TARGET::CUSTOM;
      break;
      // note: trigger transition to your custom arrangement
    }
      // nagivate the camera
    default:
      break;
    }
    return true;
  };

  void onDraw(Graphics &g) override
  {
    // note: you don't need to touch anything here
    g.clear(0.2);
    g.meshColor();
    if (target == TARGET::CUBE)
      g.draw(wireframe);
    g.draw(current); // draw the current mesh
  }
};

int main()
{
  AnApp app;
  app.start();
  return 0;
}
