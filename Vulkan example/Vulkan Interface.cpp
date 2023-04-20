#include "Vulkan Interface.h"
#include <iostream>
#include <iomanip>

VulkanInterface::VulkanInterface(void) 
{
  _swapChain = 0;
  allocator = 0;
  currentRenderPass = 0;
  globalDevice = 0;
  globalWindow = 0;
  physicalDevice = 0;
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

  CreateRenderPass();
  CreateFrameBuffer();


  VkCommandPool comPool = CreateCommandPool();
  VkCommandBuffer commBuffer = CreateCommandBuffer(comPool);
  VkPipeline currentPipeline = CreateGraphicsPipeline();
}


void add_extension(unsigned int* count, std::vector<const char*>* extensions, const char* newExtension)
{
  extensions->push_back(newExtension);
  if(count)
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
#endif

  // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
  // program, which can be useful for layers and tools to provide more debug information.
  vk::ApplicationInfo appInfo = vk::ApplicationInfo()
    .setPApplicationName("Vulkan C++ Windowed Program Template")
    .setApplicationVersion(1)
    .setPEngineName("LunarG SDK")
    .setEngineVersion(1)
    .setApiVersion(VK_API_VERSION_1_0);

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

  }
  else
  {
    vkGetDeviceQueue(globalDevice, 0, 0, &graphicsQueue0); // family 1 - queue 0
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
  VkAttachmentReference Attachments[1] = { {0, VK_IMAGE_LAYOUT_GENERAL }
  };
  attachments[0].format = VK_FORMAT_R8G8B8A8_UINT;
  attachments[0].samples = VK_SAMPLE_COUNT_16_BIT;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_GENERAL;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkSubpassDescription subDescription = {};
  subDescription.colorAttachmentCount = 1;
  subDescription.pColorAttachments = Attachments;
  subDescription.preserveAttachmentCount = 0;

  VkRenderPassCreateInfo renderPassCreate{};
  renderPassCreate.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreate.subpassCount = 1;
  renderPassCreate.pSubpasses = &subDescription;
  renderPassCreate.pAttachments = attachments;
  renderPassCreate.attachmentCount = 1;

  vkCreateRenderPass(globalDevice, &renderPassCreate, nullptr, &currentRenderPass);
}

void VulkanInterface::CreateSwapChain(void)
{
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  std::array<VkSurfaceFormatKHR,1> surfaceFormats;
  uint32_t cout = static_cast<uint32_t>(surfaceFormats.size());
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &cout, surfaceFormats.data());
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  VkSwapchainKHR swapChain;
  VkSwapchainCreateInfoKHR swapCreate{};
  swapCreate.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapCreate.surface = surface;
  swapCreate.minImageCount = surfaceCapabilities.minImageCount;
  swapCreate.imageFormat = surfaceFormats[0].format;
  swapCreate.imageColorSpace = surfaceFormats[0].colorSpace;
  swapCreate.imageExtent = { 1280, 720 };
  swapCreate.imageArrayLayers = 1;
  swapCreate.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapCreate.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  std::array<uint32_t, 3> indicies = { 0, 1 };
  swapCreate.queueFamilyIndexCount = 2;
  swapCreate.pQueueFamilyIndices = indicies.data();
  swapCreate.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
  swapCreate.clipped = VK_TRUE;
  swapCreate.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  swapCreate.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  vkCreateSwapchainKHR(globalDevice, &swapCreate, nullptr, &swapChain);
  _swapChain = swapChain;
  vkGetSwapchainImagesKHR(globalDevice, _swapChain, &swapImageCount, nullptr);
  _swapImages = std::vector<VkImage>(swapImageCount);
  vkGetSwapchainImagesKHR(globalDevice, _swapChain, &swapImageCount, _swapImages.data());

}


void VulkanInterface::CreateFrameBuffer(void)
{
  VkImageView imageView = CreateImageView();

  VkFramebuffer frameBuffer;
  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.renderPass = currentRenderPass;
  frameBufferCreateInfo.attachmentCount = 1;
  frameBufferCreateInfo.pAttachments = &imageView;
  frameBufferCreateInfo.width = 1280;
  frameBufferCreateInfo.height = 720;
  frameBufferCreateInfo.layers = 1;
  vkCreateFramebuffer(globalDevice, &frameBufferCreateInfo, nullptr, &frameBuffer);



}

