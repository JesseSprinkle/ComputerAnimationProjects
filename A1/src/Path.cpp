#include "Path.h"
#include <iostream>
Path::Path(std::vector<Keyframe> _keyframes, std::shared_ptr<Helicopter> _heli)
{
	keyframes = _keyframes;
	heli = _heli;
	B[0] = glm::vec4(0, 2, 0, 0);
	B[1] = glm::vec4(-1, 0, 1, 0);
	B[2] = glm::vec4(2, -5, 4, -1);
	B[3] = glm::vec4(-1, 3, -3, 1);
	B = 0.5f * B;
}

void Path::drawHelis(std::shared_ptr<MatrixStack> MV, float t, std::shared_ptr<Program> prog)
{
	for (int i = 0; i < keyframes.size(); i++)
	{
		heli->setPosition(keyframes[i].position);
		heli->setRotation(keyframes[i].rotation);
		heli->draw(MV, t, prog);
	}
}

void Path::drawLines(glm::mat4& G)
{
	glColor3f(1.0f, 0.5f, 0.5f);
	glBegin(GL_LINE_STRIP);
	for (float u = 0; u <= 1.05; u += .1)
	{
		glm::vec4 uv = glm::vec4(1, u, u * u, u * u * u);
		glm::vec3 p = G * (B * uv);
		glVertex3f(p.x, p.y, p.z);
	}
	glEnd();	
}

void Path::drawCurve()
{
	if (keyframes.size() >= 4)
	{
		for (int i = 0; i < keyframes.size() - 1; i++)
		{
			glm::mat4 G = getGPositionMatrix(i);
			drawLines(G);
		}
	}
}

void Path::addKeyframe(Keyframe _keyframe)
{
	keyframes.push_back(_keyframe);
}

glm::mat4 Path::getGPositionMatrix(int curr_seg)
{
	// have to deal with double up
	// if curr_seg == 0, we on first segment, curr_seg == 1, second segment, etc
	// except on edge cases, g = curr_seg - 1, curr_seg, curr_seg + 1, curr_seg + 2
	glm::mat4 G;
	if (curr_seg == 0)
	{
		G[0] = glm::vec4(keyframes[curr_seg].position, 0);
		G[1] = glm::vec4(keyframes[curr_seg].position, 0);
		G[2] = glm::vec4(keyframes[curr_seg + 1].position, 0);
		G[3] = glm::vec4(keyframes[curr_seg + 2].position, 0);
	}
	else if (curr_seg == keyframes.size() - 2)
	{
		G[0] = glm::vec4(keyframes[curr_seg - 1].position, 0);
		G[1] = glm::vec4(keyframes[curr_seg].position, 0);
		G[2] = glm::vec4(keyframes[curr_seg + 1].position, 0);
		G[3] = glm::vec4(keyframes[curr_seg + 1].position, 0);
	}
	else
	{
		G[0] = glm::vec4(keyframes[curr_seg - 1].position, 0);
		G[1] = glm::vec4(keyframes[curr_seg].position, 0);
		G[2] = glm::vec4(keyframes[curr_seg + 1].position, 0);
		G[3] = glm::vec4(keyframes[curr_seg + 2].position, 0);
	}

	return G;
}

glm::vec3 Path::getCurrentPosition(float u)
{
	u = std::fmod(u, keyframes.size() - 1);
	if (keyframes.size() >= 4)
	{
		float curr_frame;
		u = std::modf(u, &curr_frame);
		int curr_seg = (int)(curr_frame + 0.01);

		glm::mat4 G = getGPositionMatrix(curr_seg);
		glm::vec4 uv = glm::vec4(1, u, u * u, u * u * u);
		return glm::vec3(G * (B * uv));

	}
	return glm::vec3(0);

}


glm::quat Path::getCurrentRotation(float u)
{
	if (keyframes.size() >= 2)
	{
		u = fmod(u, keyframes.size() - 1);

		float curr_frame;
		u = std::modf(u, &curr_frame);
		int curr_seg = (int)(curr_frame + 0.01);
		glm::quat q0 = keyframes[curr_seg].rotation;
		glm::quat q1 = keyframes[(curr_seg + 1) % keyframes.size()].rotation;
		if (glm::dot(q0, q1) < 0)
		{
			q1 = -q1;
		}

		// uhh instructions say to do some weird stuff, I'm just going to do what seems intuitive to me
		// I think I designed mine completely differently than we were expected to
		return glm::normalize((1 - u) * q0 + u * q1);
	}
	return glm::angleAxis(0.0f, glm::vec3(1, 0, 0));
}


std::vector<std::pair<float, float>> Path::createParameterizationTable()
{
	std::vector<std::pair<float, float>> usTable;
	glm::mat4 G;
	usTable.push_back(std::make_pair<float, float>(0, 0));
	for (int i = 0; i < keyframes.size() - 1; i++)
	{
		G = getGPositionMatrix(i);
		for (int j = 0; j < 5; j++)
		{
			float ub = (j + 1) * .2;
			float ua = j * .2;

			// gaussian
			float xis[3] = { -sqrt(3.0 / 5.0), 0.0f, sqrt(3.0 / 5.0) };
			float wis[3] = { 5 / 9.0, 8 / 9.0, 5 / 9.0 };
			float s = 0;
			for (int i = 0; i < 3; i++)
			{
				float u = (ub - ua) / 2 * xis[i] + (ub + ua) / 2;
				glm::vec3 dp = G * (B * glm::vec4(0, 1, 2 * u, 3 * u * u));
				s += wis[i] * glm::length(dp);
			}
			s = s * (ub - ua) / 2;
			usTable.push_back(std::make_pair<float, float>(i + ub, usTable[usTable.size() - 1].second + s));
		}

	}
	return usTable;
}
