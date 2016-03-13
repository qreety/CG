#version 330
struct Material{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shine;
};

#define MAT_COUNT 6

smooth in vec4 Color;
flat in int m;

out vec4 color;

uniform Material mat[MAT_COUNT];
void main()
{
	vec3 c = mat[m].ambient + mat[m].diffuse + mat[m].specular;
	color = vec4(c, 1.0f);
}