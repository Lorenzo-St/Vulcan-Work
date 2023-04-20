#pragma once
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <fstream>
#include <vector>
struct Vertex
{
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    return attributeDescriptions;
  }
};

class VulkanInterface 
{
public:
  VulkanInterface(void);
  ~VulkanInterface(void);

  void Initialize(void);
  VkImage CreateImage(void);
  VkImageView CreateImageView(void);
  VkCommandPool CreateCommandPool(void);
  VkCommandBuffer CreateCommandBuffer(VkCommandPool pool = VK_NULL_HANDLE);
  VkShaderModule CreateShader(std::string path);
  VkPipeline CreateGraphicsPipeline(void);
  VkSampler CreateSampler(void);
  VkBuffer CreateVertexBuffer(int VertexCount);
  void BeginRenderPass(VkCommandBuffer buffer, VkPipeline pipeline);


private: 
  VkDevice globalDevice;
  VkPhysicalDevice physicalDevice;
  vk::Instance instance;
  vk::SurfaceKHR surface;
  SDL_Window* globalWindow;
  VmaAllocator allocator;
  std::vector<VkFramebuffer> _buffers;
  
  uint32_t swapImageCount = 0;
  VkSwapchainKHR _swapChain;
  std::vector<VkImage> _swapImages;

  VkRenderPass currentRenderPass;



  uint32_t queueCount = 0;

  void CreateInstance(void);
  void CreateSurface(void);
  void CreateDevice(void);
  void CreatePhysicalDevice(void);
  void CreateMemoryAllocator(void);
  void CreateRenderPass(void);
  void CreateSwapChain(void);
  void CreateFrameBuffer(void);


  // Pipeline helper functions
  VkPipelineShaderStageCreateInfo CreateShaderInfo(std::string path, VkShaderStageFlagBits stage);
  VkPipelineVertexInputStateCreateInfo CreateVertexInputState(void);
  VkPipelineInputAssemblyStateCreateInfo CreateInputAssemblyState(void);
  VkPipelineViewportStateCreateInfo CreateViewPortState(void);
  VkPipelineRasterizationStateCreateInfo CreateaRasterizationState(void);
  VkPipelineMultisampleStateCreateInfo CreateMultiSampleInfo(void);
  VkPipelineColorBlendStateCreateInfo CreateColorBlendState(void);
  VkDescriptorSetLayout CreateDescriptorSetLayout(void);
  VkDescriptorPool CreateDescriptorPool(VkDescriptorSetLayout setLayout);
  VkPipelineLayout CreatePipelineLayout(VkDescriptorSetLayout setLayout);


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
