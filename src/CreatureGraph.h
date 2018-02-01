#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace glm;

// joint-types
const float ONE_DEGREE = 0;
const float SPHERICAL = 1;

typedef struct Connection {
	// Vector from point of rotation to placement on parent
	vec3 position;
	
	// See joint-types above
	int jointType;
	// radians that represent the max degree from initial state a joint can rotate
	float maxMotion;

	// 0 or 1 depending if the same node should appear exactly opposite it on the body
	int reflection;
} Connection;

typedef struct Node {
	// Dimensions of the block
	vec3 dimensions;
	vec3 scale;

	// describe orientation through
	// 	x: radians to twist (about the x axis)
	// 	y: radians on the x-z plane
	// 	z: radians on the x-y plane
	//
	// 	orientation == orientation at rest
	// 	theta == orientation relative to its rotation point
	// 	rotationPoint == its joint (probably where it connects to its parent joint 
	vec3 orientation;
	vec3 theta;
	vec3 rotationPoint;

	/* The minimum and maximum thetas are allowed to be */
	vec3 min_theta;
	vec3 max_theta;

	// Direction and speed this specific node is moving
	vec3 velocity;
	// flags whether the node is moving intentionally
	int moving;
	// In order to calculate the node's velocity
	vec3 lastLocation;

	// limbs or things connected to this node (out arrows)
	int numChild;
	struct Node *children;

	// How this node is connected to its parent (null at root)
	Connection parentJoint;

	// number of times this node is its own child i.e. number of repetitions on this block
	int recursiveLimit;
	// node that appears only on the last node of the recursive limit
	struct Node *terminalOnly;
} Node;

typedef struct Creature {
	Node *root;
	vec3 position;
	vec3 velocity;
} Creature;

vec3 swimVector(Creature creature, Node *n, glm::mat4 M);

vec3 rotationVector(glm::mat4 M, vec3 velocity);

vec3 optimalDirection(Node *n, vec3 pos, vec3 goal);
