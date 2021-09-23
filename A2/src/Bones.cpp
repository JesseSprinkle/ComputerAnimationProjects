#include "Bones.h"
using namespace std;

std::shared_ptr<std::vector<glm::mat4>> processLine(stringstream& ss, int boneCount);

Bones::Bones(string filename)
{
	ifstream in;
	in.open(filename);
	if (!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}

	string line = getNextValidLine(in);
	stringstream ss(line);
	int boneCount, frameCount;
	ss >> frameCount >> boneCount;

	line = getNextValidLine(in);
	ss = stringstream(line);
	tPose = processLine(ss, boneCount);

	// read bone frames
	for (int i = 0; i < frameCount; ++i)
	{
		line = getNextValidLine(in);
		ss = stringstream(line);
		bones.push_back(processLine(ss, boneCount));
	}

	// create inverse tpose and multiplied matrices in constructor, store for later. more efficient
	itPose = make_shared<vector<glm::mat4>>(boneCount);
	for (int i = 0; i < boneCount; ++i)
	{
		itPose->operator[](i) = glm::inverse(tPose->operator[](i));
	}

	in.close();
}

// from a string, reads and stores all the bone matrices in a vector
std::shared_ptr<std::vector<glm::mat4>> processLine(stringstream& ss, int boneCount)
{
	shared_ptr<vector<glm::mat4>> v = make_shared<vector<glm::mat4>>(boneCount);
	for (int j = 0; j < boneCount; j++)
	{
		float x, y, z, w, px, py, pz;
		ss >> x >> y >> z >> w >> px >> py >> pz;
		glm::quat q(w, x, y, z);
		glm::vec4 p(px, py, pz, 1.0f);
		glm::mat4 m;
		m = glm::mat4_cast(q);
		m[3] = p;
		v->operator[](j) = m;
	}
	return v;
}

int Bones::getFrameCount()
{
	return bones.size();
}

std::shared_ptr<std::vector<glm::mat4>> Bones::getTPose()
{
	return tPose;
}

std::shared_ptr<std::vector<glm::mat4>> Bones::getITPose()
{
	return itPose;
}

shared_ptr<vector<glm::mat4>> Bones::getBonesAtFrame(int frame)
{
	return bones[frame];
}

shared_ptr<vector<glm::mat4>> Bones::getAnimationMatricesAtFrame(int frame)
{
	shared_ptr<vector<glm::mat4>> toReturn = make_shared<vector<glm::mat4>>(tPose->size());
	for (int i = 0; i < tPose->size(); i++)
	{
		toReturn->operator[](i) = bones[frame]->operator[](i) * itPose->operator[](i);
	}
	return toReturn;
}