// RobotArm.cpp : Defines the entry point for the application.
//

#include "RobotArm.h"

using namespace glm;
using namespace std;

const int window_width = 1024, window_height = 768;

typedef struct Vertex {
	float Position[4];
	float Color[4];
	float Normal[3];
	void SetPosition(float* coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = 1.0;
	}
	void SetColor(float* color) {
		Color[0] = color[0];
		Color[1] = color[1];
		Color[2] = color[2];
		Color[3] = color[3];
	}
	void SetNormal(float* coords) {
		Normal[0] = coords[0];
		Normal[1] = coords[1];
		Normal[2] = coords[2];
	}
};

// function prototypes
int initWindow(void);
void initOpenGL(void);
void createVAOs(Vertex[], GLushort[], int);
void loadObject(char* file, glm::vec4 color, Vertex*& out_Vertices, GLushort*& out_Indices, size_t& VertCount, int ObjectId);
void createObjects(void);
void pickObject(void);
void renderScene(float);
void cleanup(void);
static void keyCallback(GLFWwindow*, int, int, int, int);
static void mouseCallback(GLFWwindow*, int, int, int);
void translateObject(Vertex*& Verts, vec3 trans, int ObjectId, size_t VertCount);
void rotatePen(float, mat4&);
void moveBase(vec3&, float);
void moveBase(float);
void updateLight();
void moveTop(float, mat4&);
vec3 avgPos(Vertex*, const size_t);
void changeOpacity(Vertex*, const size_t, bool, int);

// GLOBAL VARIABLES
GLFWwindow* window;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

GLuint gPickedIndex = -1;
std::string gMessage;

GLuint programID;
GLuint pickingProgramID;

const GLuint NumObjects = 10;	// ATTN: THIS NEEDS TO CHANGE AS YOU ADD NEW OBJECTS
GLuint VertexArrayId[NumObjects];
GLuint VertexBufferId[NumObjects];
GLuint IndexBufferId[NumObjects];

// TL
size_t VertexBufferSize[NumObjects];
size_t IndexBufferSize[NumObjects];
size_t NumIdcs[NumObjects];
size_t NumVerts[NumObjects];

GLuint MatrixID;
GLuint ModelMatrixID;
GLuint ViewMatrixID;
GLuint ProjMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorID;
GLuint LightID;

glm::vec3 cameraPos, worldUp;

// Declare global objects
// TL
const size_t CoordVertsCount = 6;
Vertex CoordVerts[CoordVertsCount];

const size_t GridVertsCount = 12 * 12;
Vertex GridVerts[GridVertsCount];
GLushort GridIndices[GridVertsCount];

size_t BaseVertCount, PenVertCount, TopVertCount, Arm1VertCount, Arm2VertCount, ButtonVertCount, JointVertCount, ProjVertCount;
Vertex* BaseVerts, * PenVerts, * TopVerts, * Arm1Verts, * Arm2Verts, * ButtonVerts, * JointVerts, * ProjVerts, * InitProjVerts;

const size_t BezierVertsCount = 1200;
Vertex BezierVerts[BezierVertsCount];
GLushort BezierIndices[BezierVertsCount];

bool bPress = false, pPress = false, shiftPress = false, cPress = false, sPress = false, tPress = false, onePress = false, twoPress = false;

vec3 gOrientationTop;
vec3 gOrientationPen;
vec3 gOrientationArm1;
vec3 gOrientationArm2;

int initWindow(void) {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// FOR MAC

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Junnuthula,Mayur Reddy(36921238)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Initialize the GUI
	/*TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(window_width, window_height);
	TwBar* GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &gMessage, NULL);*/

	// Set up inputs
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);

	return 0;
}

