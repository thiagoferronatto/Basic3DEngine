#include "scene.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "glm/gtx/euler_angles.hpp"
#include "log.hpp"
#include "ppm.hpp"

// clang-format off
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
// clang-format on

static bool drawUserInterface = false;

void Scene::addCamera(Camera *camera) {
  _cameras.push_back(camera);
  _addChildren(camera);
}

void Scene::addLight(Light *light) {
  _lights.push_back(light);
  _addChildren(light);
}

const std::vector<Actor *> &Scene::actors() const { return _actors; }

const std::vector<Light *> &Scene::lights() const { return _lights; }

void Scene::addActor(Actor *actor) { _actors.push_back(actor); }

void Scene::_addChildren(Object *object) {
  // for (auto child : object->children()) {
  //   if (auto actor{std::dynamic_pointer_cast<Actor>(child)})
  //     addActor(actor);
  //   else if (auto camera{std::dynamic_pointer_cast<Camera>(child)})
  //     addCamera(camera);
  //   else if (auto light{std::dynamic_pointer_cast<Light>(child)})
  //     addLight(light);
  // }
}

static void transferActors(std::shared_ptr<GLuint[]> &buffers,
                           std::shared_ptr<GLuint[]> &textures,
                           size_t &prevObjAmt, auto &actors) {
  using namespace std::chrono;

  auto start{steady_clock::now()};

  auto objAmt{actors.size()};

  // vertex positions, normals, indices, and uv
  constexpr size_t buffersPerObject{4};

  if (buffers)
    glCheck(
        glDeleteBuffers(buffersPerObject * GLsizei(prevObjAmt), buffers.get()));

  buffers = std::make_shared<GLuint[]>(
      buffersPerObject * objAmt);  // new GLuint[buffersPerObject * objAmt];

  if (textures) glCheck(glDeleteTextures(GLsizei(prevObjAmt), textures.get()));
  textures = std::make_shared<GLuint[]>(objAmt);  // new GLuint[objAmt];

  // generating buffers for each object
  glCheck(glGenBuffers(buffersPerObject * GLsizei(objAmt), buffers.get()));

  // generating 1 texture for each object (even if it remains unused)
  glCheck(glGenTextures(GLsizei(objAmt), textures.get()));

  size_t i{};
  logMsg("[INFO] Transferring scene data to GPU...\n");
  for (auto obj : actors) {
    if (auto actor{dynamic_cast<Actor *>(obj)}) {
      auto &v{actor->mesh->vertices()}, &n{actor->mesh->normals()};
      auto &uv{actor->mesh->uv()};
      auto &t{actor->mesh->triangles()};

      // transferring vertex positions to VRAM
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i]));
      glCheck(glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(vec3), v.data(),
                           GL_STATIC_DRAW));
      //{
      //  auto p = (vec3 *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
      //  for (int k = 0; k < v.size(); ++k)
      //    printf_s("v %.3f %.3f %.3f\n", p[k].x, p[k].y, p[k].z);
      //  glUnmapBuffer(GL_ARRAY_BUFFER);
      //}

      // transferring vertex normals to VRAM
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i + objAmt]));
      glCheck(glBufferData(GL_ARRAY_BUFFER, n.size() * sizeof(vec3), n.data(),
                           GL_STATIC_DRAW));
      //{
      //  auto p = (vec3 *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
      //  for (int k = 0; k < n.size(); ++k)
      //    printf_s("vn %.3f %.3f %.3f\n", p[k].x, p[k].y, p[k].z);
      //  glUnmapBuffer(GL_ARRAY_BUFFER);
      //}

      // transferring vertex UV coords to VRAM
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i + 2 * objAmt]));
      glCheck(glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(vec2), uv.data(),
                           GL_STATIC_DRAW));

      //{
      //  auto p = (vec2 *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
      //  for (int k = 0; k < uv.size(); ++k)
      //    printf_s("uv %.3f %.3f\n", p[k].x, p[k].y);
      //  glUnmapBuffer(GL_ARRAY_BUFFER);
      //}

      // transferring triangle vertex indices to VRAM
      glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[i + 3 * objAmt]));
      glCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                           t.size() * sizeof(IndexedTriangle), t.data(),
                           GL_STATIC_DRAW));

      if (auto textureFile{actor->material.map_Kd}; !textureFile.empty()) {
        PPM bah{textureFile};
        glCheck(glBindTexture(GL_TEXTURE_2D, textures[i]));
        glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                GL_MIRRORED_REPEAT));
        glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                GL_MIRRORED_REPEAT));
        glCheck(
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        glCheck(
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        glCheck(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        glCheck(glTexImage2D(  //
            GL_TEXTURE_2D,     // target
            0,                 // level
            GL_RGB,            // internalformat
            bah.width(),       // width
            bah.height(),      // height
            0,                 // border
            GL_RGB,            // format
            GL_UNSIGNED_BYTE,  // type
            bah.pixels()       // data
            ));
        glCheck(glGenerateMipmap(GL_TEXTURE_2D));
      }
    }
    ++i;
  }
  prevObjAmt = actors.size();
  auto end{steady_clock::now()};
  logMsg("[INFO] Data transfer complete, took %g ms\n",
         duration_cast<microseconds>(end - start).count() / 1e3f);
}

