//[VERTEX SHADER]
#version 330

layout (location = 0)in vec3 InVertex;
layout (location = 1) in vec3 InNormal;
layout (location = 2)in int mindex;

smooth out vec4 Color;
//smooth out vec3 vNormal;
flat out int m;

uniform mat4 MVP;
uniform mat4 M;

void main()
{
	gl_Position = MVP * vec4(InVertex, 1.0);
	Color = vec4(InNormal, 1.0f);
	m = mindex;
}