VkImage VulkanInterface::CreateImage(void) 
{
  uint32_t queueFamilyInex = 1;
  VkImage image;
  VkImageCreateInfo imageInfoCreate = {}; // 3271
  imageInfoCreate.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfoCreate.imageType = VK_IMAGE_TYPE_2D;
  imageInfoCreate.format = VK_FORMAT_R8G8B8A8_UINT;
  imageInfoCreate.extent = { 1280, 720, 1 };
  imageInfoCreate.queueFamilyIndexCount = 1;
  imageInfoCreate.pQueueFamilyIndices = &queueFamilyInex;
  imageInfoCreate.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfoCreate.samples = VK_SAMPLE_COUNT_16_BIT;
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

VkImageView VulkanInterface::CreateImageView(void) 
{
  VkImageView imageView;
  VkImageViewCreateInfo imageCreateInfo = {}; // 3304
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageCreateInfo.image = CreateImage();
  imageCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UINT;
  VkImageSubresourceRange range = {};
  range.layerCount = 1;
  range.baseMipLevel = 0;
  range.levelCount = VK_REMAINING_MIP_LEVELS;
  range.baseArrayLayer = 0;
  range.aspectMask = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
  imageCreateInfo.subresourceRange = range;
  vkCreateImageView(globalDevice, &imageCreateInfo, nullptr, &imageView);
  return imageView;
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

VkCommandPool VulkanInterface::CreateCommandPool(void)
{
  VkCommandPool pool;
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

  return pool;
}

VkCommandBuffer VulkanInterface::CreateCommandBuffer(VkCommandPool pool)
{
  VkCommandBuffer CommandBuffer;

  if (pool == VK_NULL_HANDLE)
    pool = CreateCommandPool();
  
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(globalDevice, &allocInfo, &CommandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  return CommandBuffer;
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
  VkPipelineShaderStageCreateInfo fragmentShader{}; // 3344
  fragmentShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragmentShader.stage = stage;
  fragmentShader.flags = VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT;
  fragmentShader.module = CreateShader(path);
  fragmentShader.pName = "main";
  return fragmentShader;
}

VkPipelineVertexInputStateCreateInfo VulkanInterface::CreateVertexInputState(void) 
{

  VkVertexInputBindingDescription vertexDesc{};
  vertexDesc.binding = 0;
  vertexDesc.stride = sizeof(Vertex);
  vertexDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


  static std::array<VkVertexInputAttributeDescription, 2> vertexAtrb = {};
  vertexAtrb[0].binding = 0;
  vertexAtrb[0].location = 0;
  vertexAtrb[0].format = VK_FORMAT_R16G16B16_SFLOAT;
  vertexAtrb[0].offset = offsetof(Vertex, pos);

  vertexAtrb[1].binding = 0;
  vertexAtrb[1].location = 1;
  vertexAtrb[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
  vertexAtrb[1].offset = offsetof(Vertex, color);

  VkPipelineVertexInputStateCreateInfo vertexShader{}; // 3377
  vertexShader.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexShader.pVertexAttributeDescriptions = vertexAtrb.data();
  vertexShader.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAtrb.size());;
  vertexShader.vertexBindingDescriptionCount = 1;
  vertexShader.pVertexBindingDescriptions = &vertexDesc;
  return vertexShader;
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
  static VkViewport port{ 0,0, 1280, 720, 0, 1 };
  VkRect2D scissor{ {0,0},{1280, 720} };
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
  rasterizationCreate.depthClampEnable = false;
  rasterizationCreate.rasterizerDiscardEnable = false;
  rasterizationCreate.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationCreate.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizationCreate.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizationCreate.lineWidth = 1;

  return rasterizationCreate;
}

VkPipelineMultisampleStateCreateInfo VulkanInterface::CreateMultiSampleInfo(void) 
{

  VkPipelineMultisampleStateCreateInfo multiStateCreate{};
  multiStateCreate.rasterizationSamples = VK_SAMPLE_COUNT_16_BIT;
  multiStateCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multiStateCreate.sampleShadingEnable = false;
  multiStateCreate.alphaToCoverageEnable = true;
  return multiStateCreate;
}

VkPipelineColorBlendStateCreateInfo VulkanInterface::CreateColorBlendState(void) 
{
  VkPipelineColorBlendAttachmentState colorBlend{};
  colorBlend.blendEnable = false;
  colorBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
  colorBlend.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
  colorBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
  colorBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlend.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlend.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT;


  // THese two structures ^ v
  VkPipelineColorBlendStateCreateInfo colorBlendCreate{};
  colorBlendCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendCreate.logicOp = VK_LOGIC_OP_CLEAR;
  colorBlendCreate.logicOpEnable = false;
  colorBlendCreate.attachmentCount = 1;
  colorBlendCreate.pAttachments = &colorBlend;
  return colorBlendCreate;
}

VkDescriptorSetLayout VulkanInterface::CreateDescriptorSetLayout(void)
{
  VkSampler sampler = CreateSampler();


  VkDescriptorSetLayoutBinding layoutBinding{};
  layoutBinding.binding = uint32_t(1);
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  layoutBinding.descriptorCount = 1;
  layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  layoutBinding.pImmutableSamplers = &sampler;


  VkDescriptorSetLayout setLayout;
  VkDescriptorSetLayoutCreateInfo SetCreate{};
  SetCreate.bindingCount = 1;
  SetCreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  SetCreate.bindingCount = 1;
  SetCreate.pBindings = &layoutBinding;

  vkCreateDescriptorSetLayout(globalDevice, &SetCreate, nullptr, &setLayout);
  return setLayout;
}

VkDescriptorPool VulkanInterface::CreateDescriptorPool(VkDescriptorSetLayout setLayout)
{
  VkDescriptorPoolSize psize{};
  psize.descriptorCount = 1;
  psize.type = VK_DESCRIPTOR_TYPE_SAMPLER;

  VkDescriptorPool descriptorPool;
  VkDescriptorPoolCreateInfo descriPool{};
  descriPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriPool.maxSets = 1;
  descriPool.poolSizeCount = 1;
  descriPool.pPoolSizes = &psize;

  vkCreateDescriptorPool(globalDevice, &descriPool, nullptr, &descriptorPool);
  return descriptorPool;
}

VkPipelineLayout VulkanInterface::CreatePipelineLayout(VkDescriptorSetLayout setLayout)
{
  VkPushConstantRange constantRange{};
  constantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  constantRange.size = 16;

  VkPipelineLayout layout;
  VkPipelineLayoutCreateInfo layoutCreate{};
  layoutCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutCreate.setLayoutCount = 1;
  layoutCreate.pSetLayouts = &setLayout;
  layoutCreate.pPushConstantRanges = &constantRange;
  layoutCreate.pushConstantRangeCount = 1;
  vkCreatePipelineLayout(globalDevice, &layoutCreate, nullptr, &layout);
  return layout;
}

VkPipeline VulkanInterface::CreateGraphicsPipeline(void)
{
  VkPipeline pipeline = NULL;

  VkPipelineShaderStageCreateInfo shaders[] = 
  { 
    CreateShaderInfo("./Shaders/frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT), 
    CreateShaderInfo("./Shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
  };
  VkPipelineVertexInputStateCreateInfo vertexShader = CreateVertexInputState();
  VkPipelineInputAssemblyStateCreateInfo inputState = CreateInputAssemblyState();
  VkPipelineViewportStateCreateInfo viewPortState = CreateViewPortState();
  VkPipelineRasterizationStateCreateInfo rasterizationCreate = CreateaRasterizationState();
  VkPipelineMultisampleStateCreateInfo multiStateCreate = CreateMultiSampleInfo();
  VkPipelineColorBlendStateCreateInfo colorBlendCreate = CreateColorBlendState();

  VkDescriptorSetLayout setLayout = CreateDescriptorSetLayout();
  VkDescriptorPool descriptorPool = CreateDescriptorPool(setLayout);






  VkGraphicsPipelineCreateInfo pipelineCreate{}; // 3504
  pipelineCreate.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreate.pStages = shaders;
  pipelineCreate.stageCount = 2;
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
  pipelineCreate.layout = CreatePipelineLayout(setLayout);



  VkDescriptorSetAllocateInfo descSetAlloca{};
  descSetAlloca.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descSetAlloca.descriptorPool = descriptorPool;
  descSetAlloca.descriptorSetCount = uint32_t(1);
  descSetAlloca.pSetLayouts = &setLayout;



  vkCreateGraphicsPipelines(globalDevice, VK_NULL_HANDLE, 1, &pipelineCreate, nullptr, &pipeline);
  return pipeline;
}

void VulkanInterface::BeginRenderPass(VkCommandBuffer buffer, VkPipeline pipeline)
{

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.pNext = nullptr;
  cmdBeginInfo.pInheritanceInfo = nullptr;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(buffer, &cmdBeginInfo);
  vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  VkRect2D draw = {
    {0,0},
    {100,100}
  };
  VkRenderPassBeginInfo beginInfo{};

  beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  beginInfo.pNext = nullptr;
  beginInfo.renderArea = draw;
  beginInfo.renderPass = currentRenderPass;
  beginInfo.framebuffer = _buffers.data()[0];

  vkCmdBeginRenderPass(buffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);



}

VkBuffer VulkanInterface::CreateVertexBuffer(int VertexCount) 
{
  std::array<uint32_t, 1> indicies = { 0 };

  VmaAllocation bufferAllocation{};

  VkMemoryRequirements memRec{};
  memRec.size = VertexCount * sizeof(Vertex);
  memRec.alignment = 0;

  VmaAllocationCreateInfo allocationInfo{};
  allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
  VmaAllocationInfo AllocInfo;


  VkBuffer vertexBuffer;
  VkBufferCreateInfo bufferCreate{};
  bufferCreate.size = 4 * sizeof(Vertex);
  bufferCreate.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferCreate.queueFamilyIndexCount = static_cast<uint32_t>(indicies.size());
  bufferCreate.pQueueFamilyIndices = indicies.data();
  bufferCreate.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vmaCreateBuffer(allocator, &bufferCreate, &allocationInfo, &vertexBuffer, &bufferAllocation, &AllocInfo);
  return vertexBuffer;
}