void initOpenGL(void) {
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	cameraPos = vec3(10.0f, 10.0f, 10.0f);
	worldUp = vec3(0.0, 1.0, 0.0);

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	//gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	//gViewMatrix = glm::lookAt(glm::vec3(10.0, 10.0, 10.0f),	// eye
	//	glm::vec3(0.0, 0.0, 0.0),	// center
	//	glm::vec3(0.0, 1.0, 0.0));	// up

	gViewMatrix = glm::lookAt(cameraPos,	// eye
		glm::vec3(0.0, 0.0, 0.0),	// center
		worldUp);	// up

	updateLight();

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("shaders/StandardShading.vertexshader", "shaders/StandardShading.fragmentshader");
	pickingProgramID = LoadShaders("shaders/Picking.vertexshader", "shaders/Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ProjMatrixID = glGetUniformLocation(programID, "P");

	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");
	// Get a handle for our "LightPosition" uniform
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	// TL
	// Define objects
	createObjects();

	// ATTN: create VAOs for each of the newly created objects here:
	VertexBufferSize[0] = sizeof(CoordVerts);
	NumVerts[0] = CoordVertsCount;

	VertexBufferSize[1] = sizeof(GridVerts);
	NumVerts[1] = GridVertsCount;

	createVAOs(CoordVerts, NULL, 0);
	createVAOs(GridVerts, NULL, 1);
}

void createVAOs(Vertex Vertices[], unsigned short Indices[], int ObjectId) {
	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].Position);
	const size_t Normaloffset = sizeof(Vertices[0].Color) + RgbOffset;

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);
	glBindVertexArray(VertexArrayId[ObjectId]);

	// Create Buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Vertices, GL_STATIC_DRAW);

	// Create Buffer for indices
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[ObjectId], Indices, GL_STATIC_DRAW);
	}

	// Assign vertex attributes
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)RgbOffset);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)Normaloffset);	// TL

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color
	glEnableVertexAttribArray(2);	// normal

	// Disable our Vertex Buffer Object 
	glBindVertexArray(0);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
		);
	}
}

// Ensure your .obj files are in the correct format and properly loaded by looking at the following function
void loadObject(char* file, glm::vec4 color, Vertex*& out_Vertices, GLushort*& out_Indices, size_t& VertCount, int ObjectId) {
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ(file, vertices, normals);

	std::vector<GLushort> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, normals, indices, indexed_vertices, indexed_normals);

	const size_t vertCount = indexed_vertices.size();
	const size_t idxCount = indices.size();
	VertCount = vertCount;

	// populate output arrays
	out_Vertices = new Vertex[vertCount];
	for (int i = 0; i < vertCount; i++) {
		out_Vertices[i].SetPosition(&indexed_vertices[i].x);
		out_Vertices[i].SetNormal(&indexed_normals[i].x);
		out_Vertices[i].SetColor(&color[0]);
	}
	out_Indices = new GLushort[idxCount];
	for (int i = 0; i < idxCount; i++) {
		out_Indices[i] = indices[i];
	}

	// set global variables!!
	NumIdcs[ObjectId] = idxCount;
	VertexBufferSize[ObjectId] = sizeof(out_Vertices[0]) * vertCount;
	IndexBufferSize[ObjectId] = sizeof(GLushort) * idxCount;
}

