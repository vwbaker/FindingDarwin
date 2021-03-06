/* 
Base code for joint class with Applied Parallel computing
Using a framebuffer as a basis to implement Gaussian blur in GLSL
Currently will make 2 FBOs and textures (only uses one in base code)
and writes out frame as a .png (Texture_output.png)
upper right quad on screen is blue-er to show fragment shader effect on texture

Winter 2017 - ZJW (Piddington texture write)
Look for "TODO" in this file and write new shaders
*
*/

#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "CreatureGraph.h"
#include "Texture.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

const float PI = 3.14159;
const float DEG_85 = 1.48353;
const int POPULATION = 1;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> prog, prog2; 
shared_ptr<Shape> cube, sphere;

double lastTime = 0;
int frames = 0;

Texture texture2;
GLint h_texture2;

int g_width = 512;
int g_height = 512;
int g_GiboLen = 6;
int gMat = 0;

float cameraX = 0, cameraY = 0, cameraZ = 0;;
float theta = -PI/2.0, phi = 0;
double xorig, yorig;
int firsttime = 1;
glm::vec3 target(0, 3, -4), eye(0, 3.0, -3);

bool going_up;
vec3 goal(-1, 5, -5);

Creature creatures[POPULATION];

//global reference to texture FBO
GLuint depthBuf;

//geometry for texture render
GLuint quad_VertexArrayID;
GLuint quad_vertexbuffer;
GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;

//forward declaring a useful function listed later
void SetMaterial(int i);

/**** geometry set up for a quad *****/
void initQuad() {

 	//now set up a simple quad for rendering FBO
  	glGenVertexArrays(1, &quad_VertexArrayID);
  	glBindVertexArray(quad_VertexArrayID);

  	static const GLfloat g_quad_vertex_buffer_data[] = {
    		-1.0f, -1.0f, 0.0f,
    		1.0f, -1.0f, 0.0f,
    		-1.0f,  1.0f, 0.0f,
    		-1.0f,  1.0f, 0.0f,
    		1.0f, -1.0f, 0.0f,
    		1.0f,  1.0f, 0.0f,
  	};

  	glGenBuffers(1, &quad_vertexbuffer);
  	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
  	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

}

/* lots of initialization to set up the opengl state and data */
static void initGL()
{
	GLSL::checkVersion();
  	int width, height;
  	glfwGetFramebufferSize(window, &width, &height);

	// Set background color.
	glClearColor(.12f, .34f, .56f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	//Initialize the geometry to render a quad to the screen
	initQuad();

	// Initialize cube
	cube = make_shared<Shape>();
	cube->loadMesh(RESOURCE_DIR + "cube.obj");
	cube->resize();
	cube->init();

	sphere = make_shared<Shape>();
	sphere->loadMesh(RESOURCE_DIR + "sphere.obj");
	sphere->resize();
	sphere->init();

	// Initialize the creature program	
	prog = make_shared<Program>();
	prog->setVerbose(true);
	prog->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("M");
	prog->addUniform("V");
	prog->addUniform("MatAmb");
	prog->addUniform("MatDif");
	prog->addUniform("MatSpec");
	prog->addUniform("shine");
	prog->addAttribute("vertPos");
	prog->addAttribute("vertTex");
	prog->addAttribute("vertNor");

	// Initialize the ground program
	prog2 = make_shared<Program>();
	prog2->setVerbose(true);
	prog2->setShaderNames(RESOURCE_DIR + "tex_vert.glsl", RESOURCE_DIR + "tex_frag2.glsl");
	prog2->init();
	prog2->addUniform("P");
	prog2->addUniform("M");
	prog2->addUniform("V");
	prog2->addUniform("Texture2");
	prog2->addTexture(&texture2);

	// Initialize textures
	texture2.setFilename(RESOURCE_DIR + "grass.bmp");
	texture2.setUnit(2);
	texture2.setName("Texture2");
	texture2.init();

  	//set up depth necessary since we are rendering a mesh that needs depth test
  	glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
  	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);
  	
	//more FBO set up
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  	glDrawBuffers(1, DrawBuffers);

}

