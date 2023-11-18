#pragma once
#include <glm/glm.hpp>
#include <glm\ext\matrix_transform.hpp>
#include <glm\ext\matrix_clip_space.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <fstream>
#include <vector>

#include "Camera.h"
#include "Vertex.h"


struct uniformBuffer 
{
  glm::mat4x4 worldProjection;
  glm::mat4x4 viewProjection;
  glm::mat4x4 objectPosition;
};


enum TopoClass
{
  Triangle = 0,
  Line = 1,
  Point = 2
};



typedef struct bufferInfo 
{
  VkBuffer buffer;
  VmaAllocation memory;
  VkDeviceSize size;
}bufferInfo;

class VulkanInterface 
{
public:
  VulkanInterface(void);
  ~VulkanInterface(void);

  void Initialize(void);
  bufferInfo  CreateVertexBuffer(int VertexCount);

  // Draw a simple 2D rectangle on screen
  void DrawRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color);
  void Draw(std::vector<Vertex> const& vertexes);

  void SetActiveCamera(Camera c);

  /*
   * Must be called before any render commands are submitted.
   * Tells the interface to begin accepting render commands
   */
  void BeginRenderPass();

  /*
   * Must be called after all render commands are submitted.
   * Tells the interface to stop accepting render commands and to submit work to the system
   */
  void EndRenderPass();

  void SetTopology(VkPrimitiveTopology topology);

  void UpdateModelMatrix(glm::vec3 const& pos, glm::vec3 const& rotDeg, glm::vec3 const& scale) 
  {
    glm::vec3 local = glm::radians(rotDeg);
    glm::vec3 localPos = pos;
    localPos.x *= -1, localPos.y *= -1;
    constantBuffer.objectPosition  = glm::identity<glm::mat4x4>();
    constantBuffer.objectPosition = glm::translate(constantBuffer.objectPosition, localPos);
    constantBuffer.objectPosition = glm::rotate(constantBuffer.objectPosition, local.x, { 1,0,0 });
    constantBuffer.objectPosition = glm::rotate(constantBuffer.objectPosition, local.y, { 0,1,0 });
    constantBuffer.objectPosition = glm::rotate(constantBuffer.objectPosition, local.z, { 0,0,1 });
    constantBuffer.objectPosition = glm::scale(constantBuffer.objectPosition, scale);
  }

  Camera& GetCamera() { return activeCamera; }

private: 

  int _frame = 0;
  bool _isRendering = false;

  glm::vec2 windowSize;
  Camera activeCamera;
  uniformBuffer constantBuffer;

  VkDevice globalDevice;
  VkPhysicalDevice physicalDevice;
  VkFence fence;
  VkSemaphore presentSemaphore;
  VkSemaphore imageGet;
  uint32_t imageIndex;
  VkSurfaceFormatKHR surfaceFormat;
  VkCommandBuffer primaryBuffer;
  VkPipeline ParentPipeline;
  std::array<VkPipeline, 3> ActivePipelines;
  int activePipeline;
  TopoClass activeTopology;
  VkPipelineLayout pipelayout;

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  std::vector<VkQueue> queues;
  vk::Instance instance;
  vk::SurfaceKHR surface;
  SDL_Window* globalWindow;
  VmaAllocator allocator;
  VkCommandPool pool;
  std::vector<VkFramebuffer> _buffers;
  
  uint32_t swapImageCount = 0;
  VkSwapchainKHR _swapChain;
  std::vector<VkImage> _swapImages;
  std::vector<VkImageView> _swapImageViews;
  std::vector<VkImageLayout> _imageLayouts;
  VkRenderPass currentRenderPass;


  std::vector<bufferInfo> activeBuffers{};

  uint32_t queueCount = 0;
  void CreateInstance(void);
  void CreateSurface(void);
  void CreateDevice(void);
  void CreatePhysicalDevice(void);
  void CreateMemoryAllocator(void);
  void CreateRenderPass(void);
  void CreateCommandPool(void);
  void CreateSwapChain(void);
  void CreateFrameBuffer(void);
  void CreateImageView(void);
  void CreateCommandBuffer(void);
  void CreateGraphicsPipeline(void);
  void UpdatePushConstants(void);
  void ReleaseActiveBuffers(void);
  void TransitionImage(uint32_t image, VkImageLayout old, VkImageLayout newL);
  void EndSingleBuffer(VkCommandBuffer buffer);
  void CommitBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
  void ReleaseVertexBuffer(bufferInfo in);
  VkShaderModule CreateShader(std::string path);
  VkSampler CreateSampler(void);
  VkImage CreateImage(void); 
  VkCommandBuffer CreateSingleBuffer();

  VkSurfaceFormatKHR VulkanInterface::SelectValidFormat(std::vector<VkSurfaceFormatKHR>& formats);


  // Pipeline helper functions
  VkPipelineShaderStageCreateInfo CreateShaderInfo(std::string path, VkShaderStageFlagBits stage);
  VkPipelineInputAssemblyStateCreateInfo CreateInputAssemblyState(void);
  VkPipelineViewportStateCreateInfo CreateViewPortState(void);
  VkPipelineRasterizationStateCreateInfo CreateaRasterizationState(void);
  VkPipelineMultisampleStateCreateInfo CreateMultiSampleInfo(void);
  VkPipelineColorBlendStateCreateInfo CreateColorBlendState(void);
  VkPipelineDepthStencilStateCreateInfo CreateDepthStencilStat(void);
  VkDescriptorSetLayout CreateDescriptorSetLayout(void);
  VkDescriptorPool CreateDescriptorPool(VkDescriptorSetLayout* setLayout);
  VkPipelineLayout CreatePipelineLayout(VkDescriptorSetLayout* setLayout);


  bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
  }
  static std::vector<char> readShaderFile(std::string path)
  {
    std::ifstream read(path, std::ios_base::binary | std::ios_base::ate);
    if (read.is_open() == false)
      throw std::runtime_error("Failed to open shader file");

    size_t fileSize = static_cast<size_t>(read.tellg());
    std::vector<char> buffer(fileSize);
    read.seekg(0);
    read.read(buffer.data(), fileSize);
    read.close();
    return buffer;
  }


};