static void makeMainMenu(Scene *scene, const Window &window,
                         std::shared_ptr<GLuint[]> &buffers,
                         std::shared_ptr<GLuint[]> &textures,
                         size_t &prevObjAmt) {
  if (drawUserInterface) {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::BeginMenu("Import")) {
          if (ImGui::BeginMenu("Wavefront (OBJ)")) {
            std::string path{"./assets/"};
            for (auto &entry : std::filesystem::directory_iterator{path}) {
              auto filePath{entry.path()};
              auto fileName{filePath.filename()};
              if (fileName.extension() == ".obj" &&
                  ImGui::MenuItem(fileName.string().c_str())) {
                auto mesh{new TriangleMesh(
                    TriangleMeshData::fromObj(filePath.string()))};
                scene->addActor(new Actor{fileName.string(), mesh});
                transferActors(buffers, textures, prevObjAmt, scene->actors());
              }
            }
            ImGui::EndMenu();
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        if (ImGui::BeginMenu("Scene")) {
          if (ImGui::BeginMenu("Add premade actor")) {
            if (ImGui::MenuItem("Cube")) {
              scene->addActor(new Actor{
                  "TEMPORARY", new TriangleMesh{TriangleMeshData::cube()}});
              transferActors(buffers, textures, prevObjAmt, scene->actors());
            }
            if (ImGui::MenuItem("Plane")) {
              scene->addActor(new Actor{
                  "TEMPORARY", new TriangleMesh{TriangleMeshData::plane()}});
              transferActors(buffers, textures, prevObjAmt, scene->actors());
            }
            ImGui::EndMenu();
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View")) {
        if (ImGui::BeginMenu("Render")) {
          ImGui::MenuItem("Wireframes", "", &scene->options.wireframe);
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
  }
}

void Scene::render(const Window &window, const std::function<void()> &f) {
  using namespace std::chrono;

  if (_cameras.empty()) return;

  std::shared_ptr<GLuint[]> buffers{}, textures{};
  auto objAmt{_actors.size()};
  transferActors(buffers, textures, objAmt, _actors);

  GLuint vao;
  glCheck(glGenVertexArrays(1, &vao));
  glCheck(glBindVertexArray(vao));

  bool uWasPressedInPrevFrame = false;

  // vertex shader
  auto vsLoc{glCreateShader(GL_VERTEX_SHADER)};
  glCheck(glShaderSource(vsLoc, 1, &triangleMeshVertexShader, nullptr));
  glCheck(glCompileShader(vsLoc));
  glCheckShaderCompilation(vsLoc);

  // fragment shader
  auto fsLoc{glCreateShader(GL_FRAGMENT_SHADER)};
  glCheck(glShaderSource(fsLoc, 1, &triangleMeshPbrtFragShader, nullptr));
  glCheck(glCompileShader(fsLoc));
  glCheckShaderCompilation(fsLoc);

  // program
  auto program{glCreateProgram()};
  glCheck(glAttachShader(program, vsLoc));  // sending vs to vram
  glCheck(glAttachShader(program, fsLoc));  // sending fs to vram
  glCheck(glLinkProgram(program));          // linking program
  glCheckProgramLinkage(program);           // checking linkage status
  glCheck(glUseProgram(program));           // sending program to vram

  auto mLoc{glGetUniformLocation(program, "M")};
  auto vLoc{glGetUniformLocation(program, "V")};
  auto pLoc{glGetUniformLocation(program, "P")};
  auto materialKaLoc{glGetUniformLocation(program, "material.Ka")};
  auto materialKdLoc{glGetUniformLocation(program, "material.Kd")};
  auto materialKsLoc{glGetUniformLocation(program, "material.Ks")};
  auto materialNsLoc{glGetUniformLocation(program, "material.Ns")};
  auto materialNiLoc{glGetUniformLocation(program, "material.Ni")};
  auto materialDLoc{glGetUniformLocation(program, "material.d")};
  auto selectedLoc{glGetUniformLocation(program, "selected")};
  auto texturedLoc{glGetUniformLocation(program, "textured")};
  auto lightCountLoc{glGetUniformLocation(program, "lightCount")};
  auto toneMapLoc{glGetUniformLocation(program, "toneMap")};
  auto wireframeLoc{glGetUniformLocation(program, "wireframe")};
  auto desaturateLoc{glGetUniformLocation(program, "desaturate")};
  auto ambientLoc{glGetUniformLocation(program, "ambient")};
  GLuint lightColorLocs[100];
  GLuint lightTransformLocs[100];
  for (int i = 0; i < 100; ++i) {
    char uniformName[23];
    sprintf_s(uniformName, "lights[%d].color", i);
    lightColorLocs[i] = glGetUniformLocation(program, uniformName);
    sprintf_s(uniformName, "lights[%d].transform", i);
    lightTransformLocs[i] = glGetUniformLocation(program, uniformName);
  }

  glCheck(glEnable(GL_DEPTH_TEST));
  glCheck(
      glPolygonMode(GL_FRONT_AND_BACK, options.wireframe ? GL_LINE : GL_FILL));

  logMsg("[INFO] Starting rendering loop\n");
  window.show();
  while (!window.shouldClose()) {
    auto start = std::chrono::steady_clock::now();

    glClearColor(ambient.x, ambient.y, ambient.z, 1);

    // GUI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // show FPS regardless of UI state
    ImGui::SetNextWindowPos({21, 21});
    ImGui::SetNextWindowSize({100, 75});
    if (ImGui::Begin("Performance")) {
      ImGui::Text("%.2f fps", 1.0f / _dt);
      ImGui::End();
    }

    if (drawUserInterface) {
      makeMainMenu(this, window, buffers, textures, objAmt);

      ImGui::SetNextWindowPos({0.75f * window.width(), 20});
      ImGui::SetNextWindowSize(
          {0.25f * window.width(), 0.5f * window.height()});
      if (ImGui::Begin("Scene", nullptr)) {
        if (ImGui::BeginTabBar("scene_tabs")) {
          if (ImGui::BeginTabItem("Objects")) {
            if (ImGui::CollapsingHeader("Actors",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              auto it{_actors.end()};
              for (auto &actor : _actors) {
                if (ImGui::MenuItem(actor->name().c_str()))
                  _currentObject = actor;
                if (ImGui::BeginPopupContextItem()) {
                  if (ImGui::MenuItem("Remove")) {
                    if (_currentObject == actor) _currentObject = nullptr;
                    it = std::find(_actors.begin(), _actors.end(), actor);
                    transferActors(buffers, textures, objAmt, _actors);
                  }
                  ImGui::EndPopup();
                }
              }
              if (it != _actors.end()) {
                _actors.erase(it);
                transferActors(buffers, textures, objAmt, _actors);
              }
            }
            if (ImGui::CollapsingHeader("Cameras",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              auto it{_cameras.end()};
              for (auto &cam : _cameras) {
                if (ImGui::MenuItem(cam->name().c_str())) _currentObject = cam;
                if (_cameras.size() > 1) {
                  if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove")) {
                      if (_currentObject == cam) _currentObject = nullptr;
                      it = std::find(_cameras.begin(), _cameras.end(), cam);
                    }
                    ImGui::EndPopup();
                  }
                }
              }
              if (it != _cameras.end()) _cameras.erase(it);
            }
            if (ImGui::CollapsingHeader("Lights",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              if (ImGui::Button("Add light")) addLight(new Light{vec3{1}});
              auto it{_lights.end()};
              for (auto light : _lights) {
                if (ImGui::MenuItem(light->name().c_str()))
                  _currentObject = light;
                if (ImGui::BeginPopupContextItem()) {
                  if (ImGui::MenuItem("Remove")) {
                    if (_currentObject == light) _currentObject = nullptr;
                    it = std::find(_lights.begin(), _lights.end(), light);
                  }
                  ImGui::EndPopup();
                }
              }
              if (it != _lights.end()) _lights.erase(it);
            }
            ImGui::EndTabItem();
          }
          if (ImGui::BeginTabItem("Properties")) {
            if (ImGui::ColorEdit3("Ambient", &ambient.x))
              glClearColor(ambient.x, ambient.y, ambient.z, 1);
            ImGui::Checkbox("Desaturate bright colors", &options.desaturate);
            ImGui::Checkbox("Perform tone mapping", &options.toneMap);
            ImGui::EndTabItem();
          }
          ImGui::EndTabBar();
        }
      }
      ImGui::End();

      ImGui::SetNextWindowPos(
          {0.75f * window.width(), 0.5f * window.height() + 20});
      ImGui::SetNextWindowSize(
          {0.25f * window.width(), 0.5f * window.height() - 20});
      if (ImGui::Begin("Object properties", nullptr)) {
        if (_currentObject) {
          ImGui::Text("Selected object: %s", _currentObject->name().c_str());
          if (ImGui::CollapsingHeader("Transform",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            auto pos{_currentObject->position()};
            if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
              _currentObject->translate(
                  vec4{pos - _currentObject->position(), 0});
              if (auto cam{dynamic_cast<Camera *>(_currentObject)})
                cam->updateWorldToCamera();
            }
            vec3 rotation{_currentObject->rotation()};
            if (ImGui::DragFloat3("Rotation", &rotation.x, 0.01f, -1e3, 1e3,
                                  "XYZ")) {
              auto tmp{rotation - _currentObject->rotation()};
              _currentObject->rotate(tmp);
              if (auto cam{dynamic_cast<Camera *>(_currentObject)})
                cam->updateWorldToCamera();
            }
            vec3 scale{_currentObject->scale()};
            if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.001f, 1e5f))
              _currentObject->setScale(scale);
          }

          if (auto actor{dynamic_cast<Actor *>(_currentObject)}) {
            if (ImGui::CollapsingHeader("Material",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              ImGui::ColorEdit3("Ambient (Ka)", &actor->material.Ka.x);
              ImGui::ColorEdit3("Diffuse (Kd)", &actor->material.Kd.x);
              ImGui::ColorEdit3("Specular (Ks)", &actor->material.Ks.x);
              ImGui::DragFloat("Shininess (Ns)", &actor->material.Ns, 0.1, 0);
            }
          }
          if (auto light{dynamic_cast<Light *>(_currentObject)}) {
            if (ImGui::CollapsingHeader("Light properties",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              ImGui::ColorEdit3("Color", &light->color.x);
              ImGui::DragFloat("Intensity", &light->intensity, 0.1f, 0, 1e10);
            }
          }
          if (auto cam{dynamic_cast<Camera *>(_currentObject)}) {
            if (ImGui::CollapsingHeader("Camera properties",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              if (auto fov{cam->fov()};
                  ImGui::SliderFloat("FOV", &fov, 1, 179, "%.3f deg")) {
                cam->setFov(fov);
                cam->updatePerspective();
              }
              if (auto aspect{cam->aspect()};
                  ImGui::SliderFloat("Aspect ratio", &aspect, 0.001, 10)) {
                cam->setAspect(aspect);
                cam->updatePerspective();
              }
              if (auto near{cam->near()};
                  ImGui::DragFloat("Near plane", &near, 0.1, 0, cam->far())) {
                cam->setNear(near);
                cam->updatePerspective();
              }
              if (auto far{cam->far()};
                  ImGui::DragFloat("Far plane", &far, 0.1, cam->near(), 1e5)) {
                cam->setFar(far);
                cam->updatePerspective();
              }
            }
          }
        }
      }
      ImGui::End();

      ImGui::SetNextWindowPos({0, 0.75f * window.height()});
      ImGui::SetNextWindowSize(
          {0.75f * window.width(), 0.25f * window.height()});
      if (ImGui::Begin("log", nullptr,
                       ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_AlwaysVerticalScrollbar |
                           ImGuiWindowFlags_NoResize)) {
        ImGui::Text(globalLog.c_str());
      }
      ImGui::End();
    }

    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // DEBUG CONTROLS
    float lspd{_dt * 25.0f}, aspd{_dt * 150.0f};
    vec3 displacement{};
    if (window.keyIsPressed(GLFW_KEY_ESCAPE)) _cameras[0]->setPosition({});
    if (window.keyIsPressed('W'))
      displacement += vec3{_cameras[0]->transform() * vec4{0, 0, -1, 0}};
    if (window.keyIsPressed('A'))
      displacement += vec3{_cameras[0]->transform() * vec4{-1, 0, 0, 0}};
    if (window.keyIsPressed('S'))
      displacement += vec3{_cameras[0]->transform() * vec4{0, 0, 1, 0}};
    if (window.keyIsPressed('D'))
      displacement += vec3{_cameras[0]->transform() * vec4{1, 0, 0, 0}};
    if (window.keyIsPressed(GLFW_KEY_SPACE)) displacement += vec3{0, 1, 0};
    if (window.keyIsPressed(GLFW_KEY_LEFT_CONTROL))
      displacement += vec3{0, -1, 0};
    if (dot(displacement, displacement) > std::numeric_limits<float>::epsilon())
      displacement = normalize(displacement) * lspd;
    _cameras[0]->translate(displacement);
    if (window.keyIsPressed('Q'))
      _cameras[0]->rotate({0, glm::radians(-aspd), 0});
    if (window.keyIsPressed('E'))
      _cameras[0]->rotate({0, glm::radians(aspd), 0});
    if (window.keyIsPressed('R'))
      _cameras[0]->rotate(_cameras[0]->transform() *
                          vec4{glm::radians(-aspd), 0, 0, 0});
    if (window.keyIsPressed('F'))
      _cameras[0]->rotate(_cameras[0]->transform() *
                          vec4{glm::radians(aspd), 0, 0, 0});
    if (window.keyIsPressed('Z'))
      _cameras[0]->setFov(fmaxf(_cameras[0]->fov() - 1.0f, 0.1));
    if (window.keyIsPressed('C'))
      _cameras[0]->setFov(fminf(_cameras[0]->fov() + 1.0f, 179));
    if (window.keyIsPressed('U')) {
      if (!uWasPressedInPrevFrame) drawUserInterface = !drawUserInterface;
      uWasPressedInPrevFrame = true;
    } else {
      uWasPressedInPrevFrame = false;
    }
    //  END OF DEBUG CONTROLS

    // light uniforms
    unsigned l{};
    for (auto &light : _lights) {
      auto trueLightColor{light->intensity * light->color};
      glCheck(glUniform3fv(lightColorLocs[l], 1, &trueLightColor.x));
      glCheck(glUniformMatrix4fv(lightTransformLocs[l], 1, GL_FALSE,
                                 &light->transform()[0].x));
      ++l;
    }
    glUniform1ui(lightCountLoc, l);

    glCheck(glUniform3fv(ambientLoc, 1, &ambient.x));
    glCheck(glUniformMatrix4fv(vLoc, 1, GL_FALSE,
                               &_cameras[0]->worldToCamera()[0].x));
    glCheck(glUniformMatrix4fv(pLoc, 1, GL_FALSE,
                               &_cameras[0]->perspective()[0].x));

    // UI state and rendering options
    glCheck(glUniform1i(toneMapLoc, GLint(options.toneMap)));
    glCheck(glUniform1i(wireframeLoc, GLint(options.wireframe)));
    glCheck(glUniform1i(desaturateLoc, GLint(options.desaturate)));

    unsigned i = 0;
    for (auto obj : _actors) {
      // adding VBOs to VAO
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i]));
      glCheck(glEnableVertexAttribArray(0));
      glCheck(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
      glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i + objAmt]));
      glCheck(glEnableVertexAttribArray(1));
      glCheck(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
      if (!obj->material.map_Kd.empty()) {
        glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffers[i + 2 * objAmt]));
        glCheck(glEnableVertexAttribArray(2));
        glCheck(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
        glCheck(glBindTexture(GL_TEXTURE_2D, textures[i]));
      }

      // adding EBO to VAO
      glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[i + 3 * objAmt]));

      if (auto actor{dynamic_cast<Actor *>(obj)}) {
        auto currentActorIsSelected{dynamic_cast<Actor *>(_currentObject) ==
                                    actor};

        // uniforms
        glCheck(
            glUniformMatrix4fv(mLoc, 1, GL_FALSE, &actor->transform()[0].x));
        glCheck(glUniform3fv(materialKaLoc, 1, &actor->material.Ka.x));
        glCheck(glUniform3fv(materialKdLoc, 1, &actor->material.Kd.x));
        glCheck(glUniform3fv(materialKsLoc, 1, &actor->material.Ks.x));
        glCheck(glUniform1f(materialNsLoc, actor->material.Ns));
        glCheck(glUniform1f(materialNiLoc, actor->material.Ni));
        glCheck(glUniform1f(materialDLoc, actor->material.d));
        glUniform1i(selectedLoc, 0);
        glCheck(
            glUniform1i(texturedLoc, GLint(!actor->material.map_Kd.empty())));

        // drawing elements
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL - options.wireframe);
        glCheck(glDrawElements(GL_TRIANGLES,
                               3 * GLsizei(actor->mesh->triangles().size()),
                               GL_UNSIGNED_INT, nullptr));

        if (currentActorIsSelected && !options.wireframe) {
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
          glUniform1i(selectedLoc, 1);
          glDrawElements(GL_TRIANGLES,
                         3 * GLsizei(actor->mesh->triangles().size()),
                         GL_UNSIGNED_INT, nullptr);
        }
      } else {
        logMsg(
            "[WARNING] Attempted to draw unsupported shape, ignoring "
            "request\n");
      }
      ++i;
    }

    auto prevActorAmount{_actors.size()};

    // calling custom loop function after drawing
    f();

    // just in case the user dynamically adds more actors within f()
    if (_actors.size() != prevActorAmount)
      transferActors(buffers, textures, prevActorAmount, _actors);

    // GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    window.swapBuffers();
    window.pollEvents();

    auto end = steady_clock::now();
    _dt = 1e-6f * duration_cast<microseconds>(end - start).count();
  }

  glCheck(glDeleteShader(vsLoc));     // vs in vram, freeing ram
  glCheck(glDeleteShader(fsLoc));     // fs in vram, freeing ram
  glCheck(glDeleteProgram(program));  // program in vram, freeing ram

  logMsg("[INFO] Rendering loop ended\n");
  logMsg("[INFO] Freeing GPU memory\n");

  glCheck(glDeleteBuffers(3 * GLsizei(objAmt), buffers.get()));
  logMsg("[INFO] Done\n");
  logMsg("[INFO] Freeing CPU memory\n");
  logMsg("[INFO] Done\n");
  logMsg("[INFO] Terminating...\n");
}