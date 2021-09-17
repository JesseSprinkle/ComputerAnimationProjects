#ifndef HELI_H
#define HELI_H
#include "Helicopter.h"

Helicopter::Helicopter(std::shared_ptr<Shape> _body1, std::shared_ptr<Shape> _body2, std::shared_ptr<Shape> _prop1, std::shared_ptr<Shape> _prop2)
{
	body1 = _body1;
	body2 = _body2;
	prop1 = _prop1;
	prop2 = _prop2;
	position = glm::vec3(0);
}

void Helicopter::setPosition(glm::vec3 _position)
{
	position = _position;
}
glm::vec3 Helicopter::getPosition()
{
	return position;
}
void Helicopter::draw(std::shared_ptr<MatrixStack> MV, float t, std::shared_ptr<Program> prog)
{
	MV->pushMatrix();
	MV->translate(position);
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniform3f(prog->getUniform("kd"), 1.0f, 0.0f, 0.0f);
	body1->draw(prog);
	glUniform3f(prog->getUniform("kd"), 1.0f, 1.0f, 0.0f);
	body2->draw(prog);
	MV->pushMatrix();
		MV->translate(glm::vec3(0.0, 0.4819, 0.0));
		MV->rotate(t, glm::vec3(0, 1, 0));
		MV->translate(-glm::vec3(0.0, 0.4819, 0.0));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniform3f(prog->getUniform("kd"), 0.0f, 0.0f, 1.0f);
		prop1->draw(prog);
	MV->popMatrix();
	MV->pushMatrix();
		MV->translate(glm::vec3(0.6228, 0.1179, 0.1365));
		MV->rotate(t, glm::vec3(0, 0, 1));
		MV->translate(-glm::vec3(0.6228, 0.1179, 0.1365));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		prop2->draw(prog);
	MV->popMatrix();
	MV->popMatrix();	
}
#endif