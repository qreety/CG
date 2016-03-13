//[VERTEX SHADER]
#version 330

layout (location = 0)in vec3 InVertex;
layout (location = 1) in vec3 InColor;
layout (location = 2)in int m;

smooth out vec4 Color;

uniform mat4 ProjectionModelviewMatrix;

void main()
{
	float i = float(m);
	gl_Position = ProjectionModelviewMatrix * vec4(InVertex, 1.0);
	Color = vec4(InColor, 1.0f);
}