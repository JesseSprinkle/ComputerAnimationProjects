#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <vector>
#include <memory>
#include "Helpers.h"

class Bones
{
private:
	std::shared_ptr<std::vector<glm::mat4>> tPose;
	std::shared_ptr<std::vector<glm::mat4>> itPose;
	std::vector<std::shared_ptr<std::vector<glm::mat4>>> bones;
	std::vector<std::shared_ptr<std::vector<glm::mat4>>> animationMatrices;


public:
	Bones(std::string fileName);
	std::shared_ptr<std::vector<glm::mat4>> getTPose();
	std::shared_ptr<std::vector<glm::mat4>> getITPose();
	std::shared_ptr<std::vector<glm::mat4>> getBonesAtFrame(int frame);
	std::shared_ptr<std::vector<glm::mat4>> getAnimationMatricesAtFrame(int frame);
	int getFrameCount();
};