void createObjects(void) {
	//-- COORDINATE AXES --//
	CoordVerts[0] = { { 0.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[1] = { { 5.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[2] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[3] = { { 0.0, 5.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[4] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[5] = { { 0.0, 0.0, 5.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };

	//-- GRID --//
	// ATTN: Create your grid vertices here!
	int ind = 0;
	float z = -5.0, x = -5.0;
	bool f = true;

	while (z <= 5.0) {
		if (f)
			GridVerts[ind] = { { -5.0, 0.0, z, 1.0 }, { 255.0, 255.0, 255.0, 1.0 }, { 0.0, 0.0, 1.0 } };
		else
			GridVerts[ind] = { { 5.0, 0.0, z, 1.0}, {255.0, 255.0, 255.0, 1.0}, {0.0, 0.0, 1.0} }, z++;

		f = !f;
		ind++;
	}

	f = true;
	while (x <= 5.0) {
		if (f)
			GridVerts[ind] = { { x, 0.0, -5.0, 1.0 }, { 255.0, 255.0, 255.0, 1.0 }, { 0.0, 0.0, 1.0 } };
		else
			GridVerts[ind] = { { x, 0.0, 5.0, 1.0}, {255.0, 255.0, 255.0, 1.0}, {0.0, 0.0, 1.0} }, x++;

		f = !f;
		ind++;
	}

	for (int i = 0; i < GridVertsCount; i++) {
		GridIndices[i] = i;
	}

	//-- .OBJs --//
	// ATTN: Load your models here through .obj files -- example of how to do so is as shown
	// Vertex* Verts;
	// GLushort* Idcs;
	// loadObject("models/base.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), Verts, Idcs, ObjectID);
	// createVAOs(Verts, Idcs, ObjectID);

	Vertex* Verts;
	GLushort* Idcs;
	size_t VertCount;
	loadObject((char*)"models/Base.obj", glm::vec4(255.0, 0.0, 0.0, 1.0), Verts, Idcs, VertCount, 2);
	createVAOs(Verts, Idcs, 2);
	BaseVerts = Verts;
	BaseVertCount = VertCount;

	loadObject((char*)"models/Top.obj", glm::vec4(255.0, 165.0, 0.0, 1.0), Verts, Idcs, VertCount, 3);
	createVAOs(Verts, Idcs, 3);
	TopVerts = Verts;
	TopVertCount = VertCount;

	loadObject((char*)"models/Arm1.obj", glm::vec4(0.0, 0.0, 255.0, 1.0), Verts, Idcs, VertCount, 4);
	createVAOs(Verts, Idcs, 4);
	Arm1Verts = Verts;
	Arm1VertCount = VertCount;

	loadObject((char*)"models/Joint.obj", glm::vec4(128.0, 0.0, 128.0, 1.0), Verts, Idcs, VertCount, 5);
	createVAOs(Verts, Idcs, 5);
	JointVerts = Verts;
	JointVertCount = VertCount;

	loadObject((char*)"models/Arm2.obj", glm::vec4(0.0, 193.0, 200.0, 1.0), Verts, Idcs, VertCount, 6);
	createVAOs(Verts, Idcs, 6);
	Arm2Verts = Verts;
	Arm2VertCount = VertCount;

	loadObject((char*)"models/Pen.obj", glm::vec4(255.0, 255.0, 0.0, 1.0), Verts, Idcs, VertCount, 7);
	createVAOs(Verts, Idcs, 7);
	PenVerts = Verts;
	PenVertCount = VertCount;

	loadObject((char*)"models/Button.obj", glm::vec4(255.0, 0.0, 0.0, 1.0), Verts, Idcs, VertCount, 8);
	createVAOs(Verts, Idcs, 8);
	ButtonVerts = Verts;
	ButtonVertCount = VertCount;

	loadObject((char*)"models/Projectile.obj", glm::vec4(125, 0, 125, 1.0), Verts, Idcs, VertCount, 9);
	createVAOs(Verts, Idcs, 9);
	ProjVerts = Verts;
	ProjVertCount = VertCount;

	InitProjVerts = new Vertex[ProjVertCount];

	for (int i = 0; i < ProjVertCount; i++) {
		InitProjVerts[i].SetPosition(ProjVerts[i].Position);
		InitProjVerts[i].SetColor(ProjVerts[i].Color);
		InitProjVerts[i].SetNormal(ProjVerts[i].Normal);
	}
}

void pickObject(void) {
	// Clear the screen in white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pickingProgramID);
	{

		glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);

		// ATTN: DRAW YOUR PICKING SCENE HERE. REMEMBER TO SEND IN A DIFFERENT PICKING COLOR FOR EACH OBJECT BEFOREHAND
		glBindVertexArray(0);
	}
	glUseProgram(0);
	// Wait until all the pending drawing commands are really done.
	// Ultra-mega-over slow ! 
	// There are usually a long time between glDrawElements() and
	// all the fragments completely rasterized.
	glFlush();
	glFinish();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Read the pixel at the center of the screen.
	// You can also use glfwGetMousePos().
	// Ultra-mega-over slow too, even for 1 pixel, 
	// because the framebuffer is on the GPU.
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];
	glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data); // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);

	if (gPickedIndex == 255) { // Full white, must be the background !
		gMessage = "background";
	}
	else {
		std::ostringstream oss;
		oss << "mesh " << gPickedIndex;
		gMessage = oss.str();
	}

	// Uncomment these lines to see the picking shader in effect
	//glfwSwapBuffers(window);
	//continue; // skips the normal rendering
}

float t = 0, minY = 200;
int ind = 0;

vec4 P0;
vec4 P2;
vec4 P1;
float startTimeL = 0;

void bezierProjectile(float deltatime, mat4& ModelMatrix) {
	//glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

	vec4 ProjPos = vec4(avgPos(ProjVerts, ProjVertCount), 1);

	if (minY <= ModelMatrix[3][1]) {
		sPress = false;
		ProjPos = ModelMatrix * ProjPos;
		vec3 tpos = vec3(ProjPos);
		moveBase(tpos, deltatime);
		minY = 200, ind = 0, t = 0;
		return;
	}

	float Cx = P1.x + (1 - t) * (1 - t) * (P0.x - P1.x) + t * t * (P2.x - P1.x);
	float Cy = P1.y + (1 - t) * (1 - t) * (P0.y - P1.y) + t * t * (P2.y - P1.y);
	float Cz = P1.z + (1 - t) * (1 - t) * (P0.z - P1.z) + t * t * (P2.z - P1.z);

	BezierVerts[ind].SetPosition(new float[4] {Cx, Cy, Cz, 1});
	//vec3 ProjPos = avgPos(ProjVerts, ProjVertCount);
	float dx = Cx - ProjPos.x, dy = Cy - ProjPos.y, dz = Cz - ProjPos.z;
	minY = std::min(minY, Cy);
	//cout << "Cuve coords: " << Cx << " " << Cy << " " << Cz << endl;

	for (int i = 0; i < ProjVertCount; i++) {
		ProjVerts[i].Position[0] += dx;
		ProjVerts[i].Position[1] += dy;
		ProjVerts[i].Position[2] += dz;
	}

	//cout << ProjVerts[0].Position[0] << " " << ProjVerts[0].Position[1] << " " << ProjVerts[0].Position[2] << endl;

	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[9]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[9], ProjVerts, GL_STATIC_DRAW);

	ind++;
	t += 0.001 * 0.5 * 9.8 * (glfwGetTime() - startTimeL);
}


void launchProjectile(float deltatime, mat4& ModelMatrix) {
	vec4 Position1 = vec4(PenVerts[7].Position[0], PenVerts[7].Position[1], PenVerts[7].Position[2], 1);
	vec4 Position2 = vec4(PenVerts[16].Position[0], PenVerts[16].Position[1], PenVerts[16].Position[2], 1);
	float theta = atan(Position1.y - Position2.y / Position1.x - Position2.x);
	float phi = atan(Position1.z - Position2.z / Position1.x - Position2.x);
	float vx = sin(theta) * cos(phi), vy = sin(theta) * 0, vz = cos(theta) * cos(phi);
	float t = (float)glfwGetTime() - startTimeL;

	float x = Position1.x + vx * t;
	float y = Position1.y + vy * t + 0.5 * 9.8 * t * t;
	float z = Position1.z + vz * t;
	float dx = x - Position1.x, dy = -y + Position1.y, dz = z - Position1.z;
	dx *= deltatime, dy *= deltatime, dz *= deltatime;

	vec4 dtrans = vec4(dx, dy, dz, 1);
	for (int i = 0; i < ProjVertCount; i++) {

		vec4 trans = vec4(ProjVerts[i].Position[0] + dtrans.x, ProjVerts[i].Position[1] + dtrans.y, ProjVerts[i].Position[2] + dtrans.z, 1);

		ProjVerts[i].Position[0] = trans.x;
		ProjVerts[i].Position[1] = trans.y;
		ProjVerts[i].Position[2] = trans.z;

		if (trans.y <= 0) {
			sPress = false;
			trans = ModelMatrix * vec4(trans);
			vec3 ttrans = vec(trans);
			moveBase(ttrans, deltatime);
			break;
		}
	}

	/*cout << ProjVerts[0].Position[0] << " " << ProjVerts[1].Position[0] << " " << ProjVerts[2].Position[0] << endl;*/
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[9]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[9], ProjVerts, GL_STATIC_DRAW);

}

void moveTop(float deltatime, mat4& ModelMatrix) {

	if (glfwGetKey(window, GLFW_KEY_LEFT)) {
		gOrientationTop.y -= radians(90.f) * deltatime;
	}

	else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
		gOrientationTop.y += radians(90.f) * deltatime;
	}
}

void moveArm1(float deltatime) {
	if (glfwGetKey(window, GLFW_KEY_LEFT)) {
		float dx = gOrientationArm1.x - radians(90.f) * deltatime;
		if (dx >= radians(-90.f) && dx <= radians(90.f))
			gOrientationArm1.x = dx;
	}

	else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
		float dx = gOrientationArm1.x + radians(90.f) * deltatime;
		if (dx >= radians(-90.f) && dx <= radians(90.f))
			gOrientationArm1.x = dx;
	}
}

