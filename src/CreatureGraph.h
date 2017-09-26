
// joint-types
const float ONE_DEGREE = 0;
const float SPHERICAL = 1;

typedef struct Connection {
	// Vector from point of rotation to placement on parent
	vec3 position;
	
	// describe orientation through
	// 	x: radians to twist (about the x axis)
	// 	y: radians on the x-z plane
	// 	z: radians on the x-y plane
	vec3 orientation;

	// See joint-types above
	int jointType;
	// radians that represent the max degree from initial state a joint can rotate
	float maxMotion;

	// 0 or 1 depending if the same node should appear exactly opposite it on the body
	int reflection;
} Connection;

typedef struct Node {
	// Dimentions of the block
	float width;
	float height;
	float depth;
	vec3 scale;

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

