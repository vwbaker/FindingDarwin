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

const int NUM_NORMALS = 3;

vec4 normals[NUM_NORMALS] = { 
	vec4(1, 0, 0, 0),
	vec4(0, 1, 0, 0),
	vec4(0, 0, 1, 0),
};

typedef struct surface {
	vec3 normal;
	float area;
	float speed;
} surface;

float getSurfaceArea(int i, vec3 dimensions) {
	if (i == 0) {
		return dimensions.y * dimensions.y;
	} else if (i == 1) {
		return dimensions.x * dimensions.z;
	} else if (i == 2) {
		return dimensions.x * dimensions.y;
	}
	fprintf(stderr, "Surface area of surface %d not possible\n", i);
	return 0;
}

void swimVector(Node *n, glm::mat4 M) {
	int i;
	surface s[3];
	printf("All the normals for this node are....\n");
	for (i = 0; i < NUM_NORMALS; i++) {
		s[i].normal = vec3(normalize(M * normals[i]));
		s[i].area = getSurfaceArea(i, n->dimensions);
		s[i].speed = dot(n->velocity, s[i].normal) * s[i].area;
		printf("vec3(%f, %f, %f)", s[i].normal.x, s[i].normal.y, s[i].normal.z);
		printf("==>swimmSpeed=%f\n", s[i].speed);
	}
	printf("\n");
	
}