void moveArm2(float deltatime) {
	if (glfwGetKey(window, GLFW_KEY_LEFT)) {
		float dx = gOrientationArm2.x - radians(90.f) * deltatime;
		if (dx >= radians(-45.f) && dx <= radians(225.f))
			gOrientationArm2.x = dx;
	}

	else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
		float dx = gOrientationArm2.x + radians(90.f) * deltatime;
		if (dx >= radians(-45.f) && dx <= radians(225.f))
			gOrientationArm2.x = dx;
	}
}

void renderScene(float deltaTime) {
	//ATTN: DRAW YOUR SCENE HERE. MODIFY/ADAPT WHERE NECESSARY!

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);
	// Re-clear the screen for real rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);
	{
		glm::mat4x4 ModelMatrix = glm::mat4(1.0);
		glm::vec3 lightPos = vec3(cameraPos.x - 2, cameraPos.y, cameraPos.z - 2);
		vec3 lightPos2 = vec3(cameraPos.x + 2, cameraPos.y, cameraPos.z - 2);
		vec3 lightPosArray[2] = { lightPos, lightPos2 };
		glUniform3fv(LightID, 2, (GLfloat*)lightPosArray);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		glBindVertexArray(VertexArrayId[0]);	// Draw CoordAxes
		glDrawArrays(GL_LINES, 0, NumVerts[0]);

		glBindVertexArray(VertexArrayId[1]);	// Draw Grid
		glDrawArrays(GL_LINES, 0, NumVerts[1]);
		//glDrawArrays(GL_POINTS, 0, NumVerts[1]);

		glBindVertexArray(VertexArrayId[2]);	// Draw Vertices
		//glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		glDrawElements(GL_TRIANGLES, NumIdcs[2], GL_UNSIGNED_SHORT, (void*)0);

		vec3 trans = avgPos(TopVerts, TopVertCount);
		glm::mat4 TranslationMatrix = translate(mat4(), -trans); // A bit to the left
		glm::mat4 TranslationMatrix1 = translate(mat4(), trans);

		if (tPress) {
			moveTop(deltaTime, ModelMatrix);
		}

		mat4 RotationMatrix = eulerAngleYXZ(gOrientationTop.y, gOrientationTop.x, gOrientationTop.z);
		ModelMatrix *= TranslationMatrix1 * RotationMatrix * TranslationMatrix;
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		glBindVertexArray(VertexArrayId[3]);	// Draw Vertices
		glDrawElements(GL_TRIANGLES, NumIdcs[3], GL_UNSIGNED_SHORT, (void*)0);

		trans = avgPos(TopVerts, TopVertCount);
		TranslationMatrix = translate(mat4(), -trans);
		TranslationMatrix1 = translate(mat4(), trans);

		if (onePress) {
			moveArm1(deltaTime);
		}

		RotationMatrix = eulerAngleYXZ(gOrientationArm1.y, gOrientationArm1.x, gOrientationArm1.z);
		ModelMatrix *= TranslationMatrix1 * RotationMatrix * TranslationMatrix;
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		glBindVertexArray(VertexArrayId[4]);	// Draw Vertices
		glDrawElements(GL_TRIANGLES, NumIdcs[4], GL_UNSIGNED_SHORT, (void*)0);

		trans = avgPos(JointVerts, JointVertCount);
		TranslationMatrix = translate(mat4(), -trans);
		TranslationMatrix1 = translate(mat4(), trans);

		if (twoPress) {
			moveArm2(deltaTime);
		}

		RotationMatrix = eulerAngleYXZ(gOrientationArm2.y, gOrientationArm2.x, gOrientationArm2.z);
		ModelMatrix *= TranslationMatrix1 * RotationMatrix * TranslationMatrix;
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		glBindVertexArray(VertexArrayId[5]);	// Draw Vertices
		glDrawElements(GL_TRIANGLES, NumIdcs[5], GL_UNSIGNED_SHORT, (void*)0);

		glBindVertexArray(VertexArrayId[6]);	// Draw Vertices
		glDrawElements(GL_TRIANGLES, NumIdcs[6], GL_UNSIGNED_SHORT, (void*)0);

		trans = vec3(PenVerts[22].Position[0], PenVerts[22].Position[1], PenVerts[22].Position[2]);
		TranslationMatrix = translate(mat4(), -trans);
		TranslationMatrix1 = translate(mat4(), trans);

		if (pPress) {
			rotatePen(deltaTime, ModelMatrix);
		}

		RotationMatrix = eulerAngleYXZ(gOrientationPen.y, gOrientationPen.x, gOrientationPen.z);
		ModelMatrix *= TranslationMatrix1 * RotationMatrix * TranslationMatrix;
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		glBindVertexArray(VertexArrayId[7]);	// Draw Vertices
		glDrawElements(GL_TRIANGLES, NumIdcs[7], GL_UNSIGNED_SHORT, (void*)0);

		glBindVertexArray(VertexArrayId[8]);	// Draw Vertices
		glDrawElements(GL_TRIANGLES, NumIdcs[8], GL_UNSIGNED_SHORT, (void*)0);

		if (sPress) {

			//launchProjectile(deltaTime, ModelMatrix);
			bezierProjectile(deltaTime, ModelMatrix);
			glBindVertexArray(VertexArrayId[9]);	// Draw Vertices
			glDrawElements(GL_TRIANGLES, NumIdcs[9], GL_UNSIGNED_SHORT, (void*)0);
		}
		glBindVertexArray(0);
	}
	glUseProgram(0);
	// Draw GUI
	//TwDraw();

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void cleanup(void) {
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

void translateObject(Vertex*& Verts, vec3 trans, int ObjectId, size_t VertCount) {
	for (int i = 0; i < VertCount; i++) {
		vec4 vec(Verts[i].Position[0] + trans[0], Verts[i].Position[1] + trans[1], Verts[i].Position[2] + trans[2], Verts[i].Position[3]);
		Verts[i].SetPosition(new float[4] {vec[0], vec[1], vec[2], vec[3]});
	}

	if (ObjectId == -1) return;
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Verts, GL_STATIC_DRAW);
}

