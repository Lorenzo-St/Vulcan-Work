/*
 * Vulkan Windowed Program
 *
 * Copyright (C) 2016, 2018 Valve Corporation
 * Copyright (C) 2016, 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /*
 Vulkan C++ Windowed Project Template
 Create and destroy a Vulkan surface on an SDL window.
 */

 // Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED
#define VMA_IMPLEMENTATION
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include "Vulkan Interface.h"
#include "MeshData.h"



//Vertex 1 : (-0.5, -0.5, -0.5)
//Vertex 2 : (-0.5,  0.5, -0.5)
//Vertex 3 : ( 0.5,  0.5, -0.5)
//Vertex 4 : ( 0.5, -0.5, -0.5)
//Vertex 5 : (-0.5, -0.5,  0.5)
//Vertex 6 : (-0.5,  0.5,  0.5)
//Vertex 7 : ( 0.5,  0.5,  0.5)
//Vertex 8 : ( 0.5, -0.5,  0.5)

glm::vec3 verts[8] = {
  {-0.5, -0.5, -0.5}, // 0 
  {-0.5,  0.5, -0.5}, // 1
  { 0.5,  0.5, -0.5}, // 2
  { 0.5, -0.5, -0.5}, // 3
  {-0.5, -0.5,  0.5}, // 4
  {-0.5,  0.5,  0.5}, // 5
  { 0.5,  0.5,  0.5}, // 6
  { 0.5, -0.5,  0.5}  // 7
};

std::vector<Vertex> points = {
  // T1
  {verts[0], {1,1,1,1}},
  {verts[1], {1,1,1,1}},
  {verts[2], {1,1,1,1}},
  // T2
  {verts[0], {1,1,1,1}},
  {verts[2], {1,1,1,1}},
  {verts[3], {1,1,1,1}},

  // T3
  {verts[4], {1,1,1,1}},
  {verts[5], {1,1,1,1}},
  {verts[1], {1,1,1,1}},
  // T4
  {verts[4], {1,1,1,1}},
  {verts[1], {1,1,1,1}},
  {verts[0], {1,1,1,1}},

  // T5
  {verts[3], {1,1,1,1}},
  {verts[2], {1,1,1,1}},
  {verts[6], {1,1,1,1}},
  // T6
  {verts[3], {1,1,1,1}},
  {verts[6], {1,1,1,1}},
  {verts[7], {1,1,1,1}},

  // T7
  {verts[0], {1,1,1,1}},
  {verts[3], {1,1,1,1}},
  {verts[7], {1,1,1,1}},
  // T8
  {verts[0], {1,1,1,1}},
  {verts[7], {1,1,1,1}},
  {verts[4], {1,1,1,1}},

  // T9
  {verts[1], {1,1,1,1}},
  {verts[5], {1,1,1,1}},
  {verts[6], {1,1,1,1}},
  // T10
  {verts[1], {1,1,1,1}},
  {verts[6], {1,1,1,1}},
  {verts[2], {1,1,1,1}},

  // T11
  {verts[4], {1,1,1,1}},
  {verts[7], {1,1,1,1}},
  {verts[6], {1,1,1,1}},
  // T12
  {verts[4], {1,1,1,1}},
  {verts[6], {1,1,1,1}},
  {verts[5], {1,1,1,1}},

};


int main()
{
  VulkanInterface interface = VulkanInterface();
  interface.Initialize();
  Camera& activeCam = interface.GetCamera();
  // Poll for user input
  Mesh m(6);
  m.AddVertex({ {-.5f,-.5f,1}, {.5f,1,0,1} });
  m.AddVertex({ {-.5f, .5f,1}, {.5f,1,0,1} });
  m.AddVertex({ { .5f, .5f,1}, {.5f,1,0,1} });
  m.AddVertex({ {-.5f,-.5f,1}, {.5f,1,0,1} });
  m.AddVertex({ { .5f, .5f,1}, {.5f,1,0,1} });
  m.AddVertex({ { .5f,-.5f,1}, {.5f,1,0,1} });
  m.SetTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
  m.CalculateNormals();
  Mesh cube(points);
  Mesh plane(4);
  plane.AddVertex({ { .5f, 0, .5f}, {1, 1, 1, 1} });
  plane.AddVertex({ {-.5f, 0, .5f}, {1, 1, 1, 1} });
  plane.AddVertex({ {-.5f, 0,-.5f}, {1, 1, 1, 1} });
  plane.AddVertex({ { .5f, 0, .5f}, {1, 1, 1, 1} });
  plane.AddVertex({ {-.5f, 0,-.5f}, {1, 1, 1, 1} });
  plane.AddVertex({ { .5f, 0,-.5f}, {1, 1, 1, 1} });
  plane.CalculateNormals();
  bool stillRunning = true;
  float angle = 45.0f;
  float posX = -3;
  float posY = 0;
  glm::vec3 lightPos = { 0, 0, 5 };
  float lightStregnth = 3;
  interface.SetLightStrength(lightStregnth);
  interface.SetLightPosition(glm::vec4(lightPos, 1));
  bool up = false;
  float ltime = 0;
  while (stillRunning) {

    interface.BeginRenderPass();
    activeCam.RotateCamera(glm::vec3(0, 0, 45));
    //vkCmdDraw(c, 3, 1, 0, 0);
    interface.UpdateModelMatrix({ 0, 0, 5 }, { 0,0,0 }, { 1,1,1 });
    m.Draw();
    interface.UpdateModelMatrix({ 0, -5, 0 }, { 0,0,0 }, { 1000,1,1000 });
    plane.Draw();
    interface.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    interface.UpdateModelMatrix({ posX, posY, 10 }, { 0,0,0 }, { .25f, .25f, .25f });
    cube.Draw();
    interface.UpdateModelMatrix({ -3, 7, 30 }, { 0,0,0 }, { 2.5, 1, 3 });
    cube.Draw();
    interface.UpdateModelMatrix({ -30, 7, 100 }, { 0,0,0 }, { 2.5, 2.5, 2.5 });
    cube.Draw();
    interface.UpdateModelMatrix(lightPos, { angle,0,0 }, { 1,  1, 1 });
    cube.Draw();
    interface.SetLightStrength(lightStregnth);
    interface.SetLightPosition(glm::vec4(lightPos, 1));
    lightPos.x = 15 * glm::cos(ltime/10);
    lightPos.z = 15 * glm::sin(ltime/10) + 50;

    //if (up == false) 
    //{
    //  lightStregnth -= .05f;
    //  if (lightStregnth <= 0)
    //    up = true;
    //}
    //else
    //{
    //  if (lightStregnth >= 10)
    //    up = false;
    //  lightStregnth += .05f;
    //}

    interface.EndRenderPass();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {


      switch (event.type) {

      case SDL_QUIT:
        stillRunning = false;
        break;
      case SDL_KEYDOWN:

        break;
      default:
        // Do nothing.
        break;
      }
    }
    ltime += 1 / 10.0f;
    SDL_Delay(10);

  }

  // Clean up.


  return 0;
}