static void initGeom() {
   float g_groundSize = 20;
   float g_groundY = 0;

  /* A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2*/
    float GrndPos[] = {
    -g_groundSize, g_groundY, -g_groundSize,
    -g_groundSize, g_groundY,  g_groundSize,
     g_groundSize, g_groundY,  g_groundSize,
     g_groundSize, g_groundY, -g_groundSize
    };

    float GrndNorm[] = {
     0, 1, 0,
     0, 1, 0,
     0, 1, 0,
     0, 1, 0,
     0, 1, 0,
     0, 1, 0
    };


  static GLfloat GrndTex[] = {
      0, 0, /* back*/
      0, 1,
      1, 1,
      1, 0 };

    unsigned short idx[] = {0, 1, 2, 0, 2, 3};


   GLuint VertexArrayID;
        /*generate the VAO*/
        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);

    g_GiboLen = 6;
    glGenBuffers(1, &GrndBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

    glGenBuffers(1, &GrndNorBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);
    
         glGenBuffers(1, &GrndTexBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

    glGenBuffers(1, &GIndxBuffObj);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

static void initLastLocations(vec3 offset, Node *cur) {
	int i;
	cur->lastLocation = offset + cur->parentJoint.position;
	for (i = 0; i < cur->numChild; i++) {
		initLastLocations(cur->lastLocation, cur->children + i);
	}
}

static void initCreatures() {
	int i;
	for (i = 0; i < POPULATION; i++) {
		creatures[i].position = vec3(-1, 3, -5);
		creatures[i].velocity = vec3(0, 0, 0);


		/* Currently setting all values manually */
		Node *cur = creatures[i].root = (Node *) calloc(1, sizeof(Node));
		cur->dimensions = vec3(.5, .2, .7);
		cur->orientation = vec3(0, 0, 0);
		cur->numChild = 2;
		cur->parentJoint = {vec3(0, 0, 0), 0, 0, 0};
		cur->children = (struct Node *) calloc(sizeof(Node), 2);
		(cur->children)[0].dimensions = vec3(0.2, 0.05, 0.1);
		(cur->children)[0].parentJoint = {vec3(0.2 + 0.5, 0, 0), 0, 0, 0};
		(cur->children)[0].rotationPoint = vec3(1, 0, 0);
		(cur->children)[0].theta = vec3(0, 0, 0);
		(cur->children)[0].min_theta = vec3(-PI/2, -PI/4, -PI/4);
		(cur->children)[0].max_theta = vec3(PI/2, PI/4, PI/4);
		(cur->children)[1].dimensions = vec3(0.2, 0.05, 0.1);
		(cur->children)[1].parentJoint = {vec3(-0.2 - 0.5, 0, 0), 0, 0, 0};
		(cur->children)[1].rotationPoint = vec3(-1, 0, 0);
		(cur->children)[1].theta = vec3(0, 0, 0);
		(cur->children)[1].min_theta = vec3(-PI/2, -PI/4, -PI/4);
		(cur->children)[1].max_theta = vec3(PI/2, PI/4, PI/4);
		
		initLastLocations(creatures[i].position, cur);
	}
}

static vec3 drawNode(Creature creature, Node *cur, shared_ptr<MatrixStack> M) {
	int i;
	vec3 v = vec3(0, 0, 0);

	/* Figure out which direction to move my fin in to swim :) */
	if (creature.root != cur) {
		vec3 force_dir = going_up ? vec3(0, 1, 0) : vec3(0, -1, 0);
		vec3 best = optimalDirection(cur, creature, goal, force_dir);
		cur->theta += best;
	}

	/* translate to its position relative to its parent */
	M->translate((cur->parentJoint).position);

	/* Translate it back from it's rotation point */
	M->translate(vec3(-cur->rotationPoint.x * cur->dimensions.x,
		-cur->rotationPoint.y * cur->dimensions.y,
		-cur->rotationPoint.z * cur->dimensions.z));

	/* Set orientation / rotation */
	M->rotate(cur->theta.x, vec3(1, 0, 0));
	M->rotate(cur->theta.y, vec3(0, 1, 0));
	M->rotate(cur->theta.z, vec3(0, 0, 1));

	/* Translate it to it's rotation point to set the orientation above */
	M->translate(vec3(cur->rotationPoint.x * cur->dimensions.x,
		cur->rotationPoint.y * cur->dimensions.y,
		cur->rotationPoint.z * cur->dimensions.z));

	
	/* scale and actually draw the creature */
	M->pushMatrix();
		M->scale(cur->dimensions);
        	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
		cube->draw(prog);

		/* Calculate the resulting velocity of the node */
		v = swimVector(creature, cur, M->topMatrix());
		cur->lastLocation = M->topMatrix() * vec4(0, 0, 0, 1);
	M->popMatrix();

	/* draw any children */
	for (i = 0; i < cur->numChild; i++) {
		Node *child = (cur->children + i);
		M->pushMatrix();
		v = vec3(v + drawNode(creature, child, M));
		M->popMatrix();
	}

	return v;
	
}

static void calculateSwimmingArm(Node *n) {
	double ctime = glfwGetTime() * 1;
	int trunc_time = (int) ctime;
	double percent = ctime - trunc_time;

	/* Sets one fin to try and push more water down that it is
	 * pushing up */
	if (trunc_time % 4 == 0) {
		n->theta.x = 0;
		n->theta.y = 0;
		n->theta.z = 1 - percent;
	} else if (trunc_time % 4 == 1) {
		n->theta.x = percent * PI / 2;
		n->theta.y = -percent;
		n->theta.z = 0;
	} else if (trunc_time % 4 == 2) {
		n->theta.x = PI / 2;
		n->theta.y = percent - 1;
		n->theta.z = 0;
	} else {
		n->theta.x = PI / 2 - percent * PI / 2;
		n->theta.y = 0;
		n->theta.z = percent;
	}

	/* Uncomment this to just have one fin alternating
	 * up and down */
/*
	if (trunc_time % 2 == 0) {
		n->theta.z = 1 - 2 * percent;
	} else {
		n->theta.z = -1 + 2 * percent;

	}
*/

	/* Uncomment this to set the arm to a constant value */
/*
	n->theta.x = 0;
	n->theta.y = 0;
	n->theta.z = 0;
*/
}

static void drawCreatures() {
	int i;
   	auto M = make_shared<MatrixStack>();
	vec3 v, r;

   	auto P = make_shared<MatrixStack>();
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
   	float aspect = width/(float)height;
   	P->pushMatrix();
   	P->perspective(45.0f, aspect, 0.01f, 100.0f);

        glm::vec3 up(0, 1, 0);
	glm::mat4 V = glm::lookAt(eye, target, up);

	prog->bind();
	SetMaterial(0);
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(V));

	frames++;

	/* For every creature */
	for (i = 0; i < POPULATION; i++) {
		M->pushMatrix();
		M->loadIdentity();
		M->translate(creatures[i].position);
		Node *cur = creatures[i].root;

		/* Force y to oscilate */
		if (cur->children->theta.y < -0.75) {
			going_up = true;
		} else if (cur->children->theta.y > 0.75) {
			going_up = false;
		}

		/* FOR DEBUGGING: a statement to have something print once every second */
		if (glfwGetTime() > 1 + lastTime) {
			printf("Frames: %d, average fps: %f\n", frames, frames / glfwGetTime());
			lastTime = glfwGetTime();
			printf("position=(%f, %f, %f)\n", creatures[i].position.x,
				creatures[i].position.y, creatures[i].position.z);
			printf("Last rotation=(%f, %f, %f)\n", r.x, r.y, r.z);
			printf("theta=(%f,%f,%f)\n", cur->children->theta.x, cur->children->theta.y,
				cur->children->theta.z);
		}

		/* Draw the creature */
		v = drawNode(creatures[i], cur, M);
		r = rotationVector(M->topMatrix(), v);
		creatures[i].root->theta = vec3(creatures[i].root->theta + r);
		/* Update fields */
		creatures[i].position += vec3(v.x, v.y, v.z);
		creatures[i].velocity = v;
		M->popMatrix();
	}

	/* draw the goal */
	M->pushMatrix();
	M->loadIdentity();
	M->translate(goal);
	M->scale(vec3(0.1, 0.1, 0.1));
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	sphere->draw(prog);
	M->popMatrix();

	prog->unbind();

}

/* The render loop - this function is called repeatedly during the OGL program run */
static void render()
{
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	//set up to render to buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Leave this code to just draw the meshes alone */
	//Use the matrix stack for Lab 6
   	float aspect = width/(float)height;

   	// Create the matrix stacks - please leave these alone for now
   	auto P = make_shared<MatrixStack>();
   	auto M = make_shared<MatrixStack>();
        glm::vec3 up(0, 1, 0);
	glm::mat4 V = glm::lookAt(eye, target, up);
   	// Apply perspective projection.
   	P->pushMatrix();
   	P->perspective(45.0f, aspect, 0.01f, 100.0f);

	drawCreatures();

        /*draw the ground plane */
        prog2->bind();
	M->pushMatrix();
		M->loadIdentity();
        glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	glUniformMatrix4fv(prog2->getUniform("V"), 1, GL_FALSE, value_ptr(V));
	M->popMatrix();

        glEnableVertexAttribArray(0);
   	glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
   	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
   	glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
   	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
         
        glEnableVertexAttribArray(2);
   	glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
   	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

   	/* draw!*/
   	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
   	glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        
   	P->popMatrix();
        prog2->unbind();
}

/* helper function */
static void error_callback(int error, const char *description) {
	cerr << description << endl;
}

/* key callback */
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	} else if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		glm::vec3 right(normalize(cross(
			glm::vec3(0, 1, 0),
			glm::vec3(eye - target))));
		eye -= glm::vec3(right.x * 0.2, 0, right.z * 0.2);
		target -= glm::vec3(right.x * 0.2, 0, right.z * 0.2);
	} else if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		glm::vec3 right(normalize(cross(
			glm::vec3(0, 1, 0),
			glm::vec3(eye - target))));
		eye += glm::vec3(right.x * 0.2, 0, right.z * 0.2);
		target += glm::vec3(right.x * 0.2, 0, right.z * 0.2);
	}
	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		glm::vec3 view(normalize(target - eye));
		eye += glm::vec3(view.x * 0.2, view.y * 0.2, view.z * 0.2);
		target += glm::vec3(view.x * 0.2, view.y * 0.2, view.z * 0.2);
	} else if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		glm::vec3 view(normalize(target - eye));
		eye -= glm::vec3(view.x * 0.2, view.y * 0.2, view.z * 0.2);
		target -= glm::vec3(view.x * 0.2, view.y * 0.2, view.z * 0.2);
	} 
}

