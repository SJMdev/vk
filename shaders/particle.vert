#version 430 core

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in float life;

uniform mat4 view_matrix;
uniform mat4 projection_matrix; 

out float particle_lifetime;

void main()
{ 
	particle_lifetime = life;
	gl_Position = projection_matrix * view_matrix * vec4(vertex_position, 1.0); 
}