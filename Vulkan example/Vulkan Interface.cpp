#include "Vulkan Interface.h"
#include <iostream>
#include <iomanip>
#ifdef _DEBUG
#include <Windows.h>
#include <iostream>
#include <sstream>

#define DBOUT( s )            \
{                             \
   std::wostringstream os_;    \
   os_ << s;                   \
   OutputDebugStringW( os_.str().c_str() );  \
}
#endif

namespace pass
{
  VulkanInterface* interface;
}



VulkanInterface::VulkanInterface(void)
{
  _swapChain = 0;
  allocator = 0;
  currentRenderPass = 0;
  globalDevice = 0;
  globalWindow = 0;
  physicalDevice = 0;
  pool = 0;
  fence = 0;
  imageIndex = 0;
  presentSemaphore = 0;
  imageGet = 0;
  activeCamera = { 45, {0,0,0}, {0, 0, 0} };
  constantBuffer.worldProjection = glm::identity<glm::mat4x4>();
  constantBuffer.objectPosition = glm::identity<glm::mat4x4>();
  pipelayout = nullptr;
  primaryBuffer = nullptr;
  surfaceCapabilities = { 0 };
  surfaceFormat = { VK_FORMAT_UNDEFINED };
  windowSize = { 1280, 720 };
  pass::interface = this;
  activePipeline = Triangle;
}

VulkanInterface::~VulkanInterface(void)
{
  vkDestroySwapchainKHR(globalDevice, _swapChain, nullptr);

  instance.destroySurfaceKHR(surface);
  SDL_DestroyWindow(globalWindow);
  SDL_Quit();
  instance.destroy();
}

void VulkanInterface::Initialize(void)
{
  // Create an SDL window that supports Vulkan rendering.
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "Could not initialize SDL." << std::endl;
    return;
  }


  SDL_Window* window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
  if (window == NULL) {
    std::cout << "Could not create SDL window." << std::endl;
    return;
  }
  globalWindow = window;

  CreateInstance();
  CreateSurface();
  CreatePhysicalDevice();
  CreateDevice();
  CreateMemoryAllocator();
  CreateSwapChain();
  CreateImageView();
  CreateRenderPass();
  CreateFrameBuffer();
  CreateCommandBuffer();
  CreateGraphicsPipeline();
  VkFenceCreateInfo fenceCreate{};
  fenceCreate.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(globalDevice, &fenceCreate, nullptr, &fence);

  VkSemaphoreCreateInfo presentSema{};
  presentSema.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vkCreateSemaphore(globalDevice, &presentSema, nullptr, &presentSemaphore);
  vkCreateSemaphore(globalDevice, &presentSema, nullptr, &imageGet);

}


void add_extension(unsigned int* count, std::vector<const char*>* extensions, const char* newExtension)
{
  extensions->push_back(newExtension);
  if (count)
    *count = static_cast<unsigned int>(extensions->size());
}

void VulkanInterface::CreateInstance(void)
{
  // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
  unsigned extension_count;
  if (!SDL_Vulkan_GetInstanceExtensions(globalWindow, &extension_count, NULL)) {
    std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
    return;
  }
  std::vector<const char*> extensions(extension_count);
  if (!SDL_Vulkan_GetInstanceExtensions(globalWindow, &extension_count, extensions.data())) {
    std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
    return;
  }
  //add_extension(&extension_count, &extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  // Use validation layers if this is a debug build
  std::vector<const char*> layers;
#if defined(_DEBUG)
  layers.push_back("VK_LAYER_KHRONOS_validation");
  layers.push_back("VK_LAYER_RENDERDOC_Capture");
#endif

  // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
  // program, which can be useful for layers and tools to provide more debug information.
  vk::ApplicationInfo appInfo = vk::ApplicationInfo()
    .setPApplicationName("Vulkan Example")
    .setApplicationVersion(1)
    .setPEngineName("Strea Engine A")
    .setEngineVersion(1)
    .setApiVersion(VK_API_VERSION_1_3);

  // vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
  // are needed.
  vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
    .setFlags(vk::InstanceCreateFlags())
    .setPApplicationInfo(&appInfo)
    .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
    .setPpEnabledExtensionNames(extensions.data())
    .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
    .setPpEnabledLayerNames(layers.data());

  // Create the Vulkan instance.
  vk::Instance localinstance;
  try {
    localinstance = vk::createInstance(instInfo);
  }
  catch (const std::exception& e) {
    std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
    return;
  }
  instance = localinstance;

}


void VulkanInterface::CreateSurface(void)
{
  // Create a Vulkan surface for rendering
  VkSurfaceKHR c_surface;
  if (!SDL_Vulkan_CreateSurface(globalWindow, static_cast<VkInstance>(instance), &c_surface)) {
    std::cout << "Could not create a Vulkan surface." << std::endl;
    return;
  }
  surface = vk::SurfaceKHR(c_surface);
}