static void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
        if (firsttime) {
                xorig = xpos;
                yorig = ypos;
                firsttime = 0;
        } else {
                double dx = xpos - xorig, dy = ypos - yorig, scale = PI / (float) g_width;
                theta += dx * scale;
                phi -= dy * scale;
                phi = phi > DEG_85 ? DEG_85 : phi;
                phi = phi < -DEG_85 ? -DEG_85 : phi;
                xorig = xpos;
                yorig = ypos;
		target = glm::vec3(
			cos(phi) * cos(theta) + eye.x,
			sin(phi) + eye.y,
			cos(phi) * cos(PI / 2.0 - theta) + eye.z);
        }
}

/* resize window call back */
static void resize_callback(GLFWwindow *window, int width, int height) {
   g_width = width;
   g_height = height;
   glViewport(0, 0, width, height);
}

//helper function to set materials for shadin
void SetMaterial(int i) {
  switch (i) {
    	case 0: //shiny blue plastic
 		glUniform3f(prog->getUniform("MatAmb"), 0.02, 0.04, 0.2);
 		glUniform3f(prog->getUniform("MatDif"), 0.0, 0.16, 0.9);
		glUniform3f(prog->getUniform("MatSpec"), 0.14, 0.2, 0.8);
		glUniform1f(prog->getUniform("shine"), 120.0);
      		break;
    	case 1: // flat grey
 		glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.13, 0.14);
 		glUniform3f(prog->getUniform("MatDif"), 0.3, 0.3, 0.4);
		glUniform3f(prog->getUniform("MatSpec"), 0.3, 0.3, 0.4);
		glUniform1f(prog->getUniform("shine"), 4.0);
      		break;
    	case 2: //brass
 		glUniform3f(prog->getUniform("MatAmb"), 0.3294, 0.2235, 0.02745);
 		glUniform3f(prog->getUniform("MatDif"), 0.7804, 0.5686, 0.11373);
		glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.80784);
		glUniform1f(prog->getUniform("shine"), 27.9);
      		break;
	case 3: //copper
 		glUniform3f(prog->getUniform("MatAmb"), 0.1913, 0.0735, 0.0225);
 		glUniform3f(prog->getUniform("MatDif"), 0.7038, 0.27048, 0.0828);
		glUniform3f(prog->getUniform("MatSpec"), 0.257, 0.1376, 0.08601);
		glUniform1f(prog->getUniform("shine"), 12.8);
      		break;
	case 4: //turqouise
 		glUniform3f(prog->getUniform("MatAmb"), 0.1, 0.18725, 0.1745);
 		glUniform3f(prog->getUniform("MatDif"), 0.396, 0.74151, 0.69102);
		glUniform3f(prog->getUniform("MatSpec"), 0.297254, 0.30829, 0.306678);
		glUniform1f(prog->getUniform("shine"), 0.1 * 128);
      		break;
	case 5: //black rubber
 		glUniform3f(prog->getUniform("MatAmb"), 0.02, 0.02, 0.02);
 		glUniform3f(prog->getUniform("MatDif"), 0.01, 0.01, 0.01);
		glUniform3f(prog->getUniform("MatSpec"), 0.4, 0.4, 0.4);
		glUniform1f(prog->getUniform("shine"), 0.078125 * 128);
      		break;
	case 6: // emerald
 		glUniform3f(prog->getUniform("MatAmb"), 0.0215, 0.1745, 0.0215);
 		glUniform3f(prog->getUniform("MatDif"), 0.07568, 0.61424, 0.07568);
		glUniform3f(prog->getUniform("MatSpec"), 0.633, 0.727811, 0.633);
		glUniform1f(prog->getUniform("shine"), 0.6 * 128);
      		break;
  }
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	//request the highest possible version of OGL - important for mac
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(g_width, g_height, "FBO test", NULL, NULL);
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
	//weird bootstrap of glGetError
	glGetError();
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	//set the window resize call back
	glfwSetFramebufferSizeCallback(window, resize_callback);

	// Initialize scene. Note geometry initialized in init now
	initGL();
	initGeom();
	initCreatures();

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
