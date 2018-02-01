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
const int NUM_TESTS = 5;
const float MAX_ROTATION = 0.01;

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

/* Given a position and a potential constraint, calculate the best rotation for
 * a fin to move */
vec3 optimalDirection(Node *n, vec3 pos, vec3 goal, vec3 force_dir) {
	int rx, ry, rz;
	vec3 best(0, 0, 0), cur = n->theta;
	vec3 min_theta = n->min_theta, max_theta = n->max_theta;
	vec3 rot_point(n->rotationPoint.x * n->dimensions.x, n->rotationPoint.y *
		n->dimensions.y, n->rotationPoint.z * n->dimensions.z);
	float max_progress = -500, cur_dist = distance(pos, goal);
	Node temp;
	Creature c;
	MatrixStack M;

	M.loadIdentity();
	M.rotate(cur.x, vec3(1, 0, 0));
       	M.rotate(cur.y, vec3(0, 1, 0));
       	M.rotate(cur.z, vec3(0, 0, 1));
	M.translate(rot_point);

	temp.lastLocation = M.topMatrix() * vec4(0, 0, 0, 1);
	temp.dimensions = n->dimensions;
	c.velocity = vec3(0, 0, 0);
	
	for (rx = -NUM_TESTS/2; rx <= NUM_TESTS/2; rx++) {
		for (ry = -NUM_TESTS/2; ry <= NUM_TESTS/2; ry++) {
			for (rz = -NUM_TESTS/2; rz <= NUM_TESTS/2; rz++) {
				vec3 test = cur + vec3(rx, ry, rz) * MAX_ROTATION;
				/* Check if it violates the constraints */
				if (test.x < min_theta.x || test.x > max_theta.x ||
					test.y < min_theta.y || test.y > max_theta.y ||
					test.z < min_theta.z || test.z > max_theta.z) {
					continue;
				}
				if ((force_dir.x != 0 && force_dir.x * rx <= 0) ||
					(force_dir.y != 0 && force_dir.y * ry <= 0) ||
					(force_dir.z != 0 && force_dir.z * rz <= 0)) {
					continue;
				}
				M.loadIdentity();
				M.rotate(test.x, vec3(1, 0, 0));
        			M.rotate(test.y, vec3(0, 1, 0));
        			M.rotate(test.z, vec3(0, 0, 1));
				M.translate(rot_point);

				vec3 v = swimVector(c, &temp, M.topMatrix());
				float progress = cur_dist - distance(pos+v, goal);
/*
				printf("returned swimVector=(%f, %f, %f) ==> progress=%f\n",
					v.x, v.y, v.z, progress);
*/
				if (progress > max_progress && progress != 0) {
					best = vec3(rx, ry, rz) * MAX_ROTATION;
					max_progress = progress;
				}
			}
		}
	}

	return best;
}
