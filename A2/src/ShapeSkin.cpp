#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "ShapeSkin.h"
#include "GLSL.h"
#include "Program.h"
#include "TextureMatrix.h"
#include "Helpers.h"

using namespace std;
using namespace glm;

ShapeSkin::ShapeSkin() :
	prog(NULL),
	elemBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0),
	sendWeightData(false)
{
	T = make_shared<TextureMatrix>();
}

ShapeSkin::~ShapeSkin()
{
}

void ShapeSkin::setTextureMatrixType(const std::string &meshName)
{
	T->setType(meshName);
}

void ShapeSkin::loadMesh(const string &meshName)
{
	// Load geometry
	// This works only if the OBJ file has the same indices for v/n/t.
	// In other words, the 'f' lines must look like:
	// f 70/70/70 41/41/41 67/67/67
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = attrib.vertices;
		norBuf = attrib.normals;
		texBuf = attrib.texcoords;
		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t &mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for(size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
					elemBuf.push_back(idx.vertex_index);
				}
				index_offset += fv;
				// per-face material (IGNORE)
				shapes[s].mesh.material_ids[f];
			}
		}
	}
}

void ShapeSkin::loadAttachment(const std::string &filename)
{
	ifstream in;
	in.open(filename);
	if (!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}

	// read base pose
	string line = getNextValidLine(in);
	stringstream ss(line);
	int vertCount, boneCount, maxWeights;
	ss >> vertCount >> boneCount >> maxWeights;

	// set maxWeights so we can send data to gpu easier, define vectors of data
	maxWeights = 12;
	boneIndices = vector<int>(vertCount * maxWeights);
	skinningWeights = vector<float>(vertCount * maxWeights);
	nInfluences = vector<int>(vertCount);
	for (int i = 0; i < vertCount; ++i)
	{
		// for each vertice, read in the weights that affect it
		line = getNextValidLine(in);
		ss = stringstream(line);

		int nWeights;
		ss >> nWeights;
		nInfluences[i] = nWeights;
		int j = 0;
		while (j < nWeights)
		{
			ss >> boneIndices[i * maxWeights + j] >> skinningWeights[i * maxWeights + j];
			j++;
		}

		while (j < maxWeights)
		{
			boneIndices[i * maxWeights + j] = 0;
			skinningWeights[i * maxWeights + j] = 0.0f;
			j++;
		}
	}

	in.close();
}

// if we switch from cpu to gpu, we need to load the initial positions into the gpu.
// if we dont, we will have cpu calculated points in the gpu
void ShapeSkin::reloadVertices()
{
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_DYNAMIC_DRAW);
}

void ShapeSkin::bindBones(shared_ptr<Bones> _bones)
{
	bones = _bones;
}

void ShapeSkin::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	
	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);

	// Send weight data
	glGenBuffers(1, &weightID);
	glBindBuffer(GL_ARRAY_BUFFER, weightID);
	glBufferData(GL_ARRAY_BUFFER, skinningWeights.size() * sizeof(float), &skinningWeights[0], GL_STATIC_DRAW);

	// Send bone data
	glGenBuffers(1, &bonesID);
	glBindBuffer(GL_ARRAY_BUFFER, bonesID);
	glBufferData(GL_ARRAY_BUFFER, boneIndices.size() * sizeof(int), &boneIndices[0], GL_STATIC_DRAW);

	// Send number of weights
	glGenBuffers(1, &inflID);
	glBindBuffer(GL_ARRAY_BUFFER, inflID);
	glBufferData(GL_ARRAY_BUFFER, nInfluences.size() * sizeof(int), &nInfluences[0], GL_STATIC_DRAW);
	
	// Send the element array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::update(int k)
{
	// create new position and normal buffer
	vector<float> posBuf2(posBuf.size());
	vector<float> norBuf2(norBuf.size());

	// for each vertex, calculate the new position
	unsigned int numVerts = posBuf.size() / 3;
	unsigned int maxWeights = boneIndices.size() / numVerts;
	shared_ptr<vector<glm::mat4>> animMats = bones->getAnimationMatricesAtFrame(k);
	for (int i = 0; i < numVerts; ++i)
	{
		// start with init pos and result, sum w * A * x
		glm::vec4 initPos = glm::vec4(posBuf[i * 3], posBuf[i * 3 + 1], posBuf[i * 3 + 2], 1.0f);
		glm::vec4 initNor = glm::vec4(norBuf[i * 3], norBuf[i * 3 + 1], norBuf[i * 3 + 2], 0.0f);
		glm::vec4 resultPos(0);
		glm::vec4 resultNor(0);
		for (int j = i * maxWeights; j < (i + 1) * maxWeights; ++j)
		{
			// for efficiency, if we are greater than the number of points that influence this point, stop adding 0s
			if (j - (i * maxWeights) >= nInfluences[i])
			{
				break;
			}
			resultPos += skinningWeights[j] * (animMats->operator[](boneIndices[j]) * initPos);
			resultNor += skinningWeights[j] * (animMats->operator[](boneIndices[j]) * initNor);
		}
		// set new position for vertex, dir for norm
		posBuf2[i * 3] = resultPos.x;
		posBuf2[i * 3 + 1] = resultPos.y;
		posBuf2[i * 3 + 2] = resultPos.z;
		norBuf2[i * 3] = resultNor.x;
		norBuf2[i * 3 + 1] = resultNor.y;
		norBuf2[i * 3 + 2] = resultNor.z;
	}

	// send updated data to gpu
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf2.size() * sizeof(float), &posBuf2[0], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf2.size() * sizeof(float), &norBuf2[0], GL_DYNAMIC_DRAW);
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::draw(int k) const
{
	assert(prog);

	// Send texture matrix
	glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T->getMatrix()));
	
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// if we should send weight data to gpu
	if (sendWeightData)
	{
		int w0 = prog->getAttribute("w0");
		int w1 = prog->getAttribute("w1");
		int w2 = prog->getAttribute("w2");
		int b0 = prog->getAttribute("b0");
		int b1 = prog->getAttribute("b1");
		int b2 = prog->getAttribute("b2");
		int nInfl = prog->getAttribute("numInfl");
		unsigned stride = 12 * sizeof(float);

		// send weights
		glEnableVertexAttribArray(w0);
		glEnableVertexAttribArray(w1);
		glEnableVertexAttribArray(w2);
		glBindBuffer(GL_ARRAY_BUFFER, weightID);
		glVertexAttribPointer(w0, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(0 * sizeof(float)));
		glVertexAttribPointer(w1, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(4 * sizeof(float)));
		glVertexAttribPointer(w2, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(8 * sizeof(float)));

		// send bone indices
		glEnableVertexAttribArray(b0);
		glEnableVertexAttribArray(b1);
		glEnableVertexAttribArray(b2);
		glBindBuffer(GL_ARRAY_BUFFER, bonesID);
		glVertexAttribPointer(b0, 4, GL_INT, GL_FALSE, stride, (const void*)(0 * sizeof(int)));
		glVertexAttribPointer(b1, 4, GL_INT, GL_FALSE, stride, (const void*)(4 * sizeof(int)));
		glVertexAttribPointer(b2, 4, GL_INT, GL_FALSE, stride, (const void*)(8 * sizeof(int)));

		// send count of weights
		glEnableVertexAttribArray(nInfl);
		glBindBuffer(GL_ARRAY_BUFFER, inflID);
		glVertexAttribPointer(nInfl, 1, GL_INT, GL_FALSE, 0, (const void*)(0 * sizeof(float)));
	}
	
	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}