void VulkanInterface::CreatePhysicalDevice(void)
{
  physicalDevice = VK_NULL_HANDLE;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto& globalDevice : devices) {
    if (isDeviceSuitable(globalDevice)) {
      physicalDevice = globalDevice;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}


void VulkanInterface::CreateDevice(void)
{

  const float priorities[2] = { 0, 1 };
  VkQueueFamilyProperties properites = {};
  properites.queueCount = 1;
  properites.minImageTransferGranularity = { 1,1,1 };
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, &properites);
  VkDeviceQueueCreateInfo* queueCreate = nullptr;
  if (queueCount != 0)
  {
    // This is where most initializtion for a program should be performed
    VkDeviceQueueCreateInfo deviceQueueCreate[2] = { {} };
    deviceQueueCreate[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreate[0].queueFamilyIndex = 0; // Transfer
    deviceQueueCreate[0].queueCount = 1;
    deviceQueueCreate[0].pQueuePriorities = priorities;

    deviceQueueCreate[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreate[1].queueFamilyIndex = queueCount; // Graphics
    deviceQueueCreate[1].queueCount = 1;
    deviceQueueCreate[1].pQueuePriorities = priorities;
    queueCreate = deviceQueueCreate;
  }
  else
  {
    VkDeviceQueueCreateInfo deviceQueueCreate = {};
    deviceQueueCreate.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreate.queueFamilyIndex = queueCount; // Graphics
    deviceQueueCreate.queueCount = 1;
    deviceQueueCreate.pQueuePriorities = priorities;
    queueCreate = &deviceQueueCreate;
  }

  std::vector<const char*> extensions = std::vector<const char*>();
  add_extension(nullptr, &extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  VkDeviceCreateInfo deviceCreate = {};
  deviceCreate.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreate.pQueueCreateInfos = queueCreate;
  deviceCreate.queueCreateInfoCount = (queueCount != 0) ? 2 : 1;
  deviceCreate.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  deviceCreate.ppEnabledExtensionNames = extensions.data();

  vkCreateDevice(physicalDevice, &deviceCreate, nullptr, &globalDevice);

  VkQueue graphicsQueue0 = VK_NULL_HANDLE;
  VkQueue graphicsQueue1 = VK_NULL_HANDLE;
  VkQueue transferQueue0 = VK_NULL_HANDLE;
  if (queueCount != 0)
  {
    // Can be obtained in any order
    vkGetDeviceQueue(globalDevice, 0, 0, &transferQueue0); // family 0 - queue 0
    vkGetDeviceQueue(globalDevice, 1, 1, &graphicsQueue1); // family 1 - queue 1
    vkGetDeviceQueue(globalDevice, 1, 0, &graphicsQueue0); // family 1 - queue 0
    queues.push_back(graphicsQueue0);
    queues.push_back(graphicsQueue1);
    queues.push_back(transferQueue0);

  }
  else
  {
    vkGetDeviceQueue(globalDevice, 0, 0, &graphicsQueue0); // family 1 - queue 0
    queues.push_back(graphicsQueue0);
  }
}

void VulkanInterface::CreateMemoryAllocator(void)
{
  VmaAllocatorCreateInfo allocatorCreateInfo{};
  allocatorCreateInfo.physicalDevice = physicalDevice;
  allocatorCreateInfo.device = globalDevice;
  allocatorCreateInfo.instance = instance;
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
  vmaCreateAllocator(&allocatorCreateInfo, &allocator);

}

void VulkanInterface::CreateRenderPass(void)
{
  VkAttachmentDescription attachments[1] = {}; // 3638
  VkAttachmentReference Attachments[1] = {
    {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
  };
  attachments[0].format = surfaceFormat.format;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  //attachments[1].format = surfaceFormats[0].format;
  //attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  //attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  //attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  //attachments[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  //attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  //attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  //attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  VkSubpassDescription subDescription = {};
  subDescription.colorAttachmentCount = 1;
  subDescription.pColorAttachments = Attachments;
  subDescription.preserveAttachmentCount = 0;
  subDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  std::array<VkSubpassDependency, 1> dependencies{};

  dependencies[0].srcSubpass = 0;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = 0;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  //dependencies[1].srcSubpass = 0;
  //dependencies[1].dstSubpass = 0;
  //dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  //dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  //dependencies[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  //dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  //dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassCreate{};
  renderPassCreate.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreate.subpassCount = 1;
  renderPassCreate.pSubpasses = &subDescription;
  renderPassCreate.pAttachments = attachments;
  renderPassCreate.attachmentCount = 1;
  renderPassCreate.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassCreate.pDependencies = dependencies.data();

  vkCreateRenderPass(globalDevice, &renderPassCreate, nullptr, &currentRenderPass);
}

VkSurfaceFormatKHR VulkanInterface::SelectValidFormat(std::vector<VkSurfaceFormatKHR>& formats)
{
  for (auto format : formats)
  {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return format;
  }
  return formats[0];
}

void VulkanInterface::CreateSwapChain(void)
{
  std::vector<VkSurfaceFormatKHR> surfaceFormats(5);
  uint32_t cout = static_cast<uint32_t>(surfaceFormats.size());
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &cout, surfaceFormats.data());
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  surfaceFormat = SelectValidFormat(surfaceFormats);
  VkSwapchainKHR swapChain;
  VkSwapchainCreateInfoKHR swapCreate{};
  swapCreate.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapCreate.surface = surface;
  swapCreate.minImageCount = surfaceCapabilities.minImageCount;
  swapCreate.imageFormat = surfaceFormat.format;
  swapCreate.imageColorSpace = surfaceFormat.colorSpace;
  swapCreate.imageExtent = surfaceCapabilities.maxImageExtent;
  swapCreate.imageArrayLayers = 1;
  swapCreate.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapCreate.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

  std::array<uint32_t, 1> indicies = { 0 };

  swapCreate.queueFamilyIndexCount = 1;
  swapCreate.pQueueFamilyIndices = indicies.data();
  swapCreate.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapCreate.clipped = VK_TRUE;
  swapCreate.preTransform = surfaceCapabilities.currentTransform;
  swapCreate.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  vkCreateSwapchainKHR(globalDevice, &swapCreate, nullptr, &swapChain);

  _swapChain = swapChain;

  vkGetSwapchainImagesKHR(globalDevice, _swapChain, &swapImageCount, nullptr);

  _swapImages = std::vector<VkImage>(swapImageCount);
  _imageLayouts = std::vector<VkImageLayout>(swapImageCount);
  for (int i = 0; i < _imageLayouts.size(); i++)
  {
    _imageLayouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
  }
  vkGetSwapchainImagesKHR(globalDevice, _swapChain, &swapImageCount, _swapImages.data());

}

void VulkanInterface::CreateFrameBuffer(void)
{
  const size_t size = _swapImageViews.size();
  _buffers.reserve(size);
  for (int i = 0; i < size; ++i)
  {
    VkImageView attachments[] = {
    _swapImageViews[i]
    };
    VkFramebuffer buf;
    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass = currentRenderPass;
    frameBufferCreateInfo.attachmentCount = 1;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = surfaceCapabilities.maxImageExtent.width;
    frameBufferCreateInfo.height = surfaceCapabilities.maxImageExtent.height;
    frameBufferCreateInfo.layers = 1;
    vkCreateFramebuffer(globalDevice, &frameBufferCreateInfo, nullptr, &buf);
    _buffers.push_back(buf);
  }
}

VkImage VulkanInterface::CreateImage(void)
{
  uint32_t queueFamilyInex = 0;
  VkImage image;
  VkImageCreateInfo imageInfoCreate = {}; // 3271
  imageInfoCreate.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfoCreate.imageType = VK_IMAGE_TYPE_2D;
  imageInfoCreate.format = surfaceFormat.format;
  imageInfoCreate.extent = { surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.maxImageExtent.height, 1 };
  imageInfoCreate.queueFamilyIndexCount = 1;
  imageInfoCreate.pQueueFamilyIndices = &queueFamilyInex;
  imageInfoCreate.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfoCreate.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfoCreate.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  imageInfoCreate.mipLevels = 1;
  imageInfoCreate.arrayLayers = 1;

  VmaAllocationCreateInfo vAllocationInfo{}; // 1230
  vAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
  VmaAllocation imageAllocation;
  VmaAllocationInfo imageInfo;

  vmaCreateImage(allocator, &imageInfoCreate, &vAllocationInfo, &image, &imageAllocation, &imageInfo);
  return image;
}

void VulkanInterface::CreateImageView(void)
{
  const size_t size = _swapImages.size();
  _swapImageViews.reserve(size);
  for (int i = 0; i < size; i++)
  {
    VkImageView loc;
    VkImageViewCreateInfo imageCreateInfo = {}; // 3304
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageCreateInfo.image = _swapImages[i];
    imageCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageCreateInfo.format = surfaceFormat.format;
    VkImageSubresourceRange range = {};
    range.layerCount = 1;
    range.baseMipLevel = 0;
    range.levelCount = VK_REMAINING_MIP_LEVELS;
    range.baseArrayLayer = 0;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCreateInfo.subresourceRange = range;
    vkCreateImageView(globalDevice, &imageCreateInfo, nullptr, &loc);
    _swapImageViews.push_back(loc);
  }
}

VkSampler VulkanInterface::CreateSampler(void)
{
  VkSampler sampler;
  VkSamplerCreateInfo sampleCreate{ };
  sampleCreate.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampleCreate.magFilter = VK_FILTER_NEAREST;
  sampleCreate.minFilter = VK_FILTER_NEAREST;
  sampleCreate.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sampleCreate.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  sampleCreate.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  sampleCreate.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  sampleCreate.mipLodBias = 1;
  sampleCreate.anisotropyEnable = VK_FALSE;
  sampleCreate.maxAnisotropy = 1;
  sampleCreate.compareEnable = VK_FALSE;
  sampleCreate.minLod = .25f;
  sampleCreate.maxLod = 1.0f;
  sampleCreate.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
  sampleCreate.unnormalizedCoordinates = VK_FALSE;


  vkCreateSampler(globalDevice, &sampleCreate, nullptr, &sampler);
  return sampler;
}

void VulkanInterface::CreateCommandPool(void)
{
  VkCommandPoolCreateInfo poolInfo =
  {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    0,
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    (queueCount != 0) ? uint32_t(1) : 0 // Graphics
  };

  if (vkCreateCommandPool(globalDevice, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void VulkanInterface::CreateCommandBuffer(void)
{
  VkCommandBuffer CommandBuffer;

  if (pool == VK_NULL_HANDLE)
    CreateCommandPool();

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(globalDevice, &allocInfo, &CommandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
  vkResetCommandBuffer(CommandBuffer, 0);

  primaryBuffer = CommandBuffer;
}

VkShaderModule VulkanInterface::CreateShader(std::string path)
{
  VkShaderModule shaderModule;
  VkShaderModuleCreateInfo shaderCreate{};
  std::vector<char> shaderCode = readShaderFile(path);
  shaderCreate.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
  shaderCreate.codeSize = shaderCode.size();
  shaderCreate.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vkCreateShaderModule(globalDevice, &shaderCreate, nullptr, &shaderModule);
  return shaderModule;
}

VkPipelineShaderStageCreateInfo VulkanInterface::CreateShaderInfo(std::string path, VkShaderStageFlagBits stage)
{
  VkPipelineShaderStageCreateInfo Shader{}; // 3344
  Shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  Shader.stage = stage;
  Shader.flags = 0;
  Shader.module = CreateShader(path);
  Shader.pName = "main";
  return Shader;
}

VkPipelineInputAssemblyStateCreateInfo VulkanInterface::CreateInputAssemblyState(void)
{
  VkPipelineInputAssemblyStateCreateInfo inputState{};
  inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputState.primitiveRestartEnable = VK_FALSE;
  return inputState;
}

VkPipelineViewportStateCreateInfo VulkanInterface::CreateViewPortState(void)
{
  static VkViewport port{ 0,0,
    static_cast<float>(surfaceCapabilities.maxImageExtent.width),
    static_cast<float>(surfaceCapabilities.maxImageExtent.height), 0, 1 };
  static VkRect2D scissor{ {0,0},
    surfaceCapabilities.maxImageExtent };
  VkPipelineViewportStateCreateInfo viewPortState{};
  viewPortState.viewportCount = 1;
  viewPortState.pViewports = &port;
  viewPortState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewPortState.scissorCount = 1;
  viewPortState.pScissors = &scissor;
  return viewPortState;
}

VkPipelineRasterizationStateCreateInfo VulkanInterface::CreateaRasterizationState(void)
{
  VkPipelineRasterizationStateCreateInfo rasterizationCreate{};
  rasterizationCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationCreate.depthClampEnable = VK_FALSE;
  rasterizationCreate.rasterizerDiscardEnable = VK_FALSE;
  rasterizationCreate.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationCreate.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizationCreate.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizationCreate.lineWidth = 1;
  rasterizationCreate.depthBiasConstantFactor = 0.0f;
  rasterizationCreate.depthBiasClamp = 0.0f;
  rasterizationCreate.depthBiasSlopeFactor = 0.0f;

  return rasterizationCreate;
}

VkPipelineMultisampleStateCreateInfo VulkanInterface::CreateMultiSampleInfo(void)
{

  VkPipelineMultisampleStateCreateInfo multiStateCreate{};
  multiStateCreate.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multiStateCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multiStateCreate.sampleShadingEnable = VK_FALSE;
  multiStateCreate.minSampleShading = 1.0f;
  multiStateCreate.pSampleMask = nullptr;
  multiStateCreate.alphaToCoverageEnable = VK_FALSE;
  multiStateCreate.alphaToOneEnable = VK_FALSE;
  return multiStateCreate;
}

VkPipelineColorBlendStateCreateInfo VulkanInterface::CreateColorBlendState(void)
{
  static std::array<VkPipelineColorBlendAttachmentState, 2> colorBlend{};
  colorBlend[0].blendEnable = VK_FALSE;
  colorBlend[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT
    | VK_COLOR_COMPONENT_G_BIT
    | VK_COLOR_COMPONENT_B_BIT
    | VK_COLOR_COMPONENT_A_BIT;
  colorBlend[1] = colorBlend[0];

  // THese two structures ^ v
  VkPipelineColorBlendStateCreateInfo colorBlendCreate{};
  colorBlendCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendCreate.logicOp = VK_LOGIC_OP_AND;
  colorBlendCreate.logicOpEnable = VK_FALSE;
  colorBlendCreate.attachmentCount = 1;
  colorBlendCreate.pAttachments = colorBlend.data();
  colorBlendCreate.blendConstants[0] = 1;
  colorBlendCreate.blendConstants[1] = 1;
  colorBlendCreate.blendConstants[2] = 1;
  colorBlendCreate.blendConstants[3] = 1;

  return colorBlendCreate;
}

VkDescriptorSetLayout VulkanInterface::CreateDescriptorSetLayout(void)
{
  std::array<VkDescriptorSetLayoutBinding, 1> layoutBindings = {};
  layoutBindings[0].binding = uint32_t(0);
  layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  layoutBindings[0].descriptorCount = 0;
  layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayout setLayout;
  VkDescriptorSetLayoutCreateInfo SetCreate{};
  SetCreate.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  SetCreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  SetCreate.pBindings = layoutBindings.data();


  vkCreateDescriptorSetLayout(globalDevice, &SetCreate, nullptr, &setLayout);
  return setLayout;
}

VkDescriptorPool VulkanInterface::CreateDescriptorPool(VkDescriptorSetLayout* setLayout)
{
  VkDescriptorPoolSize psize{};

  psize.descriptorCount = 1;
  psize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  VkDescriptorPool descriptorPool;
  VkDescriptorPoolCreateInfo descriPool{};
  descriPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriPool.maxSets = 10;
  descriPool.poolSizeCount = 1;
  descriPool.pPoolSizes = &psize;

  vkCreateDescriptorPool(globalDevice, &descriPool, nullptr, &descriptorPool);
  return descriptorPool;
}

VkPipelineLayout VulkanInterface::CreatePipelineLayout(VkDescriptorSetLayout* setLayout)
{
  std::array<VkPushConstantRange, 1> constantRanges{};
  constantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  constantRanges[0].size = sizeof(uniformBuffer);
  constantRanges[0].offset = 0;

  VkPipelineLayout layout;
  VkPipelineLayoutCreateInfo layoutCreate{};
  layoutCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutCreate.setLayoutCount = 0;
  layoutCreate.pSetLayouts = setLayout;
  layoutCreate.pPushConstantRanges = constantRanges.data();
  layoutCreate.pushConstantRangeCount = constantRanges.size();
  vkCreatePipelineLayout(globalDevice, &layoutCreate, nullptr, &layout);
  pipelayout = layout;
  return layout;
}

VkPipelineDepthStencilStateCreateInfo VulkanInterface::CreateDepthStencilStat(void)
{
  VkPipelineDepthStencilStateCreateInfo state{};
  state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  state.depthTestEnable = VK_TRUE;
  state.depthWriteEnable = VK_TRUE;
  state.depthCompareOp = VK_COMPARE_OP_GREATER;
  //state.front            = state.back;
  state.back.compareOp = VK_COMPARE_OP_ALWAYS;

  return state;
}

void VulkanInterface::CreateGraphicsPipeline(void)
{
  VkPipeline pipeline = NULL;

  VkPipelineShaderStageCreateInfo shaders[] =
  {
    CreateShaderInfo("./Shaders/frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT),
    CreateShaderInfo("./Shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
  };
  VkPipelineVertexInputStateCreateInfo vertexShader{}; // 3377
  VertexInfo info = Vertex::GetInfo();
  vertexShader.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexShader.pVertexAttributeDescriptions = info.attributes.data();
  vertexShader.vertexAttributeDescriptionCount = static_cast<uint32_t>(info.attributes.size());;
  vertexShader.vertexBindingDescriptionCount = uint32_t(info.bindings.size());
  vertexShader.pVertexBindingDescriptions = info.bindings.data();
  VkPipelineInputAssemblyStateCreateInfo inputState = CreateInputAssemblyState();
  VkPipelineViewportStateCreateInfo viewPortState = CreateViewPortState();
  VkPipelineRasterizationStateCreateInfo rasterizationCreate = CreateaRasterizationState();
  VkPipelineMultisampleStateCreateInfo multiStateCreate = CreateMultiSampleInfo();
  VkPipelineColorBlendStateCreateInfo colorBlendCreate = CreateColorBlendState();
  VkPipelineDepthStencilStateCreateInfo depthStencilCreate = CreateDepthStencilStat();
  VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT ,VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY };
  VkPipelineDynamicStateCreateInfo dynamState{};
  dynamState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamState.pDynamicStates = states;
  dynamState.dynamicStateCount = _countof(states);


  VkDescriptorSetLayout setLayout = CreateDescriptorSetLayout();

  VkGraphicsPipelineCreateInfo pipelineCreate{}; // 3504
  pipelineCreate.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreate.pStages = shaders;
  pipelineCreate.stageCount = 2;
  pipelineCreate.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
  pipelineCreate.pVertexInputState = &vertexShader;
  pipelineCreate.pInputAssemblyState = &inputState;
  pipelineCreate.pViewportState = &viewPortState;
  pipelineCreate.basePipelineHandle = VK_NULL_HANDLE;
  pipelineCreate.basePipelineIndex = -1;
  pipelineCreate.renderPass = currentRenderPass;
  pipelineCreate.subpass = 0;
  pipelineCreate.pRasterizationState = &rasterizationCreate;
  pipelineCreate.pColorBlendState = &colorBlendCreate;
  pipelineCreate.pMultisampleState = &multiStateCreate;
  pipelineCreate.layout = CreatePipelineLayout(&setLayout);
  pipelineCreate.pDepthStencilState = &depthStencilCreate;
  pipelineCreate.pDynamicState = &dynamState;

  vkCreateGraphicsPipelines(globalDevice, VK_NULL_HANDLE, 1, &pipelineCreate, nullptr, &pipeline);
  ParentPipeline = pipeline;

  inputState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  pipelineCreate.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
  pipelineCreate.basePipelineHandle = ParentPipeline;
  vkCreateGraphicsPipelines(globalDevice, VK_NULL_HANDLE, 1, &pipelineCreate, nullptr, &pipeline);
  ActivePipelines[0] = pipeline;

  inputState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  vkCreateGraphicsPipelines(globalDevice, VK_NULL_HANDLE, 1, &pipelineCreate, nullptr, &pipeline);
  ActivePipelines[1] = pipeline;

  //inputState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  vkCreateGraphicsPipelines(globalDevice, VK_NULL_HANDLE, 1, &pipelineCreate, nullptr, &pipeline);
  ActivePipelines[2] = pipeline;
}

void VulkanInterface::BeginRenderPass()
{

  vkWaitForFences(globalDevice, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(globalDevice, 1, &fence);
  //TransitionImage(imageIndex, _imageLayouts[imageIndex], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  ReleaseActiveBuffers();

  vkAcquireNextImageKHR(globalDevice, _swapChain, UINT64_MAX, imageGet, nullptr, &imageIndex);
  vkResetCommandBuffer(primaryBuffer, 0);
  //TransitionImage(imageIndex, _imageLayouts[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.pNext = nullptr;
  cmdBeginInfo.pInheritanceInfo = nullptr;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

  vkBeginCommandBuffer(primaryBuffer, &cmdBeginInfo);

  VkRect2D draw = {
    {0,0},
    surfaceCapabilities.maxImageExtent
  };

  VkClearValue clear[2]{};
  clear[0].color = { 0, 0.5, 0, 1 };
  clear[1].depthStencil = { 1, 0 };
  VkRenderPassBeginInfo beginInfo{};

  beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  beginInfo.pNext = nullptr;
  beginInfo.renderArea = draw;
  beginInfo.renderPass = currentRenderPass;
  beginInfo.framebuffer = _buffers[imageIndex];
  beginInfo.clearValueCount = 2;
  beginInfo.pClearValues = clear;

  vkCmdBeginRenderPass(primaryBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
  VkRect2D scissor = { {0,0}, surfaceCapabilities.maxImageExtent };
  VkViewport port = { 0,0,
    static_cast<float>(surfaceCapabilities.maxImageExtent.width),
    static_cast<float>(surfaceCapabilities.maxImageExtent.height), 0, 1 };

  vkCmdBindPipeline(primaryBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ActivePipelines[activePipeline]);
  vkCmdSetScissor(primaryBuffer, 0, 1, &scissor);
  vkCmdSetViewport(primaryBuffer, 0, 1, &port);
  _isRendering = true;
}

bufferInfo  VulkanInterface::CreateVertexBuffer(int VertexCount)
{
  std::array<uint32_t, 1> indicies = { 0 };

  VmaAllocation bufferAllocation{};

  VkMemoryRequirements memRec{};
  memRec.size = VertexCount * sizeof(Vertex);
  memRec.alignment = 0;

  VmaAllocationCreateInfo allocationInfo{};
  allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  allocationInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
  allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocationInfo AllocInfo;


  VkBuffer vertexBuffer;
  VkBufferCreateInfo bufferCreate{};
  bufferCreate.size = VertexCount * sizeof(Vertex);
  bufferCreate.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferCreate.queueFamilyIndexCount = static_cast<uint32_t>(indicies.size());
  bufferCreate.pQueueFamilyIndices = indicies.data();
  bufferCreate.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreate.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vmaCreateBuffer(allocator, &bufferCreate, &allocationInfo, &vertexBuffer, &bufferAllocation, &AllocInfo);
  activeBuffers.push_back({ vertexBuffer, bufferAllocation, memRec.size });
  return { vertexBuffer, bufferAllocation, memRec.size };
}

void VulkanInterface::DrawRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color)
{
  UpdatePushConstants();
  if (!_isRendering)
    throw std::runtime_error("Cannot draw without a render pass started");
  bufferInfo buffer = CreateVertexBuffer(6);
  void* data = NULL;
  std::array<Vertex, 6> vertexs = {};
  vertexs[0] = { {pos.x - size.x / 2.0f, pos.y - size.y / 2.0f, 1}, color };
  vertexs[1] = { {pos.x - size.x / 2.0f, pos.y + size.y / 2.0f, 1}, color };
  vertexs[2] = { {pos.x + size.x / 2.0f, pos.y + size.y / 2.0f, 1}, color };
  vertexs[3] = { {pos.x + size.x / 2.0f, pos.y + size.y / 2.0f, 1}, color };
  vertexs[4] = { {pos.x + size.x / 2.0f, pos.y - size.y / 2.0f, 1}, color };
  vertexs[5] = { {pos.x - size.x / 2.0f, pos.y - size.y / 2.0f, 1}, color };
  vmaMapMemory(allocator, buffer.memory, &data);
  memcpy(data, vertexs.data(), sizeof(Vertex) * vertexs.size());
  VkBuffer buffers[2] = { buffer.buffer, 0 };
  VkDeviceSize ComBuffOffset = 0;

  vkCmdBindVertexBuffers(primaryBuffer, 0, 1, &buffer.buffer, &ComBuffOffset);
  vkCmdDraw(primaryBuffer, static_cast<uint32_t>(vertexs.size()), 1, 0, 0);
  vmaUnmapMemory(allocator, buffer.memory);
  //for (int i = 0; i < 6; ++i)
  //{
  //  Vertex* v = reinterpret_cast<Vertex*>(data) + i;
  //  DBOUT("[" << v->pos.x << "," << v->pos.y << "," << v->pos.z << "]");
  //  DBOUT(", ");
  //  DBOUT( "[" << v->color.r << "," << v->color.g << "," << v->color.b << "," << v->color.a << "]");
  //  DBOUT(std::endl);
  //}
}

void VulkanInterface::ReleaseVertexBuffer(bufferInfo in)
{
  vmaFreeMemory(allocator, in.memory);
  vkDestroyBuffer(globalDevice, in.buffer, nullptr);

}

void VulkanInterface::ReleaseActiveBuffers(void)
{
  for (int i = 0; i < activeBuffers.size(); i++)
  {
    ReleaseVertexBuffer(activeBuffers[i]);
  }
  activeBuffers.clear();
}

void VulkanInterface::EndRenderPass()
{
  if (!_isRendering)
    throw std::runtime_error("Cannot end submit an unstarted renderpass");

  vkCmdEndRenderPass(primaryBuffer);
  VkSemaphore waitSemas[] = { imageGet };
  VkSemaphore signalSema[] = { presentSemaphore };
  // Submit for draw
  VkSubmitInfo subInfo{};
  std::array<VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  subInfo.commandBufferCount = 1;
  subInfo.pCommandBuffers = &primaryBuffer;
  subInfo.pWaitDstStageMask = waitStages.data();
  subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  subInfo.signalSemaphoreCount = 1;
  subInfo.pSignalSemaphores = signalSema;
  subInfo.pWaitSemaphores = waitSemas;
  subInfo.waitSemaphoreCount = 1;

  vkEndCommandBuffer(primaryBuffer);
  vkQueueSubmit(queues[0], 1, &subInfo, fence);

  VkSwapchainKHR swapChains[] = { _swapChain };
  VkPresentInfoKHR presInfo{};
  presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presInfo.swapchainCount = 1;
  presInfo.pSwapchains = swapChains;
  presInfo.pImageIndices = &imageIndex;
  presInfo.pWaitSemaphores = signalSema;
  presInfo.waitSemaphoreCount = 1;
  VkResult result{};
  presInfo.pResults = &result;
  vkQueuePresentKHR(queues[0], &presInfo);
  ++_frame;
  _isRendering = false;

  UpdatePushConstants();


}

VkCommandBuffer VulkanInterface::CreateSingleBuffer()
{
  VkCommandBuffer CommandBuffer;

  if (pool == VK_NULL_HANDLE)
    CreateCommandPool();

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(globalDevice, &allocInfo, &CommandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.pNext = nullptr;
  cmdBeginInfo.pInheritanceInfo = nullptr;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(CommandBuffer, &cmdBeginInfo);

  return CommandBuffer;
}

void VulkanInterface::EndSingleBuffer(VkCommandBuffer buffer)
{
  vkEndCommandBuffer(buffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &buffer;
  vkQueueSubmit(queues[0], 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queues[0]);

  vkFreeCommandBuffers(globalDevice, pool, 1, &buffer);
}

void VulkanInterface::CommitBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
  VkCommandBuffer commandBuffer = CreateSingleBuffer();
  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = {
      width,
      height,
      1
  };
  vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );
  EndSingleBuffer(commandBuffer);
}

void VulkanInterface::SetActiveCamera(Camera c)
{
  activeCamera = c;
}

void VulkanInterface::Draw(std::vector<Vertex> const& vertexes)
{
  UpdatePushConstants();
  if (!_isRendering)
    throw std::runtime_error("Cannot draw without a render pass started");
  bufferInfo buffer = CreateVertexBuffer(vertexes.size());
  void* data = NULL;
  vmaMapMemory(allocator, buffer.memory, &data);
  memcpy(data, vertexes.data(), sizeof(Vertex) * vertexes.size());
  VkBuffer buffers[2] = { buffer.buffer, 0 };
  VkDeviceSize ComBuffOffset = 0;

  vkCmdBindVertexBuffers(primaryBuffer, 0, 1, &buffer.buffer, &ComBuffOffset);
  vkCmdDraw(primaryBuffer, static_cast<uint32_t>(vertexes.size()), 1, 0, 0);
  vmaUnmapMemory(allocator, buffer.memory);


}

void VulkanInterface::TransitionImage(uint32_t image, VkImageLayout old, VkImageLayout newL)
{
  VkCommandBuffer tempBuffer = CreateSingleBuffer();
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = _swapImages[image];
  barrier.oldLayout = old;
  barrier.newLayout = newL;
  barrier.srcQueueFamilyIndex = 0;
  barrier.dstQueueFamilyIndex = 0;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags srcStage;
  VkPipelineStageFlags dstStage;
  if (newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (newL == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (newL == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  }
  else if (newL == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else {
    throw std::invalid_argument("unsupported layout transition!");
  }
  vkCmdPipelineBarrier(
    tempBuffer,
    srcStage, dstStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );
  _imageLayouts[image] = newL;
  EndSingleBuffer(tempBuffer);
}

void VulkanInterface::UpdatePushConstants(void)
{
  glm::mat4x4 camMat = activeCamera.GetMatrix();
  glm::vec3 camPos = camMat * glm::vec4(0, 0, 0, 1);
  glm::vec3 lookatPos = camMat * glm::vec4(0, 0, 2, 1);
  windowSize = glm::vec2(surfaceCapabilities.currentExtent.width, surfaceCapabilities.currentExtent.width);
  constantBuffer.viewProjection = glm::lookAt(camPos, lookatPos, glm::vec3(0, 1, 0)) * camMat;
  constantBuffer.worldProjection = glm::perspective(activeCamera.fov, windowSize.x / windowSize.y, 1.0f, 1500.0f);
  //constantBuffer.viewProjection  = glm::transpose(constantBuffer.viewProjection);
  //constantBuffer.worldProjection = glm::transpose(constantBuffer.worldProjection);
  vkCmdPushConstants(primaryBuffer, pipelayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uniformBuffer), &constantBuffer);

}

TopoClass getTopologyClass(VkPrimitiveTopology topology)
{
  switch (topology)
  {
  case 0:
    return Point;
  case 1:
  case 2:
    return Line;
  case 3:
  case 4:
  case 5:
    return Triangle;
  default:
    return Triangle;
  }
}

void VulkanInterface::SetTopology(VkPrimitiveTopology topology)
{
  TopoClass clas = getTopologyClass(topology);

  if (clas != activeTopology)
  {
    vkCmdBindPipeline(primaryBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ActivePipelines[clas]);
  }

  vkCmdSetPrimitiveTopology(primaryBuffer, topology);
}
