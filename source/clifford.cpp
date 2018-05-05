//=============================================================================================
// Path animation with derivative calculation provided by the Clifford algebra
//=============================================================================================
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vector>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		// must be downloaded 
#include <GL/freeglut.h>	// must be downloaded unless you have an Apple
#endif

const unsigned int windowWidth = 600, windowHeight = 600;

// OpenGL major and minor versions
int majorVersion = 3, minorVersion = 3;

void getErrorInfo(unsigned int handle) {
	int logLen;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0) {
		char * log = new char[logLen];
		int written;
		glGetShaderInfoLog(handle, logLen, &written, log);
		printf("Shader log:\n%s", log);
		delete log;
	}
}

// check if shader could be compiled
void checkShader(unsigned int shader, const char * message) {
	int OK;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
	if (!OK) {
		printf("%s!\n", message);
		getErrorInfo(shader);
	}
}

// check if shader could be linked
void checkLinking(unsigned int program) {
	int OK;
	glGetProgramiv(program, GL_LINK_STATUS, &OK);
	if (!OK) {
		printf("Failed to link shader program!\n");
		getErrorInfo(program);
	}
}

// vertex shader in GLSL
const char * vertexSource = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			// View-Projection matrix in row-major format
    uniform vec2 point, tangent;

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0

	void main() {
		vec2 normal = vec2(-tangent.y, tangent.x);
		vec2 p = vertexPosition.x * tangent + vertexPosition.y * normal + point; 
		gl_Position = vec4(p.x, p.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char * fragmentSource = R"(
	#version 330
    precision highp float;

	uniform vec4 color;			// uniform color
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = color; 
	}
)";

// row-major matrix 4x4
struct mat4 {
	float m[4][4];
public:
	mat4() {}
	mat4(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33) {
		m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
		m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
		m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
		m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
	}

	mat4 operator*(const mat4& right) {
		mat4 result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.m[i][j] = 0;
				for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
			}
		}
		return result;
	}
	operator float*() { return &m[0][0]; }
};


// 2D point in homogeneous coordinates
struct vec2 {
	float x, y;
	vec2(float x0 = 0, float y0 = 0) { x = x0; y = y0; }
};

// 2D point in homogeneous coordinates
struct vec4 {
	float x, y, z, w;
	vec4(float x0 = 0, float y0 = 0, float z0 = 0, float w0 = 0) { x = x0; y = y0; z = z0; w = w0; }
};

// 2D camera
struct Camera {
	float wCx, wCy;	// center in world coordinates
	float wWx, wWy;	// width and height in world coordinates
public:
	Camera() {
		Animate(0);
	}

	mat4 V() { // view matrix: translates the center to the origin
		return mat4(1,    0, 0, 0,
			        0,    1, 0, 0,
			        0,    0, 1, 0,
			     -wCx, -wCy, 0, 1);
	}

	mat4 P() { // projection matrix: scales it to be a square of edge length 2
		return mat4(1/wWx,    0, 0, 0,
			        0,    1/wWy, 0, 0,
			        0,        0, 1, 0,
			        0,        0, 0, 1);
	}

	mat4 Vinv() { // inverse view matrix
		return mat4(1,     0, 0, 0,
				    0,     1, 0, 0,
			        0,     0, 1, 0,
			        wCx, wCy, 0, 1);
	}

	mat4 Pinv() { // inverse projection matrix
		return mat4(wWx/1, 0,    0, 0,
			           0, wWy/1, 0, 0,
			           0,  0,    1, 0,
			           0,  0,    0, 1);
	}

	void Animate(float t) {
		wCx = 0;
		wCy = 0;
		wWx = 10;
		wWy = 10;
	}
};

// 2D camera
Camera camera;

// handle of the shader program
unsigned int shaderProgram;

struct Clifford {
	float f, d;
	Clifford(float f0 = 0, float d0 = 0) { f = f0, d = d0; }
	Clifford operator+(Clifford r) { return Clifford(f + r.f, d + r.d); }
	Clifford operator-(Clifford r) { return Clifford(f - r.f, d - r.d); }
	Clifford operator*(Clifford r) { return Clifford(f * r.f, f * r.d + d * r.f); }
	Clifford operator/(Clifford r) {
		float l = r.f * r.f;
		return (*this) * Clifford(r.f / l, -r.d / l);
	}

	Clifford static T(float t) { return Clifford(t, 1); }
	Clifford static Sin(float t) { return Clifford(sin(t), cos(t)); }
	Clifford static Cos(float t) { return Clifford(cos(t), -sin(t)); }

};
/*
void Path(float t, Clifford& x, Clifford& y) {
	x = Clifford::Sin(t) * (Clifford::Sin(t) + 3) * 3 / (Clifford::Sin(t) + 2);
	y = (Clifford::Cos(t) * 4 + 1) / (Clifford::Sin(t) + 2);
}
*/

void Path(float t, Clifford& x, Clifford& y) {
	float r = 3.0f;
	x = Clifford::Sin(t) * r;
	y = Clifford::Cos(t) * r;
}

class Object {
	unsigned int vao;	// vertex array object id
	int nPoints;
	vec4 color;
public:

