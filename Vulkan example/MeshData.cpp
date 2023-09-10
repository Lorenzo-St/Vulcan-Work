#include "MeshData.h"
namespace pass
{
  extern VulkanInterface* interface;
}
void Mesh::Draw() 
{
  pass::interface->SetTopology(topology);
  pass::interface->Draw(verticies);
}
