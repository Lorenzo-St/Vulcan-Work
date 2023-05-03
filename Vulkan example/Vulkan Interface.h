#pragma once
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <fstream>
#include <vector>

struct VertexInfo 
{
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;

};
struct Vertex
{
  glm::vec3 pos;
  glm::vec4 color;

  static VertexInfo GetInfo();

  static std::array<VkVertexInputBindingDescription, 1> getBindingDescriptions() {
    static std::array<VkVertexInputBindingDescription, 1> bindingDescriptions{};
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);


    return attributeDescriptions;
  }
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
  void CreateCommandPool(void);
  void ReleaseActiveBuffers(void);
  VkImage CreateImage(void);
  VkCommandBuffer CreateCommandBuffer(void);
  VkPipeline CreateGraphicsPipeline(void);
  VkSampler CreateSampler(void);
  VkShaderModule CreateShader(std::string path);
  bufferInfo  VulkanInterface::CreateVertexBuffer(int VertexCount);
  void BeginRenderPass(VkCommandBuffer buffer, VkPipeline pipeline);
  void VulkanInterface::DrawRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color, VkCommandBuffer comBuffer);
  void ReleaseVertexBuffer(bufferInfo in);
  void EndRenderPass(VkCommandBuffer buffer, VkPipeline pipeline);
private: 
  int _frame = 0;

  VkDevice globalDevice;
  VkPhysicalDevice physicalDevice;
  VkFence fence;
  VkSemaphore presentSemaphore;
  VkSemaphore imageGet;
  uint32_t imageIndex;
  VkSurfaceFormatKHR surfaceFormat;

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
  void CreateSwapChain(void);
  void CreateFrameBuffer(void);
  void CreateImageView(void);
  void TransitionImage(uint32_t image, VkImageLayout old, VkImageLayout newL);
  void EndSingleBuffer(VkCommandBuffer buffer);
  void CommitBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
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
