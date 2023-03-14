// Shaokang
// To do list:
// Add interactions:
// 1. User can control an point
// it will be the over the shoulder camera
// 2. when two points collide, the smaller one will be absorbed by the bigger one

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Graphics.hpp"
#include "renderquad.hpp"
#include "points.hpp"
#include "cubemap.hpp"
#include "al/sound/al_SoundFile.hpp"
#include <fstream>
#include <vector>
#include "GLFW/glfw3.h"
#include "glad/glad.h"

using namespace std;
using namespace al;
const int NUM_OF_POINTS = 500;

Vec3f randomVec3f(float scale)
{
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

string slurp(string fileName); // forward declaration

struct AlloApp : DistributedApp
{
  SoundFilePlayerTS playerTS;
  std::vector<float> buffer;
  bool loop = true;

  // simple texture from image
  Texture texture;
  Texture texture_alt;
  Texture texture_user;
  Texture background;

  // an hdr fbo with 2 texture as render target
  FBO hdrFBO;
  Texture bright_tex;
  Texture color_tex;

  FBO blurFBO[2];
  Texture blur_tex[2];

  FBO dofFBO;
  Texture dof_tex;
  // SkyBox skybox;

  GLuint _skybox_vao, _skybox_vbo, _skybox_texture;
  const std::string folder = "../blue";
  const std::vector<std::string> _paths = {folder + "/right.png", folder + "/left.png", folder + "/top.png", folder + "/bottom.png", folder + "/front.png", folder + "/back.png"};

  Parameter pointSize{"/pointSize", "", 1.0, 0.0, 2.0};
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};
  Parameter sound_volume{"/soundVolume", "", 0.0, 0.01, 0.5};

  ShaderProgram pointShader;
  ShaderProgram blurShader;
  ShaderProgram bloomShader;
  ShaderProgram debugShader;
  ShaderProgram dofShader;
  ShaderProgram skyboxShader;

  // c++11 "lambda" function
  std::function<HSV()> randomColor = []()
  { return HSV(rnd::uniform(), 1.0f, 1.0f); };

  //  simulation state
  Mesh mesh; // position *is inside the mesh* mesh.vertices() are the positions
  vector<Vec3f> velocity;
  vector<Vec3f> acceleration;
  vector<float> mass;
  vector<Point> points;
  Vec2f dir;
  // assume up is always the default
  Vec3f user_face_toward;
  Vec3f user_right;

  // parameters for generating a new particle
  float press_time[6] = {0.0f};
  bool is_initiated = false;
  float total_time;
  float move_step;

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
    gui.add(sound_volume);

    {
      const char name[] = "../data/bgm.wav";
      if (!playerTS.open(name))
      {
        std::cerr << "File not found: " << name << std::endl;
        quit();
      }
      std::cout << "sampleRate: " << playerTS.soundFile.sampleRate << std::endl;
      std::cout << "channels: " << playerTS.soundFile.channels << std::endl;
      std::cout << "frameCount: " << playerTS.soundFile.frameCount << std::endl;
      // playerTS.setLoop();
      // playerTS.setPlay();
    }
  }

  void onCreate() override
  {
    // setup skybox
    {
      glGenVertexArrays(1, &_skybox_vao);
      glGenBuffers(1, &_skybox_vbo);
      glBindVertexArray(_skybox_vao);
      glBindBuffer(GL_ARRAY_BUFFER, _skybox_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

      glBindVertexArray(0);

      // init texture
      glGenTextures(1, &_skybox_texture);
      // -----------------------------------------------
      // write your code to generate texture cubemap
      glBindTexture(GL_TEXTURE_CUBE_MAP, _skybox_texture);
      int width = 0, height = 0, channels = 0;

      for (unsigned int i = 0; i < _paths.size(); i++)
      {
        unsigned char *data = stbi_load(_paths[i].c_str(), &width, &height, &channels, 0);
        if (data)
        {
          glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
          cout << width << " " << height << endl;
          stbi_image_free(data);
        }
        else
        {
          std::stringstream ss;
          ss << "Cubemap texture failed to load at path: " << _paths[i];
          stbi_image_free(data);
        }
      }

      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    {
      const std::string filename = "../textures/object_02.png";
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
      const std::string filename = "../textures/object_01.png";
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
      const std::string filename = "../textures/object_07.png";
      auto imageData = Image(filename);
      if (imageData.array().size() == 0)
      {
        cout << "failed to load image " << filename << endl;
      }
      cout << "loaded image size: " << imageData.width() << ", "
           << imageData.height() << endl;

      texture_user.create2D(imageData.width(), imageData.height());
      texture_user.submit(imageData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);
      texture_user.filter(Texture::LINEAR);
      texture_user.wrap(GL_MIRRORED_REPEAT);
    }

    {
      const std::string filename = "../textures/etheral.png";
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

    skyboxShader.compile(slurp("../skybox.vs"),
                         slurp("../skybox.fs"));
    // set initial conditions of the simulation

    mesh.primitive(Mesh::POINTS);
    for (int _ = 0; _ < NUM_OF_POINTS; _++)
    {

      float ms = 3 + rnd::normal() / 2;
      if (ms < 0.5)
        ms = 0.5;

      Vec3f pos = randomVec3f(15);
      Vec3f vel = randomVec3f(0);
      Vec3f acc = randomVec3f(0.01);
      Color color = randomColor();
      Vec2f size(pow(ms, 1.0f / 3), 0);
      points.push_back(Point(ms, size, color, pos, acc, vel, false));
    }

    points[0].position = Vec3f(0, 0, 15.f);
    points[0].acceleration = Vec3f(0);
    points[0].velocity = Vec3f(0);
    points[0].is_user_control = true;

    // set the camera to the over the shoulder of the first point
    Vec3f offset(0.9f, 0.75f, 5.0f);
    nav().pos(points[0].position + offset);
    // nav().nudge(points[0].position + offset);

    // problem here, it's not always the first point to be the user-controlled one
    // a little modification to the sorting
    // The user controlled one always draws on top of other layer
    Vec3f eye = nav().pos();
    sort(points.begin(), points.end(), [&](Point &p1, Point &p2)
         { 
           if (p1.is_user_control)
             return true;
           if (p2.is_user_control)
             return false;
          return (eye - p1.position).mag() > (eye - p2.position).mag(); });

    updateFBO(width(), height());
  }

  bool freeze = false;

  // raw opengl way of doing things
  // just for continous movement
  void processInput()
  {
    auto window = (GLFWwindow *)defaultWindow().windowHandle();
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
      points[0].position += Vec3f(0, 0, -0.5f) / points[0].mass * timeStep;
      dir = Vec2f(0, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
      points[0].position += Vec3f(0.5, 0.0, 0) / points[0].mass * timeStep;
      dir = Vec2f(1.f, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
      points[0].position += Vec3f(-0.5, 0.0, 0) / points[0].mass * timeStep;
      dir = Vec2f(-1.f, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    {
      points[0].position += Vec3f(0, 0, 0.5f) / points[0].mass * timeStep;
      dir = Vec2f(0, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
      points[0].position += Vec3f(0.0, 0.5, 0) / points[0].mass * timeStep;
      dir = Vec2f(-1.f, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
      points[0].position += Vec3f(0, -0.5f, 0) / points[0].mass * timeStep;
      dir = Vec2f(0, 0);
    }
  };

  void onAnimate(double dt) override
  {
    if (freeze)
      return;

    // ignore the real dt and set the time step;
    dt = timeStep;

    processInput();
    // cout << points[0].velocity << endl;

    // if any two points are within a certain distance (collide)
    // the small one gets absorbed into the big one
    // delete the small one, and update the big one's velocity, mass, size

    for (int i = 0; i < points.size(); i++)
    {
      for (int j = 0; j < points.size(); j++)
      {
        if (i == j)
          continue;

        auto dist_2d = pow((points[i].position.x - points[j].position.x), 2.0f) + pow((points[i].position.y - points[j].position.y), 2.0f);
        auto dist_3d = (points[i].position - points[j].position).mag();
        auto r_sum = points[i].size.x + points[j].size.x;
        // cout << dist << " " << r_sum << endl;
        if (dist_2d < r_sum * 0.001)
        {
          if (points[i].mass > points[j].mass)
          {
            points[i].mass += points[j].mass;
            points[i].size.x = pow(points[i].mass, 1.0f / 1.5f);
            points[i].velocity += points[j].velocity * points[j].mass / points[i].mass;
            if (j == 0)
              freeze = true;
            points.erase(points.begin() + j);
          }
          else
          {
            points[j].mass += points[i].mass;
            points[j].size.x = pow(points[j].mass, 1.0f / 1.5f);
            points[j].velocity += points[i].velocity * points[i].mass / points[j].mass;
            if (i == 0)
              freeze = true;
            points.erase(points.begin() + i);
          }
        }
      }
    }

    if (points[0].mass <= 0.0)
      freeze = true;

    for (int i = 1; i < velocity.size(); i++)
    {
      points[i].acceleration -= points[i].velocity * 1;
    }

    for (int i = 1; i < points.size(); i++)
    {
      points[i].velocity += points[i].acceleration / points[i].mass * dt;
      points[i].position += points[i].velocity * dt;
      points[i].acceleration = 0;
    }

    Vec3f eye = nav().pos();
    sort(points.begin(), points.end(), [&](Point p1, Point p2)
         { if (p1.is_user_control)
             return true;
           if (p2.is_user_control)
             return false; 
          return (eye - p1.position).mag() > (eye - p2.position).mag(); });

    Vec3f offset(0.9f, 0.75f, 5.0f);
    nav().pos(points[0].position + offset);
  }

  bool onKeyUp(const Keyboard &k) override
  {
    float delta_time = 0.0f;
    Vec3f going_dir;

    if (k.key() == 'w')
    {
      delta_time = total_time - press_time[0];
      going_dir = Vec3f(0, -0.5, -1);
    }
    if (k.key() == 'a')
    {
      delta_time = total_time - press_time[1];
      going_dir = Vec3f(-1, 0, 0);
    }
    if (k.key() == 'd')
    {
      delta_time = total_time - press_time[2];
      going_dir = Vec3f(1, 0, 0);
    }
    if (k.key() == 'x')
    {
      delta_time = total_time - press_time[3];
      going_dir = Vec3f(0, 0.5, 1);
    }
    if (k.key() == 'e')
    {
      delta_time = total_time - press_time[4];
      going_dir = Vec3f(0, 1, 0);
    }
    if (k.key() == 'c')
    {
      delta_time = total_time - press_time[5];
      going_dir = Vec3f(0, -1, 0);
    }

    if (delta_time < 0)
      delta_time = 2.0;

    float ms = delta_time * 0.5;

    Vec3f pos = points[0].position - 0.1f * going_dir;
    // pseudo momentum conservation
    Vec3f vel = -0.001f * points[0].mass * going_dir / ms;
    Vec3f acc = randomVec3f(0);
    Color color = randomColor();
    Vec2f size(pow(ms, 1.0f / 1.5f), 0);

    points.push_back(Point(ms, size, color, pos, acc, vel, false));

    Vec3f eye = nav().pos();
    sort(points.begin(), points.end(), [&](Point p1, Point p2)
         { if (p1.is_user_control)
             return true;
           if (p2.is_user_control)
             return false; 
          return (eye - p1.position).mag() > (eye - p2.position).mag(); });

    points[0].mass -= ms * 0.1;
    points[0].size.x = pow(points[0].mass, 1.0f / 1.5f);
  }

  bool onKeyDown(const Keyboard &k) override
  {
    if (k.key() == 'w')
    {
      press_time[0] = total_time;
    }
    if (k.key() == 'a')
    {
      press_time[1] = total_time;
    }
    if (k.key() == 'd')
    {
      press_time[2] = total_time;
    }
    if (k.key() == 'x')
    {
      press_time[3] = total_time;
    }
    if (k.key() == 'e')
    {
      press_time[4] = total_time;
    }
    if (k.key() == 'c')
    {
      press_time[5] = total_time;
    }

    // lock camera
    {
      nav().spinF(0.0);
      nav().spinU(0.0);
      nav().spinR(0.0);
    }

    if (k.key() == ' ')
    {
      freeze = !freeze;
    }

    if (k.key() == 'r')
    {
      Vec3f offset(0.9f, 0.75f, 0.0f);
      nav().faceToward(points[0].position + offset);
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

    if (k.key() == Keyboard::Key::ENTER)
    {
      points.clear();
      freeze = false;
      for (int _ = 0; _ < NUM_OF_POINTS; _++)
      {

        float ms = 3 + rnd::normal() / 2;
        if (ms < 0.5)
          ms = 0.5;

        Vec3f pos = randomVec3f(15);
        Vec3f vel = randomVec3f(0);
        Vec3f acc = randomVec3f(0.01);
        Color color = randomColor();
        Vec2f size(pow(ms, 1.0f / 1.5f), 0);
        points.push_back(Point(ms, size, color, pos, acc, vel, false));
      }

      points[0].position = Vec3f(0, 0, 15.f);
      points[0].acceleration = Vec3f(0);
      points[0].velocity = Vec3f(0);
      points[0].is_user_control = true;

      // set the camera to the over the shoulder of the first point
      Vec3f offset(0.9f, 0.75f, 5.0f);
      nav().pos(points[0].position + offset);

      // problem here, it's not always the first point to be the user-controlled one
      // a little modification to the sorting
      // The user controlled one always draws on top of other layer
      Vec3f eye = nav().pos();
      sort(points.begin(), points.end(), [&](Point &p1, Point &p2)
           { 
           if (p1.is_user_control)
             return true;
           if (p2.is_user_control)
             return false;
          return (eye - p1.position).mag() > (eye - p2.position).mag(); });
    }

    return true;
  }

  void onDraw(Graphics &g) override
  {
    {
      if (total_time >= 3.14f)
        total_time -= 3.14f;

      total_time += 0.01f;
    }
    // 1st pass render scene into floating point framebuffer
    hdrFBO.bind();
    // Clear FBO
    g.viewport(0, 0, width(), height());
    g.clear(0, 0, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    g.shader(pointShader);
    texture.bind(0);
    texture_alt.bind(1);
    texture_user.bind(2);
    g.shader().uniform("pointSize", pointSize / 10);
    g.shader().uniform("color_tex", 0);
    g.shader().uniform("color_tex_alt", 1);
    g.shader().uniform("color_user", 2);
    g.shader().uniform("is_user", false);
    g.shader().uniform("direction", dir);
    // this one defines how other points look like
    g.shader().uniform("user_size", points[0].size.x);
    g.shader().uniform("is_dead", freeze);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);

    Mesh _mesh;
    _mesh.primitive(Mesh::POINTS);
    for (int i = 1; i < points.size(); i++)
    {
      auto point = points[i];
      _mesh.vertex(point.position);
      _mesh.color(point.color);
      _mesh.texCoord(point.size.x, point.size.y);
    }

    g.draw(_mesh);

    // draw the user controlled point
    _mesh.reset();
    _mesh.primitive(Mesh::POINTS);
    g.shader().uniform("is_user", true);
    g.shader().uniform("time", total_time);
    auto point = points[0];
    _mesh.vertex(point.position);
    _mesh.color(point.color);
    _mesh.texCoord(point.size.x, point.size.y);
    g.draw(_mesh);

    // draw skybox
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    g.shader(skyboxShader);
    g.shader().uniform("projection", view().projMatrix(width(), height()));
    g.shader().uniform("view", view().viewMatrix());
    g.shader().uniform("cubemap", 0);

    glBindVertexArray(_skybox_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _skybox_texture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default

    texture.unbind();
    // texture_alt.unbind();
    hdrFBO.unbind();

    // 2nd blur pass (multiple times)
    // g.blending(false);
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
    // // draw skybox

    // // debug
    // g.clear(0, 0, 0);
    // g.viewport(0, 0, fbWidth(), fbHeight());
    // g.quadViewport(background);
  }

  void onResize(int w, int h) override { updateFBO(w, h); };

  void onSound(AudioIOData &io) override
  {
    int frames = (int)io.framesPerBuffer();
    int channels = playerTS.soundFile.channels;
    int bufferLength = frames * channels;
    if ((int)buffer.size() < bufferLength)
    {
      buffer.resize(bufferLength);
    }
    playerTS.getFrames(frames, buffer.data(), (int)buffer.size());
    int second = (channels < 2) ? 0 : 1;
    while (io())
    {
      int frame = (int)io.frame();
      int idx = frame * channels;
      io.out(0) = buffer[idx] * sound_volume;
      io.out(1) = buffer[idx + second] * sound_volume;
    }
  }
};

int main()
{
  AlloApp app;
  app.configureAudio(44100, 512, 2, 0);
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
