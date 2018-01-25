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

const int NUM_DIMENSIONS = 3;

vec4 normals[NUM_DIMENSIONS] = { 
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

/* Given a creature, one node, and its orientation M, calculate
 * how much the creature would push forward or backward based on its 
 * movement relative to the root from the last frame */
vec3 swimVector(Creature creature, Node *n, glm::mat4 M) {
	int i;
	surface s[NUM_DIMENSIONS];
	vec3 retVel = vec3(0, 0, 0);
	vec3 dampable_velocity = n->lastLocation - vec3(M * vec4(0, 0, 0, 1));

	n->velocity = n->lastLocation - vec3(M * vec4(0, 0, 0, 1)) + creature.velocity;

	/* Calculate the amount of water it is moving for velocity */
	for (i = 0; i < NUM_DIMENSIONS; i++) {
		s[i].normal = vec3(normalize(M * normals[i]));
		s[i].area = getSurfaceArea(i, n->dimensions);
		s[i].speed = dot(n->velocity, s[i].normal) * s[i].area;
		s[i].damp_speed = dot(dampable_velocity, s[i].normal) * s[i].area;
		retVel += s[i].speed * SWIM_SCALAR * vec3(normals[i]);
		retVel -= s[i].damp_speed * DAMPING_FACTOR * vec3(normals[i]);
	}

	return retVel;
}

/* Given a velocity and a creatures' root M, calculates the rotation at which
 * it would move */
vec3 rotationVector(glm::mat4 M, vec3 velocity) {
	int i;
	vec3 rot(0, 0, 0), n;

	for (i = 0; i < NUM_DIMENSIONS; i++) {
		n = vec3(normalize(M * normals[i]));
		rot.x += dot(vec2(n.y, n.z), vec2(velocity.y, velocity.z)); 
		rot.y += dot(vec2(n.x, n.z), vec2(velocity.x, velocity.z)); 
		rot.z += dot(vec2(n.x, n.y), vec2(velocity.x, velocity.y)); 
	}

	return rot;
}
