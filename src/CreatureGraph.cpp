#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "CreatureGraph.h"
#include "MatrixStack.h"
#include <stdlib.h>
#include <stdio.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

const int NUM_NORMALS = 6;

vec4 normals[NUM_NORMALS] = { 
	vec4(1, 0, 0, 0),
	vec4(-1, 0, 0, 0),
	vec4(0, 1, 0, 0),
	vec4(0, -1, 0, 0),
	vec4(0, 0, 1, 0),
	vec4(0, 0, -1, 0)
};

void swimVector(Node *n, glm::mat4 M) {
	int i;
	vec4 normal;
	printf("All the normals for this node are....\n");
	for (i = 0; i < NUM_NORMALS; i++) {
		normal = normalize(M * normals[i]);
		printf("vec3(%f, %f, %f)\n", normal.x, normal.y, normal.z);
	}
	printf("\n");
	
}