vec3 avgPos(Vertex* Verts, const size_t VertCount) {
	float sx = 0, sy = 0, sz = 0;

	for (int i = 0; i < VertCount; i++) {
		sx += Verts[i].Position[0];
		sy += Verts[i].Position[1];
		sz += Verts[i].Position[2];
	}

	return vec3(sx / VertCount, sy / VertCount, sz / VertCount);
}

void rotatePen(float deltaTime, mat4& ModelMatrix) {
	if (!pPress) return;

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)) {
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			gOrientationPen.z -= radians(90.f) * deltaTime;
		}

		else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			gOrientationPen.z += radians(90.f) * deltaTime;
		}
	}

	else {
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			gOrientationPen.y -= radians(90.f) * deltaTime;
		}

		else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			gOrientationPen.y += radians(90.f) * deltaTime;
		}

		if (glfwGetKey(window, GLFW_KEY_UP)) {
			gOrientationPen.x -= radians(90.f) * deltaTime;
		}

		else if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			gOrientationPen.x += radians(90.f) * deltaTime;
		}
	}
}

void moveBase(vec3& trans, float deltatime) {
	trans = trans - avgPos(BaseVerts, BaseVertCount);
	trans.y = 0;

	//trans *= deltatime;
	translateObject(BaseVerts, trans, 2, BaseVertCount);
	translateObject(TopVerts, trans, 3, TopVertCount);
	translateObject(Arm1Verts, trans, 4, Arm1VertCount);
	translateObject(JointVerts, trans, 5, JointVertCount);
	translateObject(Arm2Verts, trans, 6, Arm2VertCount);
	translateObject(PenVerts, trans, 7, PenVertCount);
	translateObject(ButtonVerts, trans, 8, ButtonVertCount);
	translateObject(ProjVerts, trans, 9, ProjVertCount);
	translateObject(InitProjVerts, trans, -1, ProjVertCount);
}

