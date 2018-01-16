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

#define SWIM_SCALAR 30.0f
#define DAMPING_FACTOR 0.1f

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
	float damp_speed;
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

vec3 swimVector(Creature creature, Node *n, glm::mat4 M) {
	int i;
	surface s[3];
	vec3 retVel = vec3(0, 0, 0);
	vec3 dampable_velocity = n->lastLocation - vec3(M * vec4(0, 0, 0, 1));

	n->velocity = n->lastLocation - vec3(M * vec4(0, 0, 0, 1)) + creature.velocity;
	//printf("velocity=vec3(%f, %f, %f)\n", n->velocity.x,
	//	n->velocity.y, n->velocity.z);

	/* Calculate the amount of water it is moving for velocity */
	//printf("All the normals for this node are....\n");
	for (i = 0; i < NUM_NORMALS; i++) {
		s[i].normal = vec3(normalize(M * normals[i]));
		s[i].area = getSurfaceArea(i, n->dimensions);
		s[i].speed = dot(n->velocity, s[i].normal) * s[i].area;
		s[i].damp_speed = dot(dampable_velocity, s[i].normal) * s[i].area;
		retVel += s[i].speed * SWIM_SCALAR * vec3(normals[i]);
		retVel -= s[i].damp_speed * DAMPING_FACTOR * vec3(normals[i]);
	//	printf("vec3(%f, %f, %f)", s[i].normal.x, s[i].normal.y, s[i].normal.z);
	//	printf("==>swimmSpeed=%f\n", s[i].speed);
	}
	//printf("\n");
	//printf("returned Velocity=(%f, %f, %f)\n\n", retVel.x, retVel.y, retVel.z);

	return retVel;
}


