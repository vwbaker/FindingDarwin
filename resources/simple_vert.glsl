#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;
out vec3 fragNor;
out vec3 WPos;
out vec3 Dlight;

void main()
{
	gl_Position = P * V * M * vertPos;
	fragNor = (V * M * vec4(vertNor, 0.0)).xyz;
	WPos = vec3(V * M * vertPos);
	Dlight = (V * M * vec4(-1, 3, 1, 0)).xyz;
}