void moveBase(float deltatime) {
	if (!bPress) return;

	vec3 trans = vec3();

	if (glfwGetKey(window, GLFW_KEY_LEFT)) {
		trans = vec3(-1.0f, 0.0f, 0.0f) * deltatime * 5.f;
	}

	else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
		trans = vec3(1.0f, 0.0f, 0.0f) * deltatime * 5.f;
	}

	if (glfwGetKey(window, GLFW_KEY_UP)) {
		trans = vec3(0.0f, 0.0f, -1.0f) * deltatime * 5.f;
	}

	else if (glfwGetKey(window, GLFW_KEY_DOWN)) {
		trans = vec3(0.0f, 0.0f, 1.0f) * deltatime * 5.f;
	}

	translateObject(BaseVerts, trans, 2, BaseVertCount);
	translateObject(TopVerts, trans, 3, TopVertCount);
	translateObject(Arm1Verts, trans, 4, Arm1VertCount);
	translateObject(JointVerts, trans, 5, JointVertCount);
	translateObject(Arm2Verts, trans, 6, Arm2VertCount);
	translateObject(PenVerts, trans, 7, PenVertCount);
	translateObject(ButtonVerts, trans, 8, ButtonVertCount);
	translateObject(ProjVerts, trans, 9, ProjVertCount);
	translateObject(InitProjVerts, trans, -1, ProjVertCount);
}

