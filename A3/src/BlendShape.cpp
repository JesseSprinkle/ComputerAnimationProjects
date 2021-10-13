#include "BlendShape.h"
#include <assert.h>
#include "Program.h"
#include "GLSL.h"
BlendShape::BlendShape() { }
BlendShape::~BlendShape() { }

void BlendShape::addBlendShape(const std::string& blendShapeName)
{
	std::shared_ptr<std::vector<float>> newPosBuf = std::make_shared<std::vector<float>>();
	std::shared_ptr<std::vector<float>> newNorBuf = std::make_shared<std::vector<float>>();
	loadObj(blendShapeName, *(newPosBuf.get()), *(newNorBuf.get()), texBuf, true, false);
	assert(newPosBuf->size() == posBuf.size());
	assert(newNorBuf->size() == norBuf.size());
	for (int i = 0; i < newPosBuf->size(); i++)
	{
		newPosBuf->operator[](i) = newPosBuf->operator[](i) - posBuf[i];
		newNorBuf->operator[](i) = newNorBuf->operator[](i) - norBuf[i];
	}


	posBufs.push_back(newPosBuf);
	norBufs.push_back(newNorBuf);
}

void BlendShape::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_STATIC_DRAW);

	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_STATIC_DRAW);

	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size() * sizeof(float), &texBuf[0], GL_STATIC_DRAW);

	// Send all blend pos to GPU
	glGenBuffers(1, &blendPosID);
	glBindBuffer(GL_ARRAY_BUFFER, blendPosID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float) * posBufs.size(), NULL, GL_STATIC_DRAW);
	for (int i = 0; i < posBufs.size(); i++)
	{
		glBufferSubData(GL_ARRAY_BUFFER, i * posBuf.size() * sizeof(float), posBuf.size() * sizeof(float), &(posBufs[i]->operator[](0)));
	}

	// Send all blend normals to GPU
	glGenBuffers(1, &blendNorID);
	glBindBuffer(GL_ARRAY_BUFFER, blendNorID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float)* norBufs.size(), NULL, GL_STATIC_DRAW);
	for (int i = 0; i < norBufs.size(); i++)
	{
		glBufferSubData(GL_ARRAY_BUFFER, i * norBuf.size() * sizeof(float), norBuf.size() * sizeof(float), &(norBufs[i]->operator[](0)));
	}

	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

void BlendShape::draw() const
{

	assert(prog);

	glUniform1f(prog->getUniform("numBlendShapes"), (float)posBufs.size());

	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, blendPosID);
	for (int i = 0; i < posBufs.size(); i++)
	{
		int h_blend_pos = prog->getAttribute("dBlendPos" + std::to_string(i));
		glEnableVertexAttribArray(h_blend_pos);
		glVertexAttribPointer(h_blend_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)(i * posBuf.size() * sizeof(float)));
	}

	glBindBuffer(GL_ARRAY_BUFFER, blendNorID);
	
	for (int i = 0; i < norBufs.size(); i++)
	{
		int h_blend_nor = prog->getAttribute("dBlendNor" + std::to_string(i));
		glEnableVertexAttribArray(h_blend_nor);
		glVertexAttribPointer(h_blend_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)(i * norBuf.size() * sizeof(float)));
	}

	// Draw
	int count = posBuf.size() / 3; // number of indices to be rendered
	glDrawArrays(GL_TRIANGLES, 0, count);

	glDisableVertexAttribArray(h_tex);
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	for (int i = 0; i < posBufs.size(); i++)
	{
		glDisableVertexAttribArray(prog->getAttribute("dBlendPos" + std::to_string(i)));
		glDisableVertexAttribArray(prog->getAttribute("dBlendNor" + std::to_string(i)));
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}