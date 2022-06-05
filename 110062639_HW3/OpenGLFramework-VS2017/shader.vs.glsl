#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;


out vec3 vertex_normal;
out vec3 view_normal;
out vec3 model_position;
out vec3 V;
out vec3 L;
out vec3 view_light_direction;
out vec2 texCoord;

uniform int light_type;
uniform mat4 um4p;	
uniform mat4 um4v;
uniform mat4 um4m;

uniform vec3 light_position;
uniform vec3 light_direction;

// [TODO] passing uniform variable for texture coordinate offset
void main()
{
	// [TODO]
	texCoord = aTexCoord;
	gl_Position = um4p * um4v * um4m * vec4(aPos, 1.0);
	model_position= (um4m * vec4(aPos, 1)).xyz; //model world space

	V = normalize(-(um4v * um4m * vec4(aPos, 1)).xyz); // eyeCoord - modelCoord
	L = normalize((um4v * vec4(light_position, 1)).xyz - (um4v * um4m * vec4(aPos, 1)).xyz); // lightPosition - modelCoord
	
	view_normal = normalize((um4v * um4m * vec4(aNormal, 0)).xyz); //¦beye space°µlighting

	if(light_type == 0)
		view_light_direction = normalize(-(um4v * vec4(light_direction, 0)).xyz);
	else
		view_light_direction = L;


	vertex_normal = aNormal;
}