	Object(std::vector<vec2>& points, vec4 color0) {
		color = color0;
		nPoints = points.size();

		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active
		unsigned int vbo;		// vertex buffer objects
		glGenBuffers(1, &vbo);	// Generate 1 vertex buffer objects
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		// Map Attribute Array 0 to the current bound vertex buffer (vbo[0])

		glBufferData(GL_ARRAY_BUFFER,      // copy to the GPU
			points.size() * sizeof(vec2), // number of the vbo in bytes
			points.data(),		   // address of the data array on the CPU
			GL_STATIC_DRAW);	   // copy to GPU

		glEnableVertexAttribArray(0); 
		// Data organization of Attribute Array 0 
		glVertexAttribPointer(0,			// Attribute Array 0
			                  2, GL_FLOAT,  // components/attribute, component type
							  GL_FALSE,		// not in fixed point format, do not normalized
							  0, NULL);     // stride and offset: it is tightly packed
	}

	void Draw( vec2 point, vec2 tangent ) {
		mat4 MVPTransform = camera.V() * camera.P();

		// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVPTransform); // set uniform variable MVP to the MVPTransform
		else printf("uniform MVP cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "point");
		if (location >= 0) glUniform2f(location, point.x, point.y);

		location = glGetUniformLocation(shaderProgram, "tangent");
		if (location >= 0) glUniform2f(location, tangent.x, tangent.y);

		location = glGetUniformLocation(shaderProgram, "color");
		if (location >= 0) glUniform4f(location, color.x, color.y, color.z, color.w);

		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		glDrawArrays(GL_LINE_LOOP, 0, nPoints);	// draw a single triangle with vertices defined in vao
	}

	void Animate(float t) {
		Clifford x, y;
		Path(t, x, y);
		float tangentLength = sqrt(x.d * x.d + y.d * y.d);		// tangentLength == v
		vec2 tangent(x.d / tangentLength, y.d / tangentLength);		// tangent == it
		Draw(vec2(x.f, y.f), tangent);
	}

};


// The virtual world: collection of two objects
Object * vehicle;
Object * path;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	std::vector<vec2> points;
	points.push_back(vec2(-1, -1));
	points.push_back(vec2(1, 0));
	points.push_back(vec2(-1, 1));
	points.push_back(vec2(0, 0));

	vec4 color = vec4(1, 1, 0, 1);
	vehicle = new Object(points, color);

	std::vector<vec2> pathPoints;
	for (float t = 0; t < 2.0f * M_PI; t += 0.1f) {
		Clifford x, y;
		Path(t, x, y);								//minden pontban iránymenti derivált számítás
		pathPoints.push_back(vec2(x.f, y.f));
	}
	color = vec4(1, 1, 1, 1);
	path = new Object(pathPoints, color);

	// Create vertex shader from string
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertexShader) {
		printf("Error in vertex shader creation\n");
		exit(1);
	}
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	checkShader(vertexShader, "Vertex shader error");

	// Create fragment shader from string
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fragmentShader) {
		printf("Error in fragment shader creation\n");
		exit(1);
	}
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	checkShader(fragmentShader, "Fragment shader error");

	// Attach shaders to a single program
	shaderProgram = glCreateProgram();
	if (!shaderProgram) {
		printf("Error in shader program creation\n");
		exit(1);
	}
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Connect the fragmentColor to the frame buffer memory
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");	// fragmentColor goes to the frame buffer memory

	// program packaging
	glLinkProgram(shaderProgram);
	checkLinking(shaderProgram);
	// make this program run
	glUseProgram(shaderProgram);
}

void onExit() {
	glDeleteProgram(shaderProgram);
	printf("exit");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);								// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// clear the screen
	path -> Draw(vec2(0, 0), vec2(1, 0));
	
	long time = glutGet(GLUT_ELAPSED_TIME);					// elapsed time since the start of the program
	float sec = time / 1000.0f;								// convert msec to sec
	printf("time = %f\n", sec);
	vehicle->Animate(sec);

	glutSwapBuffers();										// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
	}
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
	float sec = time / 1000.0f;				// convert msec to sec
	camera.Animate(sec);					// animate the camera
	glutPostRedisplay();					// redraw the scene
}

int main(int argc, char * argv[]) {
	glutInit(&argc, argv);
#if !defined(__APPLE__)
	glutInitContextVersion(majorVersion, minorVersion);
#endif
	glutInitWindowSize(windowWidth, windowHeight);				// Application window is initially of resolution 600x600
	glutInitWindowPosition(100, 100);							// Relative location of the application window
#if defined(__APPLE__)
	gl0utInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_3_CORE_PROFILE);  // 8 bit R,G,B,A + double buffer + depth buffer
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutCreateWindow(argv[0]);

#if !defined(__APPLE__)
	glewExperimental = true;	// magic
	glewInit();
#endif

	printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
	printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
	printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
	printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	onInitialization();

	glutDisplayFunc(onDisplay);                // Register event handlers
	glutMouseFunc(onMouse);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutKeyboardUpFunc(onKeyboardUp);
	glutMotionFunc(onMouseMotion);

	glutMainLoop();
	onExit();
	return 1;
}