void resetProjectile() {
	for (int i = 0; i < ProjVertCount; i++) {
		ProjVerts[i].SetPosition(InitProjVerts[i].Position);
	}
}

float prevX = 10.f, prevZ = 10.f, prevY = 10.f;
// Alternative way of triggering functions on keyboard events
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	// ATTN: MODIFY AS APPROPRIATE
	if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_P:
			pPress = !pPress;
			changeOpacity(PenVerts, PenVertCount, pPress, 7);
			break;
		case GLFW_KEY_B:
			bPress = !bPress;
			changeOpacity(BaseVerts, BaseVertCount, bPress, 2);
			break;
		case GLFW_KEY_C:
			cPress = !cPress;
			break;
		case GLFW_KEY_S:
			resetProjectile();
			sPress = !sPress;
			if (!sPress) {
				minY = 100;
				ind = 0;
				t = 0;
			}
			else {
				P0 = vec4(PenVerts[7].Position[0], PenVerts[7].Position[1], PenVerts[7].Position[2], 1);
				P2 = vec4(P0.x, P0.y - 20, P0.z, 1);
				P1 = vec4(P0.x, P0.y - 2, P0.z - 10, 1);
				startTimeL = glfwGetTime();
			}
			break;
		case GLFW_KEY_T:
			tPress = !tPress;
			changeOpacity(TopVerts, TopVertCount, tPress, 3);
			break;
		case GLFW_KEY_1:
			onePress = !onePress;
			changeOpacity(Arm1Verts, Arm1VertCount, onePress, 4);
			break;
		case GLFW_KEY_2:
			twoPress = !twoPress;
			changeOpacity(Arm2Verts, Arm2VertCount, twoPress, 6);
			break;
		default:
			break;
		}
	}
}

