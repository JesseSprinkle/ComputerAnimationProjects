#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shape.h"
#include "Program.h"
#include "MatrixStack.h"
class Helicopter
{
private:
	std::shared_ptr<Shape> body1;
	std::shared_ptr<Shape> body2;
	std::shared_ptr<Shape> prop1;
	std::shared_ptr<Shape> prop2;
	glm::vec3 position;
public:
	Helicopter(std::shared_ptr<Shape> _body1, std::shared_ptr<Shape> _body2, std::shared_ptr<Shape> _prop1, std::shared_ptr<Shape> _prop2);
	void setPosition(glm::vec3 _position);
	glm::vec3 getPosition();
	void draw(std::shared_ptr<MatrixStack> MV, float t, std::shared_ptr<Program> prog);
};