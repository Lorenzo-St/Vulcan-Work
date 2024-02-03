#pragma once
#include <glm/glm.hpp>
typedef struct Camera
{
  float fov = 90.0f;
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 scale = { 1,1,1 };
  glm::mat4x4 matrix;
  bool dirty = true;
  glm::mat4x4 GetMatrix()
  {
    if (dirty == true)
    {
      matrix = glm::identity<glm::mat4x4>();
      matrix = glm::translate(matrix, position);
      matrix = glm::rotate(matrix, glm::radians(rotation.x), { 0,0,1 });
      matrix = glm::rotate(matrix, glm::radians(rotation.y), { 1,0,0 });
      matrix = glm::rotate(matrix, glm::radians(rotation.z), { 0,1,0 });
      matrix = glm::scale(matrix, scale);
      dirty = false;
    }
    return matrix;
  }

  void MoveVector(glm::vec3 const& vec)
  {
    position += vec;
  }

  void ScaleVector(glm::vec3 const& sca)
  {
    scale += sca;
  }
  void RotateVector(glm::vec3 const& rot)
  {
    rotation += rot;
  }
  void MoveCamera(glm::vec3  const& newPos)
  {
    dirty = true;
    position = newPos;
  }

  void ScaleCamera(glm::vec3  const& newScale)
  {
    dirty = true;
    scale = newScale;
  }

  void RotateCamera(glm::vec3 const& newRot)
  {
    dirty = true;
    rotation = newRot;
  }

}Camera;