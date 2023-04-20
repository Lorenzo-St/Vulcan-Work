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













int main()
{
  VulkanInterface interface = VulkanInterface();
  interface.Initialize();
  //VkDeviceSize offset = 0;
  //vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &vertexBuffer, &offset);
  //vkCmdDraw(CommandBuffer, 4, 1, 1, 1);
  //VkSubmitInfo  subInfo{};
  //subInfo.commandBufferCount = 1;
  //subInfo.pCommandBuffers = &CommandBuffer;
  //subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  //subInfo.waitSemaphoreCount = 0;


  //VkImageLayout newLayout = VK_IMAGE_LAYOUT_GENERAL;
  //

  //VkImageMemoryBarrier barrier{};
  //barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  //barrier.image = image;
  //barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  //barrier.newLayout = newLayout;
  //barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  //barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  //barrier.srcAccessMask = 0;
  //barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  //barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  //barrier.subresourceRange.baseMipLevel = 0;
  //barrier.subresourceRange.levelCount = 1;
  //barrier.subresourceRange.baseArrayLayer = 0;
  //barrier.subresourceRange.layerCount = 1;

  //VkPipelineStageFlags srcStage;
  //VkPipelineStageFlags dstStage;
  //srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  //dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  //vkCmdEndRenderPass(CommandBuffer);
  //vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, {}, 0, nullptr, 0, nullptr, 1, & barrier);
  //vkEndCommandBuffer(CommandBuffer);
  //





  //if (vkQueueSubmit(graphicsQueue0, 1, &subInfo, nullptr) != VK_SUCCESS)
  //  throw std::runtime_error("Queue failed");
  //
  //VkPresentInfoKHR presInfo{};
  //presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  //presInfo.swapchainCount = 0;
  ////presInfo.pSwapchains = &swapChain;

  //vkQueuePresentKHR(graphicsQueue0, &presInfo);

  // Poll for user input

  bool stillRunning = true;
  while (stillRunning) {


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
