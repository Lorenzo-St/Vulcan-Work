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
    
    CalculateNormals();
  }
  // Copy
  Mesh(Mesh const& other) 
  {
    topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    verticies = std::vector<Vertex>(other.verticies);

    CalculateNormals();
  }
  void CalculateNormals() 
  {
    for (size_t i = 0; i < verticies.size() - 2; i += 3)
    {
      Vertex& in1 = verticies[i];
      Vertex& in2 = verticies[i + 1];
      Vertex& in3 = verticies[i + 2];

      glm::vec3 pToq = in2.pos - in1.pos;
      glm::vec3 pTor = in3.pos - in1.pos;

      in1.normal = glm::vec4(glm::cross(pToq, pTor), 0);
      in2.normal = in1.normal;
      in3.normal = in1.normal;
    }
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