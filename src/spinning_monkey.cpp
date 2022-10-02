#include <automata_engine.h>
#include <string>

/*
TODO(Noah): 

1. BMP color is red. This is like the difference in formatting between
source file and what we specify to OpenGL ...

2. Monkey looks a little squashed. Review proj mat and glViewport.
Edit: Monkey is very squashed. Matrix is bonkers.

3. UVs are messed up. Open in Blender and verify that things are sensible.
Once we find that things are sensible, do some thinking! I'm pretty sure it has
to do with assigning TWO UVs to a single vertex. In which case, that would imply
on the OpenGL side to duplicate vertices ...

4. Need to impl the view matrix -> to allow camera movement.
  - implies impl of getBasis.

*/

namespace ae = automata_engine;

void GameUpdateAndRender(game_memory_t *gameMemory);

typedef struct game_state {
  raw_model_t suzanne;
  GLuint gameShader;
  ae::GL::ibo_t suzanneIbo;
  ae::GL::vbo_t suzanneVbo;
  ae::math::transform_t suzanneTransform;
  GLuint suzanneVao;
  GLuint checkerTexture;
  ae::math::camera_t cam;
} game_state_t;

void ae::HandleWindowResize(game_memory_t *gameMemory, int newWidth, int newHeight) {}
void ae::PreInit(game_memory_t *gameMemory) {
  ae::defaultWinProfile = AUTOMATA_ENGINE_WINPROFILE_NORESIZE;
  ae::defaultWindowName = "MonkeyDemo";
}

void ae::Init(game_memory_t *gameMemory) {
  ae::GL::initGlew();
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  // TODO(Noah): Does getWindowInfo really belong in platform namespace?
  // Here's my thought process ... things that belong in platform namespace
  // are those things that we could do with the platform even if the engine was
  // not here. But a window ... the engine created it! The engine is managing the
  // window. Right?
  glViewport(
    0, 0, ae::platform::getWindowInfo().width, ae::platform::getWindowInfo().height);
  char stringBuffer[256];
  game_state_t *gameState = ae::getGameState(gameMemory);
  gameState->gameShader = ae::GL::createShader("res\\shaders\\vert.glsl", "res\\shaders\\frag.glsl");
  if ((int)gameState->gameShader == -1) { ae::setFatalExit(); return; }
  glUseProgram(gameState->gameShader);
  gameState->cam.trans.scale = ae::math::vec3(1.0f, 1.0f, 1.0f);
  gameState->cam.fov = 90.0f;
  gameState->cam.nearPlane = 0.01f;
  gameState->cam.farPlane = 1000.0f;
  gameState->suzanneTransform.scale = ae::math::vec3(1.0f, 1.0f, 1.0f);
  gameState->suzanneTransform.pos = ae::math::vec3(0.0f, 0.0f, -3.0f);
  loaded_image_t bitmap = ae::io::loadBMP("res\\highres_checker.bmp");
  if (bitmap.pixelPointer == nullptr) { ae::setFatalExit(); return; }
  //loaded_image_t bitmap = ae::platform::stbImageLoad("res\\WoodenLog_Diffuse_8K.jpg");
  gameState->checkerTexture = ae::GL::createTexture(bitmap.pixelPointer, bitmap.width, bitmap.height);
  ae::io::freeLoadedImage(bitmap);
  // I think is like "bind texture into slot 0 and interpret as TEX 2D"
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gameState->checkerTexture);
  gameState->suzanne = ae::io::loadObj("res\\monke.obj");
  if (gameState->suzanne.vertexData == nullptr) { ae::setFatalExit(); return; }
  ae::GL::objToVao(gameState->suzanne, &gameState->suzanneIbo, &gameState->suzanneVbo, 
    &gameState->suzanneVao);
  ae::io::freeObj(gameState->suzanne);
  glBindVertexArray(gameState->suzanneVao);
  ae::bifrost::registerApp("spinning_monkey", GameUpdateAndRender);
  ae::setUpdateModel(AUTOMATA_ENGINE_UPDATE_MODEL_ATOMIC);
  ae::platform::setVsync(true);
}

