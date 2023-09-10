#include "Vertex.h"


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