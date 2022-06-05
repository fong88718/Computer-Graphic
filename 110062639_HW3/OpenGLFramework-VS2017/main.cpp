#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>


#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;

// current window size
int screenWidth = WINDOW_WIDTH, screenHeight = WINDOW_HEIGHT;

int filter = 0, filter_mipmap = 0;
int eyechange = 1;

double x_start = 0, y_start = 0;

float x_distance, y_distance, scroll_distance;
bool is_auto_rotate = false;
int numbers_of_model;
bool is_draw_wire = false;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum LightType
{
	directional = 0,
	point = 1,
	spotlight = 2,
};

int light_type_index = directional;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	LightEdit = 3,
	ShininessEdit = 4,
	ViewCenter = 5,
	ViewEye = 6,
	ViewUp = 7,
};

struct Directional_light
{
	Vector3 Position = { 1, 1, 1 };
	Vector3 Direction = { -1, -1, -1};
	Vector3 Attenuation;
	Vector3 diffuse_Intensity = { 1, 1, 1 };
};
Directional_light directional_light;

struct Position_light
{
	Vector3 Position = { 0, 2, 1 };
	Vector3 Attenuation = { 0.01, 0.8, 0.1 };
	Vector3 diffuse_Intensity = { 1, 1, 1 };
};
Position_light position_light;

struct Spot_light
{
	Vector3	Position = { 0, 0, 2 };
	Vector3	Direction = { 0, 0, -1 };
	float Exponent = 50;
	float Cutoff = 30;
	Vector3 Attenuation = { 0.05, 0.3, 0.6 };
	Vector3 diffuse_Intensity = { 1, 1, 1 };
};
Spot_light spot_light;


Vector3 ambient_Intensity = { 0.15, 0.15, 0.15 };
Vector3 specular_Intensity = { 1, 1, 1 };
float shininess = 64;

struct Uniform
{
	GLint 
		iLocLight_Position,
		iLocLight_Direction,
		iLocShiniess,
		iLocKa,
		iLocKd,
		iLocKs,
		iLocConstant,
		iLocCutoff,
		iLocExpoent,
		iLocAmbient_Intensity,
		iLocDiffuse_Intensity,
		iLocSpecular_Intensity,
		iLocLight_Type,
		iLocEye,
		iLocOffset,
		iLocM,
		iLocV,
		iLocP;
	GLuint iLocTexture;
};
Uniform uniform;



vector<string> filenames; // .obj filename list

typedef struct _Offset {
	GLfloat x;
	GLfloat y;
	struct _Offset(GLfloat _x, GLfloat _y) {
		x = _x;
		y = _y;
	};
} Offset;



typedef struct
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;

	GLuint diffuseTexture;

	// eye texture coordinate 
	GLuint isEye;
	vector<Offset> offsets;

} PhongMaterial;

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	GLuint p_texCoord;
	PhongMaterial material;
	int indexCount;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;

	bool hasEye;
	GLint max_eye_offset = 7;
	GLint cur_eye_offset_idx = 0;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};

ProjMode cur_proj_mode = Perspective;
TransMode cur_trans_mode = GeoTranslation;

Vector2 offset[7] = {
	Vector2(0.0, 0.0),
	Vector2(0.0, 0.75),
	Vector2(0.0, 0.5),
	Vector2(0.0, 0.25),
	Vector2(0.5, 0.00),
	Vector2(0.5, 0.75),
	Vector2(0.5, 0.5),
};

Shape m_shpae;

Matrix4 view_matrix;
Matrix4 project_matrix;


int cur_idx = 0; // represent which model should be rendered now
vector<string> model_list{ "../TextureModels/Fushigidane.obj", "../TextureModels/Mew.obj","../TextureModels/Nyarth.obj","../TextureModels/Zenigame.obj", "../TextureModels/laurana500.obj", "../TextureModels/Nala.obj", "../TextureModels/Square.obj" };

GLuint program;

static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}

// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1
	);
	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1
	);
	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);
	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);
	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	Vector3 p1 = main_camera.center - main_camera.position;
	Vector3 p2 = main_camera.up_vector - main_camera.position;
	GLfloat camera_center[] = {p1.x, p1.y, p1.z };
	GLfloat camera_up[] = { p2.x, p2.y, p2.z };

	GLfloat Rx[3], Ry[3], Rz[3] = {-p1.x, -p1.y, -p1.z};
	Normalize(Rz);

	Cross(camera_center, camera_up, Rx);
	Normalize(Rx);

	Cross(Rz, Rx, Ry);

	Matrix4 R(
		Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1);
	Matrix4 T(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1);

	view_matrix = R * T;
}

void setOrthogonal() // parallel projection
{
	cur_proj_mode = Orthogonal;

	GLfloat tx = -(proj.right + proj.left) / (proj.right - proj.left);
	GLfloat ty = -(proj.top + proj.bottom) / (proj.top - proj.bottom);
	GLfloat tz = -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip);

	project_matrix = Matrix4(
		2 / (proj.right - proj.left), 0, 0, tx,
		0, 2 / (proj.top - proj.bottom), 0, ty,
		0, 0, -2 / (proj.farClip - proj.nearClip), tz,
		0, 0, 0, 1
	);
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	// GLfloat f = ...
	// project_matrix [...] = ...
	Matrix4 scale_down(
		proj.aspect, 0, 0, 0,
		0, proj.aspect, 0, 0,
		0, 0, proj.aspect, 0,
		0, 0, 0, 1);

	project_matrix = Matrix4(
		(-1.0 / tan(proj.fovy / 2)) / proj.aspect, 0, 0, 0,
		0, -1.0 / tan(proj.fovy / 2), 0, 0,
		0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), (2 * proj.farClip * proj.nearClip) / (proj.nearClip - proj.farClip),
		0, 0, -1, 0
	);

	if (proj.aspect < 1)
		project_matrix = scale_down * project_matrix;
}



void setGLMatrix(GLfloat* glm, Matrix4& m) {
	glm[0] = m[0];  glm[4] = m[1];   glm[8] = m[2];    glm[12] = m[3];
	glm[1] = m[4];  glm[5] = m[5];   glm[9] = m[6];    glm[13] = m[7];
	glm[2] = m[8];  glm[6] = m[9];   glm[10] = m[10];   glm[14] = m[11];
	glm[3] = m[12];  glm[7] = m[13];  glm[11] = m[14];   glm[15] = m[15];
}

// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	// [TODO] change your aspect ratio
	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;
	proj.aspect = (float)(width / 2) / height;

	if (cur_proj_mode == Perspective)
		setPerspective();
	else if (cur_proj_mode == Orthogonal)
		setOrthogonal();
}

void Vector3ToFloat4(Vector3 v, GLfloat res[4])
{
	res[0] = v.x;
	res[1] = v.y;
	res[2] = v.z;
	res[3] = 1;
}

void autororate(int is_auto_rotate)
{
	if(is_auto_rotate)
		models[cur_idx].rotation.y += 0.015;
}

