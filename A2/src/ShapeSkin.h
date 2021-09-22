#pragma once
#ifndef SHAPESKIN_H
#define SHAPESKIN_H

#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>

#include "Bones.h"

class MatrixStack;
class Program;
class TextureMatrix;

class ShapeSkin
{
public:
	ShapeSkin();
	virtual ~ShapeSkin();
	void setTextureMatrixType(const std::string &meshName);
	void loadMesh(const std::string &meshName);
	void loadAttachment(const std::string &filename);
	void setProgram(std::shared_ptr<Program> p, bool _sendWeightData) { prog = p; sendWeightData = _sendWeightData; }
	void init();
	void update(int k);
	void draw(int k) const;
	void setTextureFilename(const std::string &f) { textureFilename = f; }
	std::string getTextureFilename() const { return textureFilename; }
	std::shared_ptr<TextureMatrix> getTextureMatrix() { return T; }
	void ShapeSkin::bindBones(std::shared_ptr<Bones> _bones);
private:
	bool sendWeightData;
	std::shared_ptr<Program> prog;
	std::shared_ptr<Bones> bones;
	std::vector<unsigned int> elemBuf;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	std::vector<int> boneIndices;
	std::vector<int> nInfluences;
	std::vector<float> skinningWeights;
	GLuint elemBufID;
	GLuint posBufID;
	GLuint norBufID;
	GLuint texBufID;
	std::string textureFilename;
	std::shared_ptr<TextureMatrix> T;
};

#endif