// Alternative way of triggering functions on mouse click events
static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		pickObject();
	}
}

float theta = atan(1), phi = atan(1);
const float radius = 10.0f;

void updateLight() {
	glUseProgram(programID);
	{
		glm::vec3 lightPos = vec3(cameraPos.x - 2, cameraPos.y, cameraPos.z - 2);
		vec3 lightPos2 = vec3(cameraPos.x + 2, cameraPos.y, cameraPos.z - 2);
		vec3 lightPosArray[2] = { lightPos, lightPos2 };
		glUniform3fv(LightID, 2, (GLfloat*)lightPosArray);
	}
	glUseProgram(0);
}

void changeOpacity(Vertex* Verts, const size_t VertCount, bool f, int ObjectId) {
	for (int i = 0; i < VertCount; i++) {
		if (f)
			Verts[i].SetColor(new float[4] {Verts[i].Color[0] + 100, Verts[i].Color[1] + 100, Verts[i].Color[2] + 100, 1});
		else
			Verts[i].SetColor(new float[4] {Verts[i].Color[0] - 100, Verts[i].Color[1] - 100, Verts[i].Color[2] - 100, 1});
	}

	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Verts, GL_STATIC_DRAW);
}

void moveCamera() {
	if (!cPress) return;

	if (glfwGetKey(window, GLFW_KEY_LEFT)) {
		theta -= radians(0.1f);
		prevX = 0.0 + sin(theta) * cos(phi) * radius;
		prevZ = 0.0 + cos(theta) * cos(phi) * radius;
		cameraPos = vec3(prevX, prevY, prevZ);
		gViewMatrix = glm::lookAt(glm::vec3(prevX, prevY, prevZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
		theta += radians(0.1f);
		prevX = 0.0 + sin(theta) * cos(phi) * radius;
		prevZ = 0.0 + cos(theta) * cos(phi) * radius;
		cameraPos = vec3(prevX, prevY, prevZ);
		gViewMatrix = glm::lookAt(glm::vec3(prevX, prevY, prevZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	}

	if (glfwGetKey(window, GLFW_KEY_UP)) {
		phi += radians(0.1f);
		prevY = 0.0 + sin(phi) * radius;
		prevZ = 0.0 + cos(theta) * cos(phi) * radius;
		cameraPos = vec3(prevX, prevY, prevZ);
		gViewMatrix = glm::lookAt(glm::vec3(prevX, prevY, prevZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN)) {
		phi -= radians(0.1f);
		prevY = 0.0 + sin(phi) * radius;
		prevZ = 0.0 + cos(theta) * cos(phi) * radius;
		cameraPos = vec3(prevX, prevY, prevZ);
		gViewMatrix = glm::lookAt(glm::vec3(prevX, prevY, prevZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	}
	updateLight();
}

int main(void) {
	// TL
	// ATTN: Refer to https://learnopengl.com/Getting-started/Transformations, https://learnopengl.com/Getting-started/Coordinate-Systems,
	// and https://learnopengl.com/Getting-started/Camera to familiarize yourself with implementing the camera movement

	// ATTN (Project 3 only): Refer to https://learnopengl.com/Getting-started/Textures to familiarize yourself with mapping a texture
	// to a given mesh

	// Initialize window
	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;

	// Initialize OpenGL pipeline
	initOpenGL();

	double lastTime = 0;

	// For speed computation
	//double lastTime = glfwGetTime();
	int nbFrames = 0;
	do {
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		double deltaTime = currentTime - lastTime;
		if (deltaTime >= 1.0) { // If last prinf() was more than 1sec ago
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		if (cPress) {
			moveCamera();
		}

		if (bPress) {
			moveBase(deltaTime);
		}
		// DRAWING POINTS
		renderScene(deltaTime);
		lastTime = currentTime;

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}