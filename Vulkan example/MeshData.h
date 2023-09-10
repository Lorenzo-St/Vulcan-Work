#pragma once
#include "Vertex.h"
#include "Vulkan Interface.h"
#include <vector>
class Mesh 
{
public:
  // Default
  Mesh()
  {
    topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    verticies.reserve(3);
  };
  // Size Reservation
  Mesh(int count) 
  {
    topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    verticies.reserve(count);
  }
  // Pass Verticies
  Mesh(std::vector<Vertex> const& v) 
  {
    topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    verticies = std::vector<Vertex>(v);
  }
  // Copy
  Mesh(Mesh const& other) 
  {
    topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    verticies = std::vector<Vertex>(other.verticies);
  }
  // Destructor
  ~Mesh() 
  {
    verticies.clear();
  }
  void AddVertex(Vertex const& vert) 
  {
    verticies.push_back(vert);
  }
  void SetTopology(VkPrimitiveTopology t) { topology = t; };
  void Draw();
private:
  VkPrimitiveTopology topology;
  std::vector<Vertex> verticies;

};