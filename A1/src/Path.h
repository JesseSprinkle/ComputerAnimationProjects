#ifndef PATH_H
#define PATH_H
#include <vector>
#include "MatrixStack.h"
#include "Program.h"
#include "Keyframe.h"
#include "Helicopter.h"


class Path
{
private:
	std::vector<Keyframe> keyframes;
	std::shared_ptr<Helicopter> heli;
	glm::mat4 B;
	void drawLines(glm::mat4& G);
	glm::mat4 getGPositionMatrix(int curr_seg);
public:
	Path(std::vector<Keyframe> _keyframes, std::shared_ptr<Helicopter> _heli);
	void drawHelis(std::shared_ptr<MatrixStack> MV, float t, std::shared_ptr<Program> prog);
	void drawCurve();
	glm::vec3 getCurrentPosition(float u);
	glm::quat getCurrentRotation(float u);
	void addKeyframe(Keyframe _keyframe);
	std::vector<std::pair<float, float>> createParameterizationTable();
};
#endif