#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLSL.h"
#include "Program.h"
#include "Camera.h"
#include "MatrixStack.h"
#include "ShapeSkin.h"
#include "Texture.h"
#include "TextureMatrix.h"
#include "Bones.h"

using namespace std;

// Stores information in data/input.txt
class DataInput
{
public:
	vector<string> textureData;
	vector< vector<string> > meshData;
	string skeletonData;
};

DataInput dataInput;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the shaders are loaded from
string DATA_DIR = ""; // where the data are loaded from
bool keyToggles[256] = {false};

shared_ptr<Camera> camera = NULL;
vector< shared_ptr<ShapeSkin> > shapes;
map< string, shared_ptr<Texture> > textureMap;
shared_ptr<Program> progSimple = NULL;
shared_ptr<Program> progSkin = NULL;
shared_ptr<Program> progSkinGpu = NULL;
shared_ptr<Bones> bones = NULL;
double aggDrawTime = 0.0;
int framesMeasured = 0;
double t, t0;

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
	switch(key) {
	case 'g':
		aggDrawTime = 0.0;
		framesMeasured = 0;
		if (!keyToggles['g'])
		{
			// need to load the initial points into gpu if we turn on gpu animating
			for (const auto& shape : shapes) {
				shape->reloadVertices();
			}
		}
	}

	for(const auto &shape : shapes) {
		shape->getTextureMatrix()->update(key);
	}
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved(xmouse, ymouse);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = mods & GLFW_MOD_SHIFT;
		bool ctrl  = mods & GLFW_MOD_CONTROL;
		bool alt   = mods & GLFW_MOD_ALT;
		camera->mouseClicked(xmouse, ymouse, shift, ctrl, alt);
	}
}
void init()
{
	keyToggles[(unsigned)'c'] = true;
	
	camera = make_shared<Camera>();

	bones = make_shared<Bones>(DATA_DIR + dataInput.skeletonData);

	// Create shapes
	for(const auto &mesh : dataInput.meshData) {
		auto shape = make_shared<ShapeSkin>();
		shapes.push_back(shape);
		shape->setTextureMatrixType(mesh[0]);
		shape->loadMesh(DATA_DIR + mesh[0]);
		shape->loadAttachment(DATA_DIR + mesh[1]);
		shape->setTextureFilename(mesh[2]);
		shape->bindBones(bones);
	}
	
	// For drawing the grid, etc.
	progSimple = make_shared<Program>();
	progSimple->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	progSimple->setVerbose(true);
	
	// For skinned shape, CPU
	progSkin = make_shared<Program>();
	progSkin->setShaderNames(RESOURCE_DIR + "skin_vert.glsl", RESOURCE_DIR + "skin_frag.glsl");
	progSkin->setVerbose(true);

	// For skinned shape, GPU
	progSkinGpu = make_shared<Program>();
	progSkinGpu->setShaderNames(RESOURCE_DIR + "skin_vert_gpu.glsl", RESOURCE_DIR + "skin_frag.glsl");
	progSkinGpu->setVerbose(true);
	
	// Set background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	for(auto shape : shapes) {
		shape->init();
	}
	
	progSimple->init();
	progSimple->addUniform("P");
	progSimple->addUniform("MV");
	
	progSkin->init();
	progSkin->addAttribute("aPos");
	progSkin->addAttribute("aNor");
	progSkin->addAttribute("aTex");
	progSkin->addUniform("P");
	progSkin->addUniform("MV");
	progSkin->addUniform("ka");
	progSkin->addUniform("ks");
	progSkin->addUniform("s");
	progSkin->addUniform("kdTex");
	progSkin->addUniform("T");

	progSkinGpu->init();
	progSkinGpu->addAttribute("aPos");
	progSkinGpu->addAttribute("aNor");
	progSkinGpu->addAttribute("aTex");
	progSkinGpu->addAttribute("w0");
	progSkinGpu->addAttribute("w1");
	progSkinGpu->addAttribute("w2");
	progSkinGpu->addAttribute("b0");
	progSkinGpu->addAttribute("b1");
	progSkinGpu->addAttribute("b2");
	progSkinGpu->addAttribute("numInfl");
	progSkinGpu->addUniform("P");
	progSkinGpu->addUniform("MV");
	progSkinGpu->addUniform("M");
	progSkinGpu->addUniform("ka");
	progSkinGpu->addUniform("ks");
	progSkinGpu->addUniform("s");
	progSkinGpu->addUniform("kdTex");
	progSkinGpu->addUniform("T");
	
	// Bind the texture to unit 1.
	int unit = 1;
	progSkin->bind();
	glUniform1i(progSkin->getUniform("kdTex"), unit);
	progSkin->unbind();
	
	for(const auto &filename : dataInput.textureData) {
		auto textureKd = make_shared<Texture>();
		textureMap[filename] = textureKd;
		textureKd->setFilename(DATA_DIR + filename);
		textureKd->init();
		textureKd->setUnit(unit); // Bind to unit 1
		textureKd->setWrapModes(GL_REPEAT, GL_REPEAT);
	}

	// Initialize time.
	glfwSetTime(0.0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void render()
{
	// Update time.
	double t1 = glfwGetTime();
	float dt = (t1 - t0);
	if(keyToggles[(unsigned)' ']) {
		t += dt;
	}
	t0 = t1;

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	// Use the window size for camera.
	glfwGetWindowSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
	
	// Draw grid
	progSimple->bind();
	glUniformMatrix4fv(progSimple->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progSimple->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	float gridSizeHalf = 200.0f;
	int gridNx = 11;
	int gridNz = 11;
	glLineWidth(1);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_LINES);
	for(int i = 0; i < gridNx; ++i) {
		float alpha = i / (gridNx - 1.0f);
		float x = (1.0f - alpha) * (-gridSizeHalf) + alpha * gridSizeHalf;
		glVertex3f(x, 0, -gridSizeHalf);
		glVertex3f(x, 0,  gridSizeHalf);
	}
	for(int i = 0; i < gridNz; ++i) {
		float alpha = i / (gridNz - 1.0f);
		float z = (1.0f - alpha) * (-gridSizeHalf) + alpha * gridSizeHalf;
		glVertex3f(-gridSizeHalf, 0, z);
		glVertex3f( gridSizeHalf, 0, z);
	}
	glEnd();
	progSimple->unbind();
	
	double fps = 30;
	int frameCount = bones->getFrameCount();
	int frame = ((int)floor(t*fps)) % frameCount;
	
	// draw xyz
	if (keyToggles[(unsigned)'k'])
	{
		progSimple->bind();
		glUniformMatrix4fv(progSimple->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(progSimple->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glLineWidth(2);
		glBegin(GL_LINES);
		shared_ptr<vector<glm::mat4>> curr_bones = bones->getBonesAtFrame(frame);
		for (int i = 0; i < curr_bones->size(); ++i)
		{
			glm::vec3 x, y, z, o;
			x = curr_bones->operator[](i) * glm::vec4(8, 0, 0, 1);
			y = curr_bones->operator[](i) * glm::vec4(0, 8, 0, 1);
			z = curr_bones->operator[](i) * glm::vec4(0, 0, 8, 1);
			o = curr_bones->operator[](i) * glm::vec4(0, 0, 0, 1);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex3f(o.x, o.y, o.z);
			glVertex3f(x.x, x.y, x.z);
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3f(o.x, o.y, o.z);
			glVertex3f(y.x, y.y, y.z);
			glColor3f(0.0f, 0.0f, 1.0f);
			glVertex3f(o.x, o.y, o.z);
			glVertex3f(z.x, z.y, z.z);
		}
		glEnd();
		progSimple->unbind();
	}
	

	// Draw character
	double it = glfwGetTime();
	if (keyToggles[(unsigned int)'g'])
	{
		// calculate on cpu
		for (const auto& shape : shapes) {
			MV->pushMatrix();
			// Draw skin
			progSkin->bind();
			textureMap[shape->getTextureFilename()]->bind(progSkin->getUniform("kdTex"));
			glLineWidth(1.0f); // for wireframe
			glUniformMatrix4fv(progSkin->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(progSkin->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniform3f(progSkin->getUniform("ka"), 0.1f, 0.1f, 0.1f);
			glUniform3f(progSkin->getUniform("ks"), 0.1f, 0.1f, 0.1f);
			glUniform1f(progSkin->getUniform("s"), 200.0f);
			shape->setProgram(progSkin, false);
			shape->update(frame);
			shape->draw(frame);
			progSkin->unbind();

			MV->popMatrix();
		}
	}
	else
	{
		// calculate on gpu
		for (const auto& shape : shapes) {
			MV->pushMatrix();
			// Draw skin
			progSkinGpu->bind();
			textureMap[shape->getTextureFilename()]->bind(progSkinGpu->getUniform("kdTex"));
			glLineWidth(1.0f); // for wireframe
			glUniformMatrix4fv(progSkinGpu->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(progSkinGpu->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(progSkinGpu->getUniform("M"), 82, GL_FALSE, glm::value_ptr(bones->getAnimationMatricesAtFrame(frame)->operator[](0)));
			glUniform3f(progSkinGpu->getUniform("ka"), 0.1f, 0.1f, 0.1f);
			glUniform3f(progSkinGpu->getUniform("ks"), 0.1f, 0.1f, 0.1f);
			glUniform1f(progSkinGpu->getUniform("s"), 200.0f);
			shape->setProgram(progSkinGpu, true);
			// no update, only draw
			shape->draw(frame);
			progSkin->unbind();

			MV->popMatrix();
		}

	}
	double ft = glfwGetTime();
	framesMeasured++;
	aggDrawTime += ft - it;
	if (framesMeasured >= 60)
	{
		if (keyToggles[(unsigned int)'p'])
		{
			cout << "Average Draw Time using " << (keyToggles[(unsigned int)'g'] ? "CPU" : "GPU") << ": " << aggDrawTime / framesMeasured << endl;
		}
		framesMeasured = 0;
		aggDrawTime = 0;
	}

	// Pop matrix stacks.
	MV->popMatrix();
	P->popMatrix();

	GLSL::checkError(GET_FILE_LINE);
}

void loadDataInputFile()
{
	string filename = DATA_DIR + "input.txt";
	ifstream in;
	in.open(filename);
	if(!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;
	
	string line;
	while(1) {
		getline(in, line);
		if(in.eof()) {
			break;
		}
		if(line.empty()) {
			continue;
		}
		// Skip comments
		if(line.at(0) == '#') {
			continue;
		}
		// Parse lines
		string key, value;
		stringstream ss(line);
		// key
		ss >> key;
		if(key.compare("TEXTURE") == 0) {
			ss >> value;
			dataInput.textureData.push_back(value);
		} else if(key.compare("MESH") == 0) {
			vector<string> mesh;
			ss >> value;
			mesh.push_back(value); // obj
			ss >> value;
			mesh.push_back(value); // skin
			ss >> value;
			mesh.push_back(value); // texture
			dataInput.meshData.push_back(mesh);
		} else if(key.compare("SKELETON") == 0) {
			ss >> value;
			dataInput.skeletonData = value;
		} else {
			cout << "Unkown key word: " << key << endl;
		}
	}
	in.close();
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		cout << "Usage: A2 <SHADER DIR> <DATA DIR>" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	DATA_DIR = argv[2] + string("/");
	loadDataInputFile();
	
	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
