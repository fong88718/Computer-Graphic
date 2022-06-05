#version 330 core

in vec2 texCoord;
in vec3 vertex_normal;
in vec3 model_position;
in vec3 L;
in vec3 V;
in vec3 view_light_direction;
in vec3 view_normal;

out vec4 fragColor;

uniform sampler2D um4texture;

uniform vec3 light_position;
uniform float shininess;
uniform vec3 ambient_intensity;
uniform vec3 diffuse_intensity;
uniform vec3 specular_intensity;
uniform int light_type;
uniform float exponent;
uniform float cutoff;
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform vec3 constant;

uniform int eye;
uniform vec2 offset;

void main() {

	// [TODO] sampleing from texture
	// Hint: texture

	float spotlight_effect = 1;// Spotlight effect
	float spot_cutoff = cos(cutoff);
	float spot_theta = dot(-L, view_light_direction);
	
	if(light_type == 2)
	{
		if(spot_theta < spot_cutoff)
			spotlight_effect = 0;
		else
			spotlight_effect = pow(max(spot_theta, 1), exponent);
	}
	

	// ambient light
	vec3 a_color = ka * ambient_intensity;
	
	// direction light
	float distance = length(light_position - model_position);
	float f_att = min(1 / (constant.x + constant.y * distance + constant.z * distance * distance), 1);

	float cos_theta = clamp(dot(view_normal, view_light_direction), 0, 1);
	vec3 d_color =  f_att * diffuse_intensity * kd * cos_theta * spotlight_effect;

	
	// Specular light
	vec3 H = normalize(L + V);
	vec3 s_color =  f_att * ks * specular_intensity * pow(dot(H, view_normal), shininess) * spotlight_effect;

	vec3 vertex_color = a_color + d_color + s_color;

	// [TODO] sampleing from texture
	// Hint: texture
	if(eye == 1)
		fragColor = texture(um4texture, texCoord.xy + offset)* vec4(vertex_color, 0);
	else
		fragColor = texture(um4texture, texCoord.xy)* vec4(vertex_color, 0);

}