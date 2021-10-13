#pragma once
#ifndef BLEND_SHAPE_H
#define BLEND_SHAPE_H
#include <string>
#include <vector>
#include <memory>
#include "Shape.h"
class BlendShape : public Shape
{
protected:
	std::vector<std::shared_ptr<std::vector<float>>> posBufs;
	std::vector<std::shared_ptr<std::vector<float>>> norBufs;
public:
	BlendShape();
	~BlendShape();
	void addBlendShape(const std::string& blendShapeName);
	virtual void draw() const;
	virtual void init();
	GLuint blendPosID;
	GLuint blendNorID;

};
#endif