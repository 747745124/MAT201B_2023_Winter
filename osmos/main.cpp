// Karl Yerkes
// 2022-01-20

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/graphics/al_Graphics.hpp"
#include "renderquad.hpp"
#include "points.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f randomVec3f(float scale)
{
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

string slurp(string fileName); // forward declaration

struct AlloApp : App
{
  // simple texture from image
  Texture texture;
  Texture texture_alt;
  Texture background;

  // an hdr fbo with 2 texture as render target
  FBO hdrFBO;
  Texture bright_tex;
  Texture color_tex;

  FBO blurFBO[2];
  Texture blur_tex[2];

  FBO dofFBO;
  Texture dof_tex;

  Parameter pointSize{"/pointSize", "", 1.0, 0.0, 2.0};
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};
  ShaderProgram pointShader;
  ShaderProgram blurShader;
  ShaderProgram bloomShader;
  ShaderProgram debugShader;
  ShaderProgram dofShader;

  //  simulation state
  Mesh mesh; // position *is inside the mesh* mesh.vertices() are the positions
  vector<Vec3f> velocity;
  vector<Vec3f> acceleration;
  vector<float> mass;
  vector<Point> points;

  void updateFBO(int w, int h)
  {

    // using floating point precision for HDR
    bright_tex.create2D(w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    color_tex.create2D(w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    blur_tex[0].create2D(w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    blur_tex[0].wrap(GL_CLAMP_TO_EDGE);
    blur_tex[1].create2D(w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    blur_tex[1].wrap(GL_CLAMP_TO_EDGE);

    // bind texture
    dof_tex.create2D(w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    dofFBO.bind();
    dofFBO.attachTexture2D(dof_tex);
    dofFBO.unbind();

    hdrFBO.bind();
    hdrFBO.attachTexture2D(color_tex, GL_COLOR_ATTACHMENT0, 0);
    hdrFBO.attachTexture2D(bright_tex, GL_COLOR_ATTACHMENT1, 0);

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // render to 2 targets
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    hdrFBO.unbind();

    blurFBO[0].bind();
    blurFBO[0].attachTexture2D(blur_tex[0]);
    blurFBO[0].unbind();

    blurFBO[1].bind();
    blurFBO[1].attachTexture2D(blur_tex[1]);
    blurFBO[1].unbind();
  }

  void onInit() override
  {
    // set up GUI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize); // add parameter to GUI
    gui.add(timeStep);  // add parameter to GUI
  }

  void onCreate() override
  {
    {
      const std::string filename = "/Users/naoyuki/Library/Mobile Documents/com~apple~CloudDocs/专业课/Master's/201B/allolib_playground/MAT201B-2023/osmos/textures/object_02.png";
      auto imageData = Image(filename);
      if (imageData.array().size() == 0)
      {
        cout << "failed to load image " << filename << endl;
      }
      cout << "loaded image size: " << imageData.width() << ", "
           << imageData.height() << endl;

      texture.create2D(imageData.width(), imageData.height());
      texture.submit(imageData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);
      texture.filter(Texture::LINEAR);
      texture.wrap(GL_MIRRORED_REPEAT);
    }

    {
      const std::string filename = "/Users/naoyuki/Library/Mobile Documents/com~apple~CloudDocs/专业课/Master's/201B/allolib_playground/MAT201B-2023/osmos/textures/object_01.png";
      auto imageData = Image(filename);
      if (imageData.array().size() == 0)
      {
        cout << "failed to load image " << filename << endl;
      }
      cout << "loaded image size: " << imageData.width() << ", "
           << imageData.height() << endl;

      texture_alt.create2D(imageData.width(), imageData.height());
      texture_alt.submit(imageData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);
      texture_alt.filter(Texture::LINEAR);
      texture_alt.wrap(GL_MIRRORED_REPEAT);
    }

    {
      const std::string filename = "/Users/naoyuki/Library/Mobile Documents/com~apple~CloudDocs/专业课/Master's/201B/allolib_playground/MAT201B-2023/osmos/textures/etheral.png";
      auto imageData = Image(filename);
      if (imageData.array().size() == 0)
      {
        cout << "failed to load image " << filename << endl;
      }
      cout << "loaded image size: " << imageData.width() << ", "
           << imageData.height() << endl;

      background.create2D(imageData.width(), imageData.height());
      background.submit(imageData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);
      background.filter(Texture::LINEAR);
      background.wrap(GL_CLAMP_TO_BORDER);
    }

    // compile shaders
    pointShader.compile(slurp("../point.vs"),
                        slurp("../point.fs"),
                        slurp("../point.gs"));

    blurShader.compile(slurp("../gaussian.vs"),
                       slurp("../gaussian.fs"));

    bloomShader.compile(slurp("../hdr.vs"),
                        slurp("../hdr.fs"));

    debugShader.compile(slurp("../debug.vs"),
                        slurp("../debug.fs"));

    dofShader.compile(slurp("../dof.vs"),
                      slurp("../dof.fs"));
    // set initial conditions of the simulation

    // c++11 "lambda" function
    auto randomColor = []()
    { return HSV(rnd::uniform(), 1.0f, 1.0f); };

    mesh.primitive(Mesh::POINTS);
    // does 1000 work on your system? how many can you make before you get a low
    // frame rate? do you need to use <1000?
    for (int _ = 0; _ < 1000; _++)
    {

      float ms = 3 + rnd::normal() / 2;
      if (ms < 0.5)
        ms = 0.5;

      Vec3f pos = randomVec3f(5);
      Vec3f vel = randomVec3f(0.1);
      Vec3f acc = randomVec3f(1);
      Color color = randomColor();
      Vec2f size(pow(ms, 1.0f / 3), 0);
      points.push_back(Point(ms, size, color, pos, acc, vel));

      // mesh.vertex(randomVec3f(5));
      // mesh.color(randomColor());
      // // float m = rnd::uniform(3.0, 0.5);
      // float m = 3 + rnd::normal() / 2;
      // if (m < 0.5)
      //   m = 0.5;
      // mass.push_back(m);
      // // using a simplified volume/size relationship
      // mesh.texCoord(pow(m, 1.0f / 3), 0); // s, t

      // // separate state arrays
      // velocity.push_back(randomVec3f(0.1));
      // acceleration.push_back(randomVec3f(1));
    }

    nav().pos(0, 0, 10);
    Vec3f eye = nav().pos();
    sort(points.begin(), points.end(), [&](Point &p1, Point &p2)
         { return (eye - p1.position).mag() > (eye - p2.position).mag(); });
    // Vec3f eye = nav().pos();
    // sort(mesh.vertices().begin(), mesh.vertices().end(), [&](Vec3f v1, Vec3f v2)
    //      { return (eye - v1).mag() > (eye - v2).mag(); });

    // auto indices = sort_indexes(mesh.vertices());
    // for (int i = 0; i < indices.size(); ++i)
    // {
    //   mesh.vertices()[i] = mesh.vertices()[indices[i]];
    // }

    updateFBO(width(), height());
  }

  bool freeze = false;
  void onAnimate(double dt) override
  {
    if (freeze)
      return;

    // ignore the real dt and set the time step;
    dt = timeStep;

    // Calculate forces

    // drag
    for (int i = 0; i < velocity.size(); i++)
    {
      points[i].acceleration -= points[i].velocity * 1;
    }

    // Integration
    //
    // vector<Vec3f> &position(mesh.vertices());

    for (int i = 0; i < points.size(); i++)
    {
      // "semi-implicit" Euler integration
      points[i].velocity += points[i].acceleration / points[i].mass * dt;
      points[i].position += points[i].velocity * dt;
      points[i].acceleration = 0;
      // Explicit (or "forward") Euler integration would look like this:
      // position[i] += velocity[i] * dt;
      // velocity[i] += acceleration[i] / mass[i] * dt;
    }

    Vec3f eye = nav().pos();
    sort(points.begin(), points.end(), [&](Point p1, Point p2)
         { return (eye - p1.position).mag() > (eye - p2.position).mag(); });
  }

  bool onKeyDown(const Keyboard &k) override
  {
    if (k.key() == ' ')
    {
      freeze = !freeze;
    }

    if (k.key() == '1')
    {
      // introduce some "random" forces
      for (int i = 0; i < velocity.size(); i++)
      {
        // F = ma
        acceleration[i] = randomVec3f(1) / mass[i];
      }
    }

    return true;
  }

  void onDraw(Graphics &g) override
  {
    // 1st pass render scene into floating point framebuffer
    hdrFBO.bind();
    // Clear FBO
    g.viewport(0, 0, width(), height());
    g.clear(0, 0, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    g.shader(pointShader);
    texture.bind(0);
    texture_alt.bind(1);
    g.shader().uniform("pointSize", pointSize / 10);
    g.shader().uniform("color_tex", 0);
    g.shader().uniform("color_tex_alt", 1);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);

    Mesh _mesh;
    _mesh.primitive(Mesh::POINTS);
    for (auto &point : points)
    {
      _mesh.vertex(point.position);
      _mesh.color(point.color);
      _mesh.texCoord(point.size.x, point.size.y);
    }

    g.draw(_mesh);

    texture.unbind();
    // texture_alt.unbind();
    hdrFBO.unbind();

    // 2nd blur pass (multiple times)
    g.blending(false);
    bool horizontal = true, first_itr = true;
    uint blur_amount = 10;
    for (uint i = 0; i < blur_amount; i++)
    {
      blurFBO[i % 2].bind();
      g.shader(blurShader);
      g.shader().uniform("image", 0);
      g.shader().uniform("horizontal", horizontal);

      if (first_itr)
        bright_tex.bind();
      else
        blur_tex[(i + 1) % 2].bind();

      renderQuad();
      horizontal = !horizontal;
      if (first_itr)
        first_itr = false;

      if (first_itr)
        bright_tex.unbind();
      else
        blur_tex[(i + 1) % 2].unbind();

      blurFBO[i % 2].unbind();
    }

    // hdrFBO.bind();
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
    // // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
    // // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the
    // // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
    // glBlitFramebuffer(0, 0, fbWidth(), fbHeight(), 0, 0, fbWidth(), fbHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3rd pass rendered back to dof with tone mapping
    dofFBO.bind();
    g.viewport(0, 0, fbWidth(), fbHeight());

    glDisable(GL_DEPTH_TEST);
    // glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // g.shader(debugShader);
    // g.shader().uniform("debugText", 0);
    // background.bind(0);
    // renderQuad();

    g.shader(bloomShader);
    g.shader().uniform("scene", 0);
    g.shader().uniform("bloomBlur", 1);
    color_tex.bind(0);
    blur_tex[1].bind(1);
    renderQuad();

    dofFBO.unbind();

    // 4th pass, dof as a post process
    g.shader(dofShader);
    g.shader().uniform("postBuffer", 0);
    dof_tex.bind(0);
    renderQuad();

    // // debug
    // g.clear(0, 0, 0);
    // g.viewport(0, 0, fbWidth(), fbHeight());
    // g.quadViewport(background);
  }

  void onResize(int w, int h) override { updateFBO(w, h); };
};

int main()
{
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}

string slurp(string fileName)
{
  fstream file(fileName);
  string returnValue = "";
  while (file.good())
  {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}