void GameUpdateAndRender(game_memory_t *gameMemory) {
  game_state_t *gameState = ae::getGameState(gameMemory);
  // TODO(Noah): We gotta be thinking about input latency and being frame
  // perfect, man. What is the contract of the method below?
  user_input_t userInput = {};
  ae::platform::getUserInput(&userInput);
  //process input
  float speed = 5 / 60.0f;
  ae::math::mat3_t camBasis = ae::math::mat3(buildRotMat4(
    ae::math::vec3_t(0.0f, gameState->cam.trans.eulerAngles.y, 0.0f)));
  if (userInput.keyDown[GAME_KEY_W]) {
    // NOTE(Noah): rotation matrix for a camera happens to also be the basis vectors
    // defining the space for any children of the camera.
    // ...
    // we also note that as per a game-feel thing, WASD movement is only along cam
    // local axis by Y rot. Ignore X and Z rot.
    gameState->cam.trans.pos += 
      camBasis * ae::math::vec3(0.0f, 0.0f, -speed);
  }
  if (userInput.keyDown[GAME_KEY_A]) {
    gameState->cam.trans.pos += 
      camBasis * ae::math::vec3(-speed, 0.0f, 0.0f);
  }
  if (userInput.keyDown[GAME_KEY_S]) {
    gameState->cam.trans.pos += 
      camBasis * ae::math::vec3(0.0f, 0.0f, speed);
  }
  if (userInput.keyDown[GAME_KEY_D]) {
    
    gameState->cam.trans.pos += 
      camBasis * ae::math::vec3(speed, 0.0f, 0.0f);
  }
  // NOTE(Noah): up and down movement go in world space. This is a game-feel thing.
  if (userInput.keyDown[GAME_KEY_SHIFT]) {
    gameState->cam.trans.pos +=
      ae::math::vec3(0.0f, -speed, 0.0f);
  }
  if (userInput.keyDown[GAME_KEY_SPACE]) {
    gameState->cam.trans.pos +=
      ae::math::vec3(0.0f, speed, 0.0f);
  }
  if (userInput.mouseLBttnDown) {
    gameState->cam.trans.eulerAngles += ae::math::vec3(0.0f, userInput.deltaMouseX / 5, 0.0f);
    gameState->cam.trans.eulerAngles += ae::math::vec3(-userInput.deltaMouseY / 5, 0.0f, 0.0f);
  }
  // TODO(Noah): Add Quaternions :)
  gameState->suzanneTransform.eulerAngles += ae::math::vec3(0.0f, 2.0f, 0.0f);
  // NOTE(Noah): Depth test is enabled, also clear depth buffer.
  // Depth testing culls frags that are occluded by other frags.
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // set pos of monke
  ae::GL::setUniformMat4f(gameState->gameShader, "umodel", 
    ae::math::buildMat4fFromTransform(gameState->suzanneTransform));
  // set camera transforms
  ae::GL::setUniformMat4f(gameState->gameShader, "uproj", buildProjMat(gameState->cam));
  ae::GL::setUniformMat4f(gameState->gameShader, "uview", buildViewMat(gameState->cam));
  // Do the draw call
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gameState->suzanneIbo.glHandle);
  glDrawElements(GL_TRIANGLES, gameState->suzanneIbo.count, GL_UNSIGNED_INT, NULL);

#ifndef RELEASE   
    ImGui::Begin("MonkeyDemo");
    ae::ImGuiRenderMat4("camProjMat", buildProjMat(gameState->cam));
    ae::ImGuiRenderMat4("camViewMat", buildViewMat(gameState->cam));
    ae::ImGuiRenderMat4((char *)(std::string(gameState->suzanne.modelName) + "Mat").c_str(), 
      ae::math::buildMat4fFromTransform(gameState->suzanneTransform));
    ae::ImGuiRenderVec3("camPos", gameState->cam.trans.pos);
    ae::ImGuiRenderVec3((char *)(std::string(gameState->suzanne.modelName) + "Pos").c_str(),
      gameState->suzanneTransform.pos);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
      1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
#endif
}

void ae::Close(game_memory_t *gameMemory) {
  game_state_t *gameState = ae::getGameState(gameMemory);
  // TODO(Noah): Clean up some of the OpenGL resources that we made. 
}