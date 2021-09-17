#ifndef KEYFRAME_H
#define KEYFRAME_H
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
class Keyframe
{
public:
	glm::vec3 position;
	glm::quat rotation;
	Keyframe(glm::vec3 _position, glm::quat _rotation)
	{
		position = _position;
		rotation = _rotation;
	}
};
#endif