// Render function for display rendering
void RenderScene(int per_vertex_or_per_pixel) 
{
	autororate(is_auto_rotate);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	setViewingMatrix();
	if (cur_proj_mode == Perspective)
		setPerspective();
	else if (cur_proj_mode == Orthogonal)
		setOrthogonal();

	if (is_draw_wire)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	

	Matrix4 model_matrix = T * R * S;
	glUniformMatrix4fv(uniform.iLocM, 1, GL_FALSE, model_matrix.getTranspose());
	glUniformMatrix4fv(uniform.iLocV, 1, GL_FALSE, view_matrix.getTranspose());
	glUniformMatrix4fv(uniform.iLocP, 1, GL_FALSE, project_matrix.getTranspose());

	glUniform3f(uniform.iLocKa, models[cur_idx].shapes[0].material.Ka.x, models[cur_idx].shapes[0].material.Ka.y, models[cur_idx].shapes[0].material.Ka.z);
	glUniform3f(uniform.iLocKd, models[cur_idx].shapes[0].material.Kd.x, models[cur_idx].shapes[0].material.Kd.y, models[cur_idx].shapes[0].material.Kd.z);
	glUniform3f(uniform.iLocKs, models[cur_idx].shapes[0].material.Ks.x, models[cur_idx].shapes[0].material.Ks.y, models[cur_idx].shapes[0].material.Ks.z);

	switch (light_type_index) 
	{
		case directional:
			glUniform3f(uniform.iLocLight_Position, directional_light.Position.x, directional_light.Position.y, directional_light.Position.z);
			glUniform3f(uniform.iLocLight_Direction, directional_light.Direction.x, directional_light.Direction.y, directional_light.Direction.z);
			glUniform3f(uniform.iLocConstant, directional_light.Attenuation.x, directional_light.Attenuation.y, directional_light.Attenuation.z);
			glUniform3f(uniform.iLocDiffuse_Intensity, directional_light.diffuse_Intensity.x, directional_light.diffuse_Intensity.y, directional_light.diffuse_Intensity.z);
			break;
		case point:
			glUniform3f(uniform.iLocLight_Position, position_light.Position.x, position_light.Position.y, position_light.Position.z);
			glUniform3f(uniform.iLocConstant, position_light.Attenuation.x, position_light.Attenuation.y, position_light.Attenuation.z);
			glUniform3f(uniform.iLocDiffuse_Intensity, position_light.diffuse_Intensity.x, position_light.diffuse_Intensity.y, position_light.diffuse_Intensity.z);
			break;
		case spotlight:
			glUniform3f(uniform.iLocLight_Position, spot_light.Position.x, spot_light.Position.y, spot_light.Position.z);
			glUniform3f(uniform.iLocLight_Direction, spot_light.Direction.x, spot_light.Direction.y, spot_light.Direction.z);
			glUniform3f(uniform.iLocConstant, spot_light.Attenuation.x, spot_light.Attenuation.y, spot_light.Attenuation.z);
			glUniform3f(uniform.iLocDiffuse_Intensity, spot_light.diffuse_Intensity.x, spot_light.diffuse_Intensity.y, spot_light.diffuse_Intensity.z);
			break;
	}

	glUniform1f(uniform.iLocExpoent, spot_light.Exponent);
	glUniform1i(uniform.iLocLight_Type, light_type_index);

	glUniform3f(uniform.iLocAmbient_Intensity, ambient_Intensity.x, ambient_Intensity.y, ambient_Intensity.z);
	glUniform3f(uniform.iLocSpecular_Intensity, specular_Intensity.x, specular_Intensity.y, specular_Intensity.z);
	glUniform1f(uniform.iLocShiniess, shininess);
	glUniform1f(uniform.iLocCutoff, spot_light.Cutoff);

	glUniform1i(uniform.iLocTexture, 0);

	// render object
	int eye_idx = models[cur_idx].cur_eye_offset_idx;
	glUniform2f(uniform.iLocOffset, offset[eye_idx].x, offset[eye_idx].y);

	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		glBindVertexArray(models[cur_idx].shapes[i].vao);

		// [TODO] Bind texture and modify texture filtering & wrapping mode
		// Hint: glActiveTexture, glBindTexture, glTexParameteri

		int diffuse_texture = models[cur_idx].shapes[i].material.diffuseTexture;
		if (diffuse_texture == 1 || diffuse_texture == 4 || diffuse_texture == 6 || diffuse_texture == 8)
			glUniform1i(uniform.iLocEye, 1);
		else
			glUniform1i(uniform.iLocEye, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, models[cur_idx].shapes[i].material.diffuseTexture);
		if(filter == 0) 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		if(filter_mipmap == 0)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE)
			exit(0);
		else if (key == 'w' || key == 'W')
			is_draw_wire = !is_draw_wire;
		else if (key == 'z' || key == 'Z')
			cur_idx = (--cur_idx + numbers_of_model) % numbers_of_model;
		else if (key == 'x' || key == 'X')
			cur_idx = ++cur_idx % numbers_of_model;
		else if (key == 'o' || key == 'O')
			cur_proj_mode = Orthogonal, setOrthogonal();
		else if (key == 'p' || key == 'P')
			cur_proj_mode = Perspective, setPerspective();
		else if (key == 't' || key == 'T')
			cur_trans_mode = GeoTranslation;
		else if (key == 's' || key == 'S')
			cur_trans_mode = GeoScaling;
		else if (key == 'r' || key == 'R')
			cur_trans_mode = GeoRotation;
		else if (key == 'y' || key == 'Y')
			is_auto_rotate = !is_auto_rotate;
		else if (key == 'e' || key == 'E')
			cur_trans_mode = ViewEye;
		else if (key == 'c' || key == 'C')
			cur_trans_mode = ViewCenter;
		else if (key == 'u' || key == 'U')
			cur_trans_mode = ViewUp;
		else if (key == 'l' || key == 'L')
		{
			light_type_index = ++light_type_index % 3;
			printf("Current mode : ");
			switch (light_type_index)
			{
			case directional:
				printf("directional light\n");
				break;
			case point:
				printf("position light\n");
				break;
			case spotlight:
				printf("spot light\n");
				break;
			}
		}
		else if (key == 'k' || key == 'K')
			cur_trans_mode = LightEdit;
		else if (key == 'j' || key == 'J')
			cur_trans_mode = ShininessEdit;
		else if (key == 'g' || key == 'G')
			filter = ++filter % 2;
		else if (key == 'b' || key == 'B')
			filter_mipmap = ++filter_mipmap % 2;
		else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
			models[cur_idx].cur_eye_offset_idx = ++models[cur_idx].cur_eye_offset_idx % models[cur_idx].max_eye_offset;
		else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) 
			models[cur_idx].cur_eye_offset_idx = (--models[cur_idx].cur_eye_offset_idx  + models[cur_idx].max_eye_offset) % models[cur_idx].max_eye_offset;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	switch (cur_trans_mode)
	{
		case GeoTranslation:
			models[cur_idx].position.z += yoffset; 
			break;
		case GeoScaling:
			models[cur_idx].scale.z += yoffset; 
			break;
		case GeoRotation:
			models[cur_idx].rotation.z += yoffset; 
			break;
		case ViewEye:
			main_camera.position.z += yoffset;
			setViewingMatrix(); 
			break;
		case ViewCenter:
			main_camera.center.z += yoffset;
			setViewingMatrix();
			break;
		case ViewUp:
			main_camera.up_vector.z += yoffset;
			setViewingMatrix();
			break;
		case LightEdit:
			switch (light_type_index)
			{
				case directional:
					directional_light.diffuse_Intensity += (yoffset * 0.1) * Vector3 { 1, 1, 1 };
					break;
				case point:
					position_light.diffuse_Intensity += (yoffset * 0.1) * Vector3 { 1, 1, 1 };
					break;
				case spotlight:
					spot_light.Cutoff += yoffset * 0.1;
					if (spot_light.Cutoff < 0)
						spot_light.Cutoff = 0;
					break;
			}
			break;
		case ShininessEdit:
			shininess += yoffset * 3;
			break;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		mouse_pressed = true;
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		mouse_pressed = false;
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	x_distance = (xpos - starting_press_x) * 0.005;
	y_distance = (ypos - starting_press_y) * 0.005;

	starting_press_x = xpos;
	starting_press_y = ypos;

	if (mouse_pressed)
	{
		switch (cur_trans_mode)
		{
			case GeoTranslation:
				models[cur_idx].position.x += x_distance;
				models[cur_idx].position.y -= y_distance;
				break;
			case GeoScaling:
				models[cur_idx].scale.x += x_distance;
				models[cur_idx].scale.y -= y_distance;
				break;
			case GeoRotation:
				models[cur_idx].rotation.x -= y_distance;
				models[cur_idx].rotation.y -= x_distance;
				break;
			case ViewUp:
				main_camera.up_vector.x -= x_distance;
				main_camera.up_vector.y -= y_distance;
				setViewingMatrix();
				break;
			case ViewEye:
				main_camera.position.x -= x_distance;
				main_camera.position.y -= y_distance;
				setViewingMatrix();
				break;
			case ViewCenter:
				main_camera.center.x -= x_distance;
				main_camera.center.y += y_distance;
				main_camera.up_vector.x -= x_distance;
				main_camera.up_vector.y += y_distance;
				setViewingMatrix();
				break;
			case LightEdit:
				switch (light_type_index)
				{
					case directional:
						directional_light.Position += Vector3( x_distance, -y_distance, 0);
						directional_light.Direction = -directional_light.Position;
						break;
					case point:
						position_light.Position += Vector3( x_distance, -y_distance, 0);
						break;
					case spotlight:
						spot_light.Position += Vector3( x_distance, -y_distance, 0);
						break;
				}
				break;
		}
	}
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs.glsl");
	fs = textFileRead("shader.fs.glsl");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p, f);
	glAttachShader(p, v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}
	
	glDeleteShader(v);
	glDeleteShader(f);

	if (success)
		glUseProgram(p);
	else
	{
		system("pause");
		exit(123);
	}
	program = p;
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, vector<GLfloat>& textureCoords, vector<int>& material_id, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
			normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
			normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			// Optional: texture coordinate
			textureCoords.push_back(attrib->texcoords[2 * idx.texcoord_index + 0]);
			textureCoords.push_back(attrib->texcoords[2 * idx.texcoord_index + 1]);
			// The material of this vertex
			material_id.push_back(shape->mesh.material_ids[f]);
		}
		index_offset += fv;
	}
}

static string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

GLuint LoadTextureImage(string image_path)
{
	int channel, width, height;
	int require_channel = 4;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc *data = stbi_load(image_path.c_str(), &width, &height, &channel, require_channel);
	if (data != NULL)
	{
		// [TODO] Bind the image to texture
		// Hint: glGenTextures, glBindTexture, glTexImage2D, glGenerateMipmap
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image from memory after binding to texture
		stbi_image_free(data);
		return texture;
	}
	else
	{
		cout << "LoadTextureImage: Cannot load image from " << image_path << endl;
		return -1;
	}
}

vector<Shape> SplitShapeByMaterial(vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, vector<GLfloat>& textureCoords, vector<int>& material_id, vector<PhongMaterial>& materials)
{
	vector<Shape> res;
	for (int m = 0; m < materials.size(); m++)
	{
		vector<GLfloat> m_vertices, m_colors, m_normals, m_textureCoords;
		for (int v = 0; v < material_id.size(); v++)
		{
			// extract all vertices with same material id and create a new shape for it.
			if (material_id[v] == m)
			{
				m_vertices.push_back(vertices[v * 3 + 0]);
				m_vertices.push_back(vertices[v * 3 + 1]);
				m_vertices.push_back(vertices[v * 3 + 2]);

				m_colors.push_back(colors[v * 3 + 0]);
				m_colors.push_back(colors[v * 3 + 1]);
				m_colors.push_back(colors[v * 3 + 2]);

				m_normals.push_back(normals[v * 3 + 0]);
				m_normals.push_back(normals[v * 3 + 1]);
				m_normals.push_back(normals[v * 3 + 2]);

				m_textureCoords.push_back(textureCoords[v * 2 + 0]);
				m_textureCoords.push_back(textureCoords[v * 2 + 1]);
			}
		}

		if (!m_vertices.empty())
		{
			Shape tmp_shape;
			glGenVertexArrays(1, &tmp_shape.vao);
			glBindVertexArray(tmp_shape.vao);

			glGenBuffers(1, &tmp_shape.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
			glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(GL_FLOAT), &m_vertices.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
			tmp_shape.vertex_count = (int)m_vertices.size() / 3;

			glGenBuffers(1, &tmp_shape.p_color);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
			glBufferData(GL_ARRAY_BUFFER, m_colors.size() * sizeof(GL_FLOAT), &m_colors.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glGenBuffers(1, &tmp_shape.p_normal);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
			glBufferData(GL_ARRAY_BUFFER, m_normals.size() * sizeof(GL_FLOAT), &m_normals.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glGenBuffers(1, &tmp_shape.p_texCoord);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_texCoord);
			glBufferData(GL_ARRAY_BUFFER, m_textureCoords.size() * sizeof(GL_FLOAT), &m_textureCoords.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);

			tmp_shape.material = materials[m];
			res.push_back(tmp_shape);
		}
	}

	return res;
}

void LoadTexturedModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;
	vector<GLfloat> textureCoords;
	vector<int> material_id;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", (int)shapes.size(), (int)materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);

		material.diffuseTexture = LoadTextureImage(base_dir + string(materials[i].diffuse_texname));
		if (material.diffuseTexture == -1)
		{
			cout << "LoadTexturedModels: Fail to load model's material " << i << endl;
			system("pause");
		}

		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		textureCoords.clear();
		material_id.clear();

		normalization(&attrib, vertices, colors, normals, textureCoords, material_id, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		// split current shape into multiple shapes base on material_id.
		vector<Shape> splitedShapeByMaterial = SplitShapeByMaterial(vertices, colors, normals, textureCoords, material_id, allMaterial);

		// concatenate splited shape to model's shape list
		tmp_model.shapes.insert(tmp_model.shapes.end(), splitedShapeByMaterial.begin(), splitedShapeByMaterial.end());
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.8;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)(WINDOW_WIDTH / 2) / (float)WINDOW_HEIGHT; // adjust width for side by side view

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setUniformVariables()
{
	uniform.iLocConstant = glGetUniformLocation(program, "constant");
	uniform.iLocExpoent = glGetUniformLocation(program, "exponent");
	uniform.iLocLight_Position = glGetUniformLocation(program, "light_position");
	uniform.iLocLight_Direction = glGetUniformLocation(program, "light_direction");
	uniform.iLocLight_Type = glGetUniformLocation(program, "light_type");
	uniform.iLocKa = glGetUniformLocation(program, "ka");
	uniform.iLocKd = glGetUniformLocation(program, "kd");
	uniform.iLocKs = glGetUniformLocation(program, "ks");
	uniform.iLocCutoff = glGetUniformLocation(program, "cutoff");
	uniform.iLocAmbient_Intensity = glGetUniformLocation(program, "ambient_intensity");
	uniform.iLocDiffuse_Intensity = glGetUniformLocation(program, "diffuse_intensity");
	uniform.iLocSpecular_Intensity = glGetUniformLocation(program, "specular_intensity");
	uniform.iLocShiniess = glGetUniformLocation(program, "shininess");

	// [TODO] Get uniform location of texture
	uniform.iLocP = glGetUniformLocation(program, "um4p");
	uniform.iLocV = glGetUniformLocation(program, "um4v");
	uniform.iLocM = glGetUniformLocation(program, "um4m");
	uniform.iLocTexture = glGetUniformLocation(program, "um4texture");
	uniform.iLocEye = glGetUniformLocation(program, "eye");
	uniform.iLocOffset = glGetUniformLocation(program, "offset");
}


void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();
	setUniformVariables();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	// [TODO] Load five model at here
	numbers_of_model = model_list.size();

	for (string &model_path : model_list) 
		LoadTexturedModels(model_path);
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}

int main(int argc, char **argv)
{
	// initial glfw
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif


	// create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "110062639 HW3", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);


	// load OpenGL function pointer
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glPrintContextInfo(false);

	// register glfw callback functions
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		// render
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		// render left view
		glViewport(0, 0, screenWidth / 2, screenHeight);
		RenderScene(1);
		// render right view
		glViewport(screenWidth / 2, 0, screenWidth / 2, screenHeight);
		RenderScene(0);

		// swap buffer from back to front
		glfwSwapBuffers(window);

		// Poll input event
		glfwPollEvents();
	}

	// just for compatibiliy purposes
	return 0;
}

