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
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include "vk_mem_alloc.h"
#include "Vulkan Interface.h"


VertexInfo Vertex::GetInfo()
{
  VertexInfo info;
  VkVertexInputBindingDescription bindingDescriptions{};
  bindingDescriptions.binding = 0;
  bindingDescriptions.stride = sizeof(Vertex);
  bindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  info.bindings.push_back(bindingDescriptions);

  VkVertexInputAttributeDescription PositionDescription{};
  PositionDescription.binding = 0;
  PositionDescription.location = 0;
  PositionDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
  PositionDescription.offset = offsetof(Vertex, pos);

  VkVertexInputAttributeDescription ColorDescription{};
  ColorDescription.binding = 0;
  ColorDescription.location = 1;
  ColorDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  ColorDescription.offset = offsetof(Vertex, color);
  info.attributes.push_back(PositionDescription);
  info.attributes.push_back(ColorDescription);
  return info;
}

int main()
{
  VulkanInterface interface = VulkanInterface();
  interface.Initialize();
  VkCommandBuffer c = interface.CreateCommandBuffer();
  VkPipeline p = interface.CreateGraphicsPipeline();

  // Poll for user input

  bool stillRunning = true;
  while (stillRunning) {

    interface.BeginRenderPass(c, p);
    //vkCmdDraw(c, 3, 1, 0, 0);
    interface.DrawRect({ 0, 0 }, { .5f, .5f }, { 1, 0, 0, 1}, c);
    interface.DrawRect({ .25f, .5f }, { .5f, .5f }, { 1, 1, 1, 1  }, c);
    interface.EndRenderPass(c, p);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {


      switch (event.type) {

      case SDL_QUIT:
        stillRunning = false;
        break;

      default:
        // Do nothing.
        break;
      }
    }
    SDL_Delay(10);
  }

  // Clean up.


  return 0;
}
