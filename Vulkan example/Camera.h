#pragma once
#include <glm/glm.hpp>
typedef struct Camera 
{
  float fov = 90.0f;
  glm::vec3 position;
  glm::vec3 rotation;
  glm::mat4x4 matrix;
  bool dirty = true;
  glm::mat4x4 GetMatrix()
  {
    if(dirty)
    {
      glm::mat4x4 trans = glm::translate(glm::identity<glm::mat4x4>(), position);
      glm::mat4x4 Xrot = glm::rotate(trans, glm::radians(rotation.x), { 1,0,0 });
      glm::mat4x4 Yrot = glm::rotate(Xrot,  glm::radians(rotation.y), { 0,1,0 });
      glm::mat4x4 Zrot = glm::rotate(Yrot,  glm::radians(rotation.z), { 0,0,1 });
      matrix = Zrot;
      dirty = false;
    }
    return matrix;
  }
}Camera;