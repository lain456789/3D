#define FREEGLUT_STATIC
#include <GL/freeglut.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <stdio.h>
#pragma comment (linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

// ==============================================================================
// 1. Data Structures
// ==============================================================================

// Building structure
struct Building {
    float x, y, z;                // Position coordinates
    float length, height, width;  // Dimensions
    float glowColor[3];           // RGB color for glowing edges
    int windowRows;               // Number of window rows
    int windowCols;               // Number of window columns
};

// Vehicle structure
struct Vehicle {
    float xPos;                   // X position
    float zPos;                   // Z position
    int moveDirection;            // Movement direction (1 or -1)
    Vehicle(float initX, float initZ, int direction)
        : xPos(initX), zPos(initZ), moveDirection(direction) {
    }
};

// ==============================================================================
// 2. Global Variable Declarations
// ==============================================================================

const float M_PI = 3.14159f;      // Approximate value of π

// Help text displayed on screen
const char* helpText[] = {
    "Key function description:",
    "h: Show/Hide help",
    "WSAD: Move the camera position",
    "Mouse drag(left click and drag): Rotate the view",
    "Mouse wheel: Zoom in and out of view",
    "1-3: Switch the tunnel display section",
    "v: Switch perspective mode",
    "R,G,B: change the color of the light source",
    "C: change the position of the light source",
    NULL
};

bool showHelp = false;            // Toggle for displaying help text
int mainWindowWidth = 800;        // Initial window width
int mainWindowHeight = 600;       // Initial window height

#define TEXTURE_CHESSBOARD 2      // Index for chessboard texture

// Tunnel parameters
const float TORUS_OUTER_RADIUS = 8.0f;   // Outer radius of torus tunnel
const float TORUS_INNER_RADIUS = 5.0f;   // Inner radius of torus tunnel
const float TUNNEL_DIAMETER = TORUS_OUTER_RADIUS - TORUS_INNER_RADIUS;
const float TUNNEL_RADIUS = TUNNEL_DIAMETER / 2.0f;
const float TUNNEL_HEIGHT = 30.0f;       // Height of tunnel above ground
const int   TUNNEL_SEGMENTS = 60;        // Segmentation for tunnel smoothness
const float TUNNEL_START_XZ = -50.0f;    // Starting coordinate for linear tunnels
const float TUNNEL_END_XZ = 50.0f;       // Ending coordinate for linear tunnels
const float TUNNEL_LENGTH = TUNNEL_END_XZ - TUNNEL_START_XZ;

// Scene object collections
#define NUM_BUILDINGS 40          // Total number of buildings
Building buildings[NUM_BUILDINGS]; // Array of building objects
std::vector<Vehicle> vehicles;    // Dynamic list of moving vehicles

// Camera parameters for default view
float camX = 0.0f, camY = 40.0f, camZ = 60.0f; // Camera position
float angle = 0.0f;               // Horizontal rotation angle
float verticalAngle = 0.0f;       // Vertical rotation angle

// View mode control
int viewMode = 0;                 // 0: default view, 1: fixed view
const float fixedCamX = 0.0f, fixedCamY = 5.0f, fixedCamZ = -50.0f; // Fixed camera position

// Scene offset for moving view mode
float sceneZOffset = 0.0f;        // Z-axis offset for scene movement
const float sceneMoveSpeed = 0.4f; // Speed of scene movement

// Lighting and material parameters
GLfloat light_x = 1.0f, light_y = 1.0f, light_z = 1.0f; // Light position
GLfloat light_diffuse_color[] = { 0.8f, 0.8f, 0.8f, 1.0f }; // Diffuse light color
GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };    // Material specular reflection
GLfloat mat_shininess[] = { 50.0f };                    // Material shininess
GLfloat diffuseMaterial[] = { 0.5f, 0.5f, 0.5f, 1.0f }; // Default diffuse material

// Animation control variables
float animationTime = 0.0f;       // Global time for animations
float billboardRotation1 = 0.0f;  // Rotation angle for first billboard
float billboardRotation2 = 0.0f;  // Rotation angle for second billboard
float colorSphereAngle = 0.0f;    // Rotation angle for colored sphere
float whiteSphereAngle = 0.0f;    // Rotation angle for white sphere
float diamondRotation = 0.0f;     // Rotation angle for diamond decoration

// Interactive control variables
int mouseX = 0, mouseY = 0;       // Mouse coordinates
bool mouseLeftPressed = false;    // Left mouse button state
bool showOverlap = false;         // Flag to show tunnel overlap (unused in current version)
int tunnelDisplayMode = 1;        // Tunnel display mode (1,2,3)
bool showDiamond = true;          // Toggle for diamond decoration
const float vehicleMoveSpeed = 0.4f; // Speed of vehicle movement

// Texture related variables
GLuint textures[3];               // Array of texture IDs
GLint imageWidth[2], imageHeight[2], pixelLength[2]; // Texture dimensions
std::vector<GLubyte*> pixelData; // Vector storing pixel data for textures

// Special material presets
GLfloat whiteSphere_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat whiteSphere_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat whiteSphere_shininess[] = { 50.0f };
GLfloat textureSphere_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat textureSphere_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat textureSphere_shininess[] = { 50.0f };

// Bezier surface control points
GLfloat bezierControlPoints[4][4][3] = {
    {{-8.0f, 0.0f, -4.0f}, {-4.0f, 2.0f, -4.0f}, {4.0f, 2.0f, -4.0f}, {8.0f, 0.0f, -4.0f}},
    {{-8.0f, 1.0f, 0.0f}, {-4.0f, 3.0f, 1.0f}, {4.0f, 3.0f, -1.0f}, {8.0f, 1.0f, 0.0f}},
    {{-8.0f, 1.0f, 0.0f}, {-4.0f, 3.0f, -1.0f}, {4.0f, 3.0f, 1.0f}, {8.0f, 1.0f, 0.0f}},
    {{-8.0f, 0.0f, 4.0f}, {-4.0f, 2.0f, 4.0f}, {4.0f, 2.0f, 4.0f}, {8.0f, 0.0f, 4.0f}}
};

// EVA-01 color definitions
GLfloat eva01_purple[] = { 0.4f, 0.1f, 0.5f, 1.0f };
GLfloat eva01_green[] = { 0.1f, 0.8f, 0.2f, 1.0f };
GLfloat eva01_black[] = { 0.1f, 0.1f, 0.1f, 1.0f };
GLfloat eva01_yellow[] = { 0.9f, 0.8f, 0.1f, 1.0f };
GLfloat eva01_white[] = { 0.9f, 0.9f, 0.9f, 1.0f };
GLfloat eva01_red[] = { 0.8f, 0.1f, 0.1f, 1.0f };

// EVA robot joint angles (for animation)
GLfloat neck_angle = 0.0f;
GLfloat left_shoulder_angle = 0.0f;
GLfloat right_shoulder_angle = 0.0f;
GLfloat left_elbow_angle = 0.0f;
GLfloat right_elbow_angle = 0.0f;
GLfloat left_hip_angle = 0.0f;
GLfloat right_hip_angle = 0.0f;
GLfloat left_knee_angle = 0.0f;
GLfloat right_knee_angle = 0.0f;

// ==============================================================================
// 3. Model Data (Capsule patch and control points)
// ==============================================================================

static int patchdata[][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27},
    {24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39},
    {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 0, 1, 2, 3},
    {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63},
    {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 48, 49, 50, 51},
    {72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87},
    {84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 72, 73, 74, 75}
};

static float cpdata[][3] = {
    // Main cylinder body
    {1.0, -2.0, 0.0}, {0.707, -2.0, 0.707}, {0.0, -2.0, 1.0}, {-0.707, -2.0, 0.707},
    {1.0, -2.0, 0.0}, {0.707, -2.0, 0.707}, {0.0, -2.0, 1.0}, {-0.707, -2.0, 0.707},
    {1.0, -2.0, 0.0}, {0.707, -2.0, 0.707}, {0.0, -2.0, 1.0}, {-0.707, -2.0, 0.707},
    {1.0, -2.0, 0.0}, {0.707, -2.0, 0.707}, {0.0, -2.0, 1.0}, {-0.707, -2.0, 0.707},
    {1.0, -1.0, 0.0}, {0.707, -1.0, 0.707}, {0.0, -1.0, 1.0}, {-0.707, -1.0, 0.707},
    {1.0, -1.0, 0.0}, {0.707, -1.0, 0.707}, {0.0, -1.0, 1.0}, {-0.707, -1.0, 0.707},
    {1.0, -1.0, 0.0}, {0.707, -1.0, 0.707}, {0.0, -1.0, 1.0}, {-0.707, -1.0, 0.707},
    {1.0, -1.0, 0.0}, {0.707, -1.0, 0.707}, {0.0, -1.0, 1.0}, {-0.707, -1.0, 0.707},
    {1.0, 1.0, 0.0}, {0.707, 1.0, 0.707}, {0.0, 1.0, 1.0}, {-0.707, 1.0, 0.707},
    {1.0, 1.0, 0.0}, {0.707, 1.0, 0.707}, {0.0, 1.0, 1.0}, {-0.707, 1.0, 0.707},
    {1.0, 1.0, 0.0}, {0.707, 1.0, 0.707}, {0.0, 1.0, 1.0}, {-0.707, 1.0, 0.707},
    {1.0, 1.0, 0.0}, {0.707, 1.0, 0.707}, {0.0, 1.0, 1.0}, {-0.707, 1.0, 0.707},
    {1.0, 2.0, 0.0}, {0.707, 2.0, 0.707}, {0.0, 2.0, 1.0}, {-0.707, 2.0, 0.707},
    {1.0, 2.0, 0.0}, {0.707, 2.0, 0.707}, {0.0, 2.0, 1.0}, {-0.707, 2.0, 0.707},
    {1.0, 2.0, 0.0}, {0.707, 2.0, 0.707}, {0.0, 2.0, 1.0}, {-0.707, 2.0, 0.707},
    {1.0, 2.0, 0.0}, {0.707, 2.0, 0.707}, {0.0, 2.0, 1.0}, {-0.707, 2.0, 0.707},
    // Upper hemisphere
    {0.0, 3.0, 0.0}, {0.0, 3.0, 0.0}, {0.0, 3.0, 0.0}, {0.0, 3.0, 0.0},
    {1.0, 2.0, 0.0}, {0.707, 2.0, 0.707}, {0.0, 2.0, 1.0}, {-0.707, 2.0, 0.707},
    {1.0, 2.5, 0.0}, {0.707, 2.5, 0.707}, {0.0, 2.5, 1.0}, {-0.707, 2.5, 0.707},
    {0.5, 2.866, 0.0}, {0.353, 2.866, 0.353}, {0.0, 2.866, 0.5}, {-0.353, 2.866, 0.353},
    {0.0, 3.0, 0.0}, {0.0, 3.0, 0.0}, {0.0, 3.0, 0.0}, {0.0, 3.0, 0.0},
    {-0.707, 2.0, 0.707}, {-1.0, 2.0, 0.0}, {-0.707, 2.0, -0.707}, {0.0, 2.0, -1.0},
    {-0.707, 2.5, 0.707}, {-1.0, 2.5, 0.0}, {-0.707, 2.5, -0.707}, {0.0, 2.5, -1.0},
    {-0.353, 2.866, 0.353}, {-0.5, 2.866, 0.0}, {-0.353, 2.866, -0.353}, {0.0, 2.866, -0.5},
    // Lower hemisphere
    {0.0, -3.0, 0.0}, {0.0, -3.0, 0.0}, {0.0, -3.0, 0.0}, {0.0, -3.0, 0.0},
    {1.0, -2.0, 0.0}, {0.707, -2.0, 0.707}, {0.0, -2.0, 1.0}, {-0.707, -2.0, 0.707},
    {1.0, -2.5, 0.0}, {0.707, -2.5, 0.707}, {0.0, -2.5, 1.0}, {-0.707, -2.5, 0.707},
    {0.5, -2.866, 0.0}, {0.353, -2.866, 0.353}, {0.0, -2.866, 0.5}, {-0.353, -2.866, 0.353},
    {0.0, -3.0, 0.0}, {0.0, -3.0, 0.0}, {0.0, -3.0, 0.0}, {0.0, -3.0, 0.0},
    {-0.707, -2.0, 0.707}, {-1.0, -2.0, 0.0}, {-0.707, -2.0, -0.707}, {0.0, -2.0, -1.0},
    {-0.707, -2.5, 0.707}, {-1.0, -2.5, 0.0}, {-0.707, -2.5, -0.707}, {0.0, -2.5, -1.0},
    {-0.353, -2.866, 0.353}, {-0.5, -2.866, 0.0}, {-0.353, -2.866, -0.353}, {0.0, -2.866, -0.5}
};

// Texture coordinates for capsule
static float tex[2][2][2] = {
    { {0.0f, 0.0f}, {1.0f, 0.0f} },
    { {0.0f, 1.0f}, {1.0f, 1.0f} }
};

// ==============================================================================
// 4. Function Prototypes
// ==============================================================================
// Initialization and utility functions
void initLightingAndMaterial();
void initVehicles();
void initBuildings();
void readMap(const char* path, int index);
void reShape(int w, int h);

// Mathematical calculations
void calculateBezierPoint(float t, GLfloat p0[3], GLfloat p1[3], GLfloat p2[3], GLfloat p3[3], GLfloat result[3]);
void calculateBezierSurfacePoint(float u, float v, GLfloat result[3]);

// Basic drawing components
static void capsule(GLint grid, GLfloat scale, GLenum type);
void wireCapsule(GLfloat scale);
void solidCapsule(GLfloat scale);
void makeChessboardTexture();
void drawGrid();
void drawWindows(float x, float y, float z, float length, float height, float width, int rows, int cols);
void drawGlowingEdges(float x, float y, float z, float length, float height, float width, float glowColor[3]);
void drawFutureBuilding(float x, float y, float z, float length, float height, float width, float glowColor[3], int windowRows, int windowCols);
void DrawBuildingRoof(int buildingIndex);

// Scene element drawing functions
void draw_spike(float radius, float height, int segments);
void draw_eva_arm(int is_left);
void draw_eva_leg(int is_left);
void draw_eva_backpack();
void draw_eva_robot();
void drawVehicle(const Vehicle& vehicle);
void updateVehicles();
void drawBezierBillboardForBuilding(int buildingIndex, float& rotationAngle);
void drawBezierBillboard_Quad1_Building8();
void drawBezierBillboard_Quad2_Building8();
void drawSuspendedTorus();
void drawXAxisCylinderTunnel();
void drawZAxisCylinderTunnel();
void drawWhiteSolidSphere();
void drawDiamondDecoration();
void drawSolidCapsuleOnQuad1Building6();
void drawBuildings();

// Callback functions
void processNormalKeys(unsigned char key, int x, int y);
void mouseButton(int button, int state, int x, int y);
void mouseMove(int x, int y);
void mouseWheel(int wheel, int direction, int x, int y);
void when_in_mainloop();
void renderScene(void);

// Help text drawing
void drawTextAtCorner();

// ==============================================================================
// 5. Initialization and Loading Implementation
// ==============================================================================

// Initialize lighting and material properties
void initLightingAndMaterial() {
    GLfloat light_position[] = { light_x, light_y, light_z, 0.0f };
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light_diffuse[] = { light_diffuse_color[0], light_diffuse_color[1], light_diffuse_color[2], light_diffuse_color[3] };

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseMaterial);

    glColorMaterial(GL_FRONT, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
}

// Initialize vehicles with starting positions and directions
void initVehicles() {
    // Vehicles at X=1 moving in negative Z direction
    vehicles.emplace_back(1.0f, 50.0f, -1);
    vehicles.emplace_back(1.0f, 35.0f, -1);
    vehicles.emplace_back(1.0f, 20.0f, -1);
    vehicles.emplace_back(1.0f, 5.0f, -1);
    vehicles.emplace_back(1.0f, -10.0f, -1);
    vehicles.emplace_back(1.0f, -25.0f, -1);
    // Vehicles at X=-1 moving in positive Z direction
    vehicles.emplace_back(-1.0f, -50.0f, 1);
    vehicles.emplace_back(-1.0f, -40.0f, 1);
    vehicles.emplace_back(-1.0f, -30.0f, 1);
    vehicles.emplace_back(-1.0f, -20.0f, 1);
    vehicles.emplace_back(-1.0f, -15.0f, 1);
    vehicles.emplace_back(-1.0f, -10.0f, 1);
}

// Initialize building data with positions, dimensions, and colors
void initBuildings() {
    srand(time(NULL));
    float buildingData[40][6] = {
        // Quadrant 1
        {8.0f, 0.0f, 8.0f, 10.0f, 25.0f, 10.0f}, {8.0f, 0.0f, 28.0f, 20.0f, 20.0f, 10.0f},
        {8.0f, 0.0f, 46.0f, 6.0f, 30.0f, 10.0f}, {24.0f, 0.0f, 6.0f, 6.0f, 10.0f, 16.0f},
        {25.0f, 0.0f, 44.0f, 10.0f, 20.0f, 20.0f}, {19.0f, 0.0f, 18.0f, 10.0f, 35.0f, 6.0f},
        {18.0f, 0.0f, 30.0f, 6.0f, 24.0f, 6.0f}, {36.0f, 0.0f, 24.0f, 20.0f, 15.0f, 20.0f},
        {40.0f, 0.0f, 6.0f, 6.0f, 26.0f, 8.0f}, {44.0f, 0.0f, 44.0f, 8.0f, 28.0f, 8.0f},
        // Quadrant 2 (mirror of quadrant 1)
        {-8.0f, 0.0f, 8.0f, 10.0f, 25.0f, 10.0f}, {-8.0f, 0.0f, 28.0f, 20.0f, 20.0f, 10.0f},
        {-8.0f, 0.0f, 46.0f, 6.0f, 30.0f, 10.0f}, {-24.0f, 0.0f, 6.0f, 6.0f, 10.0f, 16.0f},
        {-25.0f, 0.0f, 44.0f, 10.0f, 20.0f, 20.0f}, {-19.0f, 0.0f, 18.0f, 10.0f, 35.0f, 6.0f},
        {-18.0f, 0.0f, 30.0f, 6.0f, 24.0f, 6.0f}, {-36.0f, 0.0f, 24.0f, 20.0f, 15.0f, 20.0f},
        {-40.0f, 0.0f, 6.0f, 6.0f, 26.0f, 8.0f}, {-44.0f, 0.0f, 44.0f, 8.0f, 28.0f, 8.0f},
        // Quadrant 3 (mirror)
        {-8.0f, 0.0f, -8.0f, 10.0f, 25.0f, 10.0f}, {-8.0f, 0.0f, -28.0f, 20.0f, 20.0f, 10.0f},
        {-8.0f, 0.0f, -46.0f, 6.0f, 30.0f, 10.0f}, {-24.0f, 0.0f, -6.0f, 6.0f, 10.0f, 16.0f},
        {-25.0f, 0.0f, -44.0f, 10.0f, 20.0f, 20.0f}, {-19.0f, 0.0f, -18.0f, 10.0f, 35.0f, 6.0f},
        {-18.0f, 0.0f, -30.0f, 6.0f, 24.0f, 6.0f}, {-36.0f, 0.0f, -24.0f, 20.0f, 15.0f, 20.0f},
        {-40.0f, 0.0f, -6.0f, 6.0f, 26.0f, 8.0f}, {-44.0f, 0.0f, -44.0f, 8.0f, 28.0f, 8.0f},
        // Quadrant 4 (mirror)
        {8.0f, 0.0f, -8.0f, 10.0f, 25.0f, 10.0f}, {8.0f, 0.0f, -28.0f, 20.0f, 20.0f, 10.0f},
        {8.0f, 0.0f, -46.0f, 6.0f, 30.0f, 10.0f}, {24.0f, 0.0f, -6.0f, 6.0f, 10.0f, 16.0f},
        {25.0f, 0.0f, -44.0f, 10.0f, 20.0f, 20.0f}, {19.0f, 0.0f, -18.0f, 10.0f, 35.0f, 6.0f},
        {18.0f, 0.0f, -30.0f, 6.0f, 24.0f, 6.0f}, {36.0f, 0.0f, -24.0f, 20.0f, 15.0f, 20.0f},
        {40.0f, 0.0f, -6.0f, 6.0f, 26.0f, 8.0f}, {44.0f, 0.0f, -44.0f, 8.0f, 28.0f, 8.0f}
    };

    for (int i = 0; i < NUM_BUILDINGS; i++) {
        buildings[i].x = buildingData[i][0];
        buildings[i].y = buildingData[i][1];
        buildings[i].z = buildingData[i][2];
        buildings[i].length = buildingData[i][3];
        buildings[i].height = buildingData[i][4];
        buildings[i].width = buildingData[i][5];

        // Calculate window grid based on building dimensions
        buildings[i].windowRows = (int)(buildings[i].height / 1.8f);
        buildings[i].windowCols = (int)(fmax(buildings[i].width, buildings[i].length) / 1.8f);
        if (buildings[i].windowRows < 5) buildings[i].windowRows = 5;
        if (buildings[i].windowCols < 5) buildings[i].windowCols = 5;

        // Generate random glow colors with blue bias
        buildings[i].glowColor[0] = 0.2f + (rand() % 80) / 100.0f;
        buildings[i].glowColor[1] = 0.4f + (rand() % 60) / 100.0f;
        buildings[i].glowColor[2] = 0.8f + (rand() % 20) / 100.0f;
    }
}

// Read texture image from file (supports batch loading)
void readMap(const char* path, int index) {
    FILE* pfile = NULL;
    fopen_s(&pfile, path, "rb");
    if (pfile == NULL) {
        printf("Error: Cannot open texture file %s\n", path);
        exit(0);
    }

    // Read image width and height from BMP file (offset 0x0012)
    fseek(pfile, 0x0012, SEEK_SET);
    fread(&imageWidth[index], sizeof(GLint), 1, pfile);
    fread(&imageHeight[index], sizeof(GLint), 1, pfile);

    // Calculate pixel data length (BMP uses BGR format, each row must be multiple of 4 bytes)
    pixelLength[index] = imageWidth[index] * 3;
    while (pixelLength[index] % 4 != 0) {
        pixelLength[index]++;
    }
    pixelLength[index] *= imageHeight[index];

    // Allocate memory for pixel data and read from file
    GLubyte* data = (GLubyte*)malloc(pixelLength[index]);
    if (data == NULL) {
        fclose(pfile);
        exit(0);
    }
    fseek(pfile, 54, SEEK_SET); // Skip 54-byte BMP header
    fread(data, pixelLength[index], 1, pfile);
    fclose(pfile);

    pixelData.push_back(data); // Store pixel data in vector

    // Initialize texture object
    glBindTexture(GL_TEXTURE_2D, textures[index]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, imageWidth[index], imageHeight[index], 0,
        GL_BGR_EXT, GL_UNSIGNED_BYTE, pixelData[index]);

    // Set texture parameters (CLAMP for roof texture, REPEAT for ground texture)
    if (index == 0) { // 1.bmp (roof): texture does not repeat
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    else { // 2.bmp (ground): texture repeats
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

// ==============================================================================
// 6. Core Functionality and Drawing Implementation
// ==============================================================================

// Calculate point on cubic Bezier curve
void calculateBezierPoint(float t, GLfloat p0[3], GLfloat p1[3], GLfloat p2[3], GLfloat p3[3], GLfloat result[3]) {
    float t2 = t * t, t3 = t2 * t;
    float b0 = (1 - t) * (1 - t) * (1 - t);
    float b1 = 3 * t * (1 - t) * (1 - t);
    float b2 = 3 * t2 * (1 - t);
    float b3 = t3;

    result[0] = b0 * p0[0] + b1 * p1[0] + b2 * p2[0] + b3 * p3[0];
    result[1] = b0 * p0[1] + b1 * p1[1] + b2 * p2[1] + b3 * p3[1];
    result[2] = b0 * p0[2] + b1 * p1[2] + b2 * p2[2] + b3 * p3[2];
}

// Calculate point on Bezier surface using 4x4 control points
void calculateBezierSurfacePoint(float u, float v, GLfloat result[3]) {
    GLfloat tempPoints[4][3];
    // Calculate intermediate points along u direction
    for (int i = 0; i < 4; i++) {
        calculateBezierPoint(u, bezierControlPoints[i][0], bezierControlPoints[i][1],
            bezierControlPoints[i][2], bezierControlPoints[i][3], tempPoints[i]);
    }
    // Calculate final point along v direction
    calculateBezierPoint(v, tempPoints[0], tempPoints[1], tempPoints[2], tempPoints[3], result);
}

// Core capsule drawing algorithm using Bezier patches
static void capsule(GLint grid, GLfloat scale, GLenum type) {
    float p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];
    long i, j, k, l;

    glPushAttrib(GL_ENABLE_BIT | GL_EVAL_BIT);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_MAP2_VERTEX_3);
    glEnable(GL_MAP2_TEXTURE_COORD_2);
    glPushMatrix();
    glScalef(0.5f * scale, 0.5f * scale, 0.5f * scale);

    // Process each of the 8 patches
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 4; k++) {
                for (l = 0; l < 3; l++) {
                    // Generate four symmetric patch variations
                    p[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
                    q[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
                    if (l == 1) q[j][k][l] *= -1.0;
                    r[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
                    if (l == 0) r[j][k][l] *= -1.0;
                    s[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
                    if (l == 0) s[j][k][l] *= -1.0;
                    if (l == 1) s[j][k][l] *= -1.0;
                }
            }
        }
        // Set up texture coordinates
        glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2, &tex[0][0][0]);
        glMapGrid2f(grid, 0.0, 1.0, grid, 0.0, 1.0);

        // Draw each patch variation
        glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &p[0][0][0]);
        glEvalMesh2(type, 0, grid, 0, grid);
        glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &q[0][0][0]);
        glEvalMesh2(type, 0, grid, 0, grid);
        glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &r[0][0][0]);
        glEvalMesh2(type, 0, grid, 0, grid);
        glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &s[0][0][0]);
        glEvalMesh2(type, 0, grid, 0, grid);
    }
    glPopMatrix();
    glPopAttrib();
}

// Draw wireframe capsule
void wireCapsule(GLfloat scale) {
    glColor3f(0.6f, 0.8f, 0.9f);
    capsule(10, scale, GL_LINE);
}

// Draw solid capsule
void solidCapsule(GLfloat scale) {
    glColor3f(0.8f, 0.2f, 0.5f);
    capsule(14, scale, GL_FILL);
}

// Create chessboard texture procedurally
void makeChessboardTexture() {
    const int size = 64;
    GLubyte chessboard[size][size][3];

    // Generate 8x8 chessboard pattern
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            bool isGray = (((i & 0x8) == 0) ^ ((j & 0x8) == 0));
            GLubyte color = isGray ? 0 : 25; // Dark gray and light gray
            chessboard[i][j][0] = color;
            chessboard[i][j][1] = color;
            chessboard[i][j][2] = color;
        }
    }

    // Initialize chessboard texture
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_CHESSBOARD]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, size, size, 0,
        GL_RGB, GL_UNSIGNED_BYTE, chessboard);

    // Set texture parameters for repeating pattern
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

// Draw ground grid with textures
void drawGrid() {
    glPushMatrix();
    glTranslatef(0.0f, -0.5f, 0.0f);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    float groundScale = 1000.0f;
    float texRepeat = 50.0f;

    // Draw center road with asphalt texture
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);     glVertex3f(-2.0f, 0.5f, -groundScale);
    glTexCoord2f(0.0f, texRepeat); glVertex3f(-2.0f, 0.5f, groundScale);
    glTexCoord2f(texRepeat / 50, texRepeat); glVertex3f(2.0f, 0.5f, groundScale);
    glTexCoord2f(texRepeat / 50, 0.0f);   glVertex3f(2.0f, 0.5f, -groundScale);
    glEnd();

    // Draw left side with chessboard texture
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_CHESSBOARD]);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);     glVertex3f(-groundScale, 0.5f, -groundScale);
    glTexCoord2f(0.0f, texRepeat); glVertex3f(-groundScale, 0.5f, groundScale);
    glTexCoord2f(texRepeat, texRepeat); glVertex3f(-2.0f, 0.5f, groundScale);
    glTexCoord2f(texRepeat, 0.0f);   glVertex3f(-2.0f, 0.5f, -groundScale);
    glEnd();

    // Draw right side with chessboard texture
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_CHESSBOARD]);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);     glVertex3f(2.0f, 0.5f, -groundScale);
    glTexCoord2f(0.0f, texRepeat); glVertex3f(2.0f, 0.5f, groundScale);
    glTexCoord2f(texRepeat, texRepeat); glVertex3f(groundScale, 0.5f, groundScale);
    glTexCoord2f(texRepeat, 0.0f);   glVertex3f(groundScale, 0.5f, -groundScale);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    // Draw ground sides (thick edges)
    glColor3f(0.15f, 0.15f, 0.15f);
    // Front side
    glBegin(GL_QUADS);
    glVertex3f(-groundScale, -0.5f, groundScale);
    glVertex3f(-groundScale, 0.5f, groundScale);
    glVertex3f(groundScale, 0.5f, groundScale);
    glVertex3f(groundScale, -0.5f, groundScale);
    glEnd();
    // Back side
    glBegin(GL_QUADS);
    glVertex3f(-groundScale, -0.5f, -groundScale);
    glVertex3f(groundScale, -0.5f, -groundScale);
    glVertex3f(groundScale, 0.5f, -groundScale);
    glVertex3f(-groundScale, 0.5f, -groundScale);
    glEnd();
    // Left side
    glBegin(GL_QUADS);
    glVertex3f(-groundScale, -0.5f, -groundScale);
    glVertex3f(-groundScale, -0.5f, groundScale);
    glVertex3f(-groundScale, 0.5f, groundScale);
    glVertex3f(-groundScale, 0.5f, -groundScale);
    glEnd();
    // Right side
    glBegin(GL_QUADS);
    glVertex3f(groundScale, -0.5f, -groundScale);
    glVertex3f(groundScale, 0.5f, -groundScale);
    glVertex3f(groundScale, 0.5f, groundScale);
    glVertex3f(groundScale, -0.5f, groundScale);
    glEnd();
    // Bottom side
    glBegin(GL_QUADS);
    glVertex3f(-groundScale, -0.5f, -groundScale);
    glVertex3f(-groundScale, -0.5f, groundScale);
    glVertex3f(groundScale, -0.5f, groundScale);
    glVertex3f(groundScale, -0.5f, -groundScale);
    glEnd();

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// Draw animated windows on building faces
void drawWindows(float x, float y, float z, float length, float height, float width, int rows, int cols) {
    glPushMatrix();
    glTranslatef(x, y + height / 2, z);
    glDisable(GL_LIGHTING);

    float windowSize = 0.6f;
    float edgeOffset = 0.6f;
    float embedRatio = 0.8f;
    float embedOffset = windowSize * (embedRatio - 0.5f);

    // Lambda function to draw windows on a specific face
    auto drawFace = [&](float spX, float spY, bool isZ, float sign) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                float wx = -width / 2 + (isZ ? edgeOffset + j * spX : (sign > 0 ? width - embedOffset * 2 : embedOffset));
                float wy = -height / 2 + edgeOffset + i * spY;
                float wz = -length / 2 + (isZ ? (sign > 0 ? length - embedOffset : embedOffset) : edgeOffset + j * spX);

                if (!isZ) { wx = (sign > 0) ? width / 2 - embedOffset : -width / 2 + embedOffset; }

                // Pulsing effect with time-based animation
                float pulse = 0.95f + 0.25f * sin(animationTime * 2.5f + i * cols + j);
                float colorVar = 0.08f * sin(animationTime * 0.8f + i * 0.4f + j * 0.6f);
                glColor3f(0.25f * pulse + colorVar * 0.5f, 0.45f * pulse + colorVar * 0.5f, 0.8f * pulse + colorVar);

                glPushMatrix();
                glTranslatef(wx, wy, wz);
                glScalef(windowSize, windowSize, windowSize);
                glutSolidCube(1.0);
                glPopMatrix();
            }
        }
        };

    // Calculate window spacing
    float spX = (width - 2 * edgeOffset) / (cols - 1);
    float spY = (height - 2 * edgeOffset) / (rows - 1);
    float spZ = (length - 2 * edgeOffset) / (cols - 1);

    // Draw windows on all four faces
    drawFace(spX, spY, true, 1.0f);  // Front face
    drawFace(spX, spY, true, -1.0f); // Back face
    drawFace(spZ, spY, false, -1.0f); // Left face
    drawFace(spZ, spY, false, 1.0f);  // Right face

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// Draw glowing edges around buildings
void drawGlowingEdges(float x, float y, float z, float length, float height, float width, float glowColor[3]) {
    glPushMatrix();
    glTranslatef(x, y + height / 2, z);
    glDisable(GL_LIGHTING);

    // Pulsing glow intensity
    float glowIntensity = 0.8f + 0.2f * sin(animationTime * 2.0f);
    glColor3f(glowColor[0] * glowIntensity, glowColor[1] * glowIntensity, glowColor[2] * glowIntensity);
    glLineWidth(3.0f);

    // Bottom edge loop
    glBegin(GL_LINE_LOOP);
    glVertex3f(-width / 2, -height / 2, -length / 2);
    glVertex3f(width / 2, -height / 2, -length / 2);
    glVertex3f(width / 2, -height / 2, length / 2);
    glVertex3f(-width / 2, -height / 2, length / 2);
    glEnd();

    // Top edge loop
    glBegin(GL_LINE_LOOP);
    glVertex3f(-width / 2, height / 2, -length / 2);
    glVertex3f(width / 2, height / 2, -length / 2);
    glVertex3f(width / 2, height / 2, length / 2);
    glVertex3f(-width / 2, height / 2, length / 2);
    glEnd();

    // Vertical edges
    glBegin(GL_LINES);
    glVertex3f(-width / 2, -height / 2, -length / 2); glVertex3f(-width / 2, height / 2, -length / 2);
    glVertex3f(width / 2, -height / 2, -length / 2); glVertex3f(width / 2, height / 2, -length / 2);
    glVertex3f(width / 2, -height / 2, length / 2); glVertex3f(width / 2, height / 2, length / 2);
    glVertex3f(-width / 2, -height / 2, length / 2); glVertex3f(-width / 2, height / 2, length / 2);
    glEnd();

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// Draw a complete futuristic building
void drawFutureBuilding(float x, float y, float z, float length, float height, float width, float glowColor[3], int windowRows, int windowCols) {
    // Draw building main body
    glPushMatrix();
    glTranslatef(x, y + height / 2, z);
    glColor3f(0.15f, 0.25f, 0.35f); // Dark blue-gray color
    glScalef(width, height, length);
    glutSolidCube(1.0);
    glPopMatrix();

    // Add glowing edges and windows
    drawGlowingEdges(x, y, z, length, height, width, glowColor);
    drawWindows(x, y, z, length, height, width, windowRows, windowCols);
}

// Draw textured roof on specific buildings
void DrawBuildingRoof(int buildingIndex) {
    Building& building = buildings[buildingIndex];
    glPushMatrix();
    glTranslatef(building.x, building.y + building.height + 0.01f, building.z);
    glBindTexture(GL_TEXTURE_2D, textures[0]); // Roof texture
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    // Draw roof as textured quad
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-building.width / 2, 0.0f, -building.length / 2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-building.width / 2, 0.0f, building.length / 2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(building.width / 2, 0.0f, building.length / 2);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(building.width / 2, 0.0f, -building.length / 2);
    glEnd();

    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// Draw all buildings in the scene
void drawBuildings() {
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        Building b = buildings[i];
        drawFutureBuilding(b.x, b.y, b.z, b.length, b.height, b.width, b.glowColor, b.windowRows, b.windowCols);

        // Add textured roof to corner buildings (1st in each quadrant)
        if (i == 0 || i == 10 || i == 20 || i == 30) {
            DrawBuildingRoof(i);
        }

        // Place EVA robot on building 15
        if (i == 15) {
            glPushMatrix();
            glTranslatef(b.x, b.y + b.height + 3.0f, b.z);
            glScalef(0.2f, 0.2f, 0.2f); // Scale down robot
            draw_eva_robot();
            glPopMatrix();
        }
    }
}

// Draw individual vehicle as wireframe capsule
void drawVehicle(const Vehicle& vehicle) {
    glPushMatrix();
    glTranslatef(vehicle.xPos, 1.0f, vehicle.zPos);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    wireCapsule(0.8f);
    glPopMatrix();
}

// Update vehicle positions with wrap-around
void updateVehicles() {
    for (auto& vehicle : vehicles) {
        vehicle.zPos += vehicleMoveSpeed * vehicle.moveDirection;
        // Wrap around when vehicles reach boundary
        if (vehicle.moveDirection == -1 && vehicle.zPos < -250.0f) vehicle.zPos = 250.0f;
        if (vehicle.moveDirection == 1 && vehicle.zPos > 250.0f) vehicle.zPos = -250.0f;
    }
}

// Generic Bezier billboard drawing for buildings
void drawBezierBillboardForBuilding(int buildingIndex, float& rotationAngle) {
    glPushMatrix();
    Building& targetBuilding = buildings[buildingIndex];
    // Position billboard above building
    glTranslatef(targetBuilding.x, targetBuilding.y + targetBuilding.height + 2.0f, targetBuilding.z);

    // Update rotation
    rotationAngle += 0.2f;
    if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);

    int segments = 20;
    GLfloat p[3], pNext[3];

    // Draw Bezier surface as quad strips
    for (int u = 0; u < segments; u++) {
        glBegin(GL_QUAD_STRIP);
        for (int v = 0; v <= segments; v++) {
            float uVal = (float)u / segments, uNextVal = (float)(u + 1) / segments;
            float vVal = (float)v / segments;
            calculateBezierSurfacePoint(uVal, vVal, p);
            calculateBezierSurfacePoint(uNextVal, vVal, pNext);

            // Color animation with time
            float colorIntensity = 0.7f + 0.3f * sin(animationTime * 1.5f + vVal * 4.0f);
            glColor4f(0.2f, 0.4f * colorIntensity, 0.9f, 0.9f);
            glVertex3fv(p);
            glColor4f(0.2f, 0.4f * colorIntensity, 0.9f, 0.9f);
            glVertex3fv(pNext);
        }
        glEnd();
    }

    // Draw wireframe border
    glColor4f(1.0f, 1.0f, 0.0f, 0.8f);
    glLineWidth(2.0f);

    // U-direction border lines
    for (int u = 0; u <= segments; u += segments) {
        glBegin(GL_LINE_STRIP);
        for (int v = 0; v <= segments; v++) {
            calculateBezierSurfacePoint((float)u / segments, (float)v / segments, p);
            glVertex3fv(p);
        }
        glEnd();
    }

    // V-direction border lines
    for (int v = 0; v <= segments; v += segments) {
        glBegin(GL_LINE_STRIP);
        for (int u = 0; u <= segments; u++) {
            calculateBezierSurfacePoint((float)u / segments, (float)v / segments, p);
            glVertex3fv(p);
        }
        glEnd();
    }

    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// Wrapper functions for specific billboards
void drawBezierBillboard_Quad1_Building8() { drawBezierBillboardForBuilding(7, billboardRotation1); }
void drawBezierBillboard_Quad2_Building8() { drawBezierBillboardForBuilding(17, billboardRotation2); }

// Draw suspended torus tunnel using stencil buffer for overlap effect
void drawSuspendedTorus() {
    glPushMatrix();
    glTranslatef(0.0f, TUNNEL_HEIGHT, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Orient torus horizontally

    // Stencil buffer operation for tunnel mode 1
    if (tunnelDisplayMode == 1 && !showOverlap) {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);
        // Draw torus to stencil buffer only
        glutSolidTorus((TORUS_OUTER_RADIUS - TORUS_INNER_RADIUS) / 2,
            (TORUS_OUTER_RADIUS + TORUS_INNER_RADIUS) / 2, 30, TUNNEL_SEGMENTS);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glDisable(GL_STENCIL_TEST);
    }

    // Draw torus wireframe
    glDisable(GL_LIGHTING);
    float pulse = sin(animationTime * 2.0f) * 0.5f + 0.5f;
    glColor3f(1.0f - pulse * 0.5f, 1.0f - pulse * 0.25f, 1.0f); // Pulsing cyan color
    glLineWidth(2.0f);
    glutWireTorus((TORUS_OUTER_RADIUS - TORUS_INNER_RADIUS) / 2,
        (TORUS_OUTER_RADIUS + TORUS_INNER_RADIUS) / 2, 30, TUNNEL_SEGMENTS);
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// Draw X-axis aligned cylinder tunnel
void drawXAxisCylinderTunnel() {
    if (tunnelDisplayMode == 2) return; // Skip in mode 2

    glPushMatrix();
    glTranslatef(-50.0f, TUNNEL_HEIGHT, 0.0f);

    // Stencil test for overlap effect
    if (tunnelDisplayMode == 1 && !showOverlap) {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF); // Draw only where stencil != 1
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }

    // Transparent cylinder
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Disable depth writing for transparency

    glDisable(GL_LIGHTING);
    glColor4f(0.6f, 0.8f, 1.0f, 0.6f); // Semi-transparent blue
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f); // Rotate to align with X-axis
    glutSolidCylinder(TUNNEL_RADIUS, TUNNEL_LENGTH, TUNNEL_SEGMENTS, 16);
    glEnable(GL_LIGHTING);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glPopMatrix();
}

// Draw Z-axis aligned cylinder tunnel
void drawZAxisCylinderTunnel() {
    if (tunnelDisplayMode == 2) return; // Skip in mode 2

    glPushMatrix();
    glTranslatef(0.0f, TUNNEL_HEIGHT, -50.0f);

    // Stencil test for overlap effect
    if (tunnelDisplayMode == 1 && !showOverlap) {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }

    // Transparent cylinder
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glDisable(GL_LIGHTING);
    glColor4f(0.6f, 0.8f, 1.0f, 0.6f);
    glutSolidCylinder(TUNNEL_RADIUS, TUNNEL_LENGTH, TUNNEL_SEGMENTS, 16);
    glEnable(GL_LIGHTING);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glPopMatrix();
}

// Draw orbiting white sphere with metallic material
void drawWhiteSolidSphere() {
    glPushMatrix();
    const float rotateY = 30.0f;
    const float orbitRadius = 6.5f;
    const float sphereRadius = 1.0f;

    // Update orbital position
    whiteSphereAngle += 0.5f;
    if (whiteSphereAngle >= 360.0f) whiteSphereAngle -= 360.0f;
    glTranslatef(orbitRadius * cos(whiteSphereAngle * M_PI / 180.0f),
        rotateY,
        orbitRadius * sin(whiteSphereAngle * M_PI / 180.0f));

    // Set material properties for metallic appearance
    glMaterialfv(GL_FRONT, GL_DIFFUSE, whiteSphere_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, whiteSphere_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, whiteSphere_shininess);
    glEnable(GL_COLOR_MATERIAL);
    glColor4fv(whiteSphere_diffuse);

    glutSolidSphere(sphereRadius, 32, 32);
    glPopMatrix();
}

// Draw rotating diamond decoration inside torus
void drawDiamondDecoration() {
    if (!showDiamond) return;

    glPushMatrix();
    glTranslatef(0.0f, TUNNEL_HEIGHT, 0.0f);

    // Update rotation
    diamondRotation += 0.3f;
    if (diamondRotation >= 360.0f) diamondRotation -= 360.0f;
    glRotatef(diamondRotation, 1.0f, 1.0f, 0.0f);

    const float maxDiamondSize = TORUS_INNER_RADIUS * 0.95f;

    // Transparent rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);

    // Color animation
    float transition = sin(animationTime * 0.8f) * 0.5f + 0.5f;
    float alpha = 0.7f + 0.2f * sin(animationTime * 2.0f);
    glColor4f(0.4f + 0.4f * transition, 0.1f, 0.9f, alpha);

    // Draw solid octahedron (diamond shape)
    glScalef(maxDiamondSize, maxDiamondSize, maxDiamondSize);
    glutSolidOctahedron();

    // Draw wireframe overlay
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    glLineWidth(2.0f);
    glutWireOctahedron();
    glLineWidth(1.0f);

    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glPopMatrix();
}

// Draw solid capsule on building 6 in quadrant 1
void drawSolidCapsuleOnQuad1Building6() {
    glPushMatrix();
    Building& targetBuilding = buildings[5];
    glTranslatef(targetBuilding.x, targetBuilding.y + targetBuilding.height + 3.5f, targetBuilding.z);

    // Rotating capsule
    static float capsuleRot = 0.0f;
    capsuleRot += 0.5f;
    if (capsuleRot >= 360.0f) capsuleRot -= 360.0f;
    glRotatef(capsuleRot, 0.0f, 1.0f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);

    solidCapsule(1.5f);
    glPopMatrix();
}

// Draw spike (cone) for EVA robot details
void draw_spike(float radius, float height, int segments = 8)
{
    // Draw cone sides
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, height / 2, 0.0f);  // Top vertex

    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        glVertex3f(x, -height / 2, z);
    }
    glEnd();

    // Draw base circle
    glBegin(GL_POLYGON);
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        glVertex3f(x, -height / 2, z);
    }
    glEnd();
}

// Draw EVA-style head with armor plates and visor
void draw_eva_head()
{
    glPushMatrix();
    // Scale down head proportions
    glScalef(0.6f, 0.8f, 0.5f);

    // Main helmet (purple)
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_purple);
    glutSolidSphere(2.2f, 20, 20);

    // Face armor (white)
    glPushMatrix();
    glTranslatef(0.0f, -0.4f, 1.6f);
    glScalef(1.2f, 1.0f, 0.1f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_white);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Visor (green glowing)
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 1.7f);
    glScalef(0.8f, 0.6f, 0.05f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_green);
    glMaterialfv(GL_FRONT, GL_EMISSION, eva01_green); // Emissive for glow
    glutSolidCube(1.0f);
    glMaterialfv(GL_FRONT, GL_EMISSION, eva01_black); // Reset emission
    glPopMatrix();

    // Main horn (yellow)
    glPushMatrix();
    glTranslatef(0.0f, 1.8f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glScalef(0.8f, 0.8f, 1.6f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_yellow);
    draw_spike(0.25f, 1.6f);
    glPopMatrix();

    // Side horns (symmetrical)
    for (int i = -1; i <= 1; i += 2)
    {
        glPushMatrix();
        glTranslatef(i * 1.2f, 1.4f, 0.0f);
        glRotatef(-70.0f, 1.0f, 0.0f, i * 0.5f);
        glScalef(0.6f, 0.6f, 1.2f);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_yellow);
        draw_spike(0.25f, 1.2f);
        glPopMatrix();
    }

    // Ear vents/air intakes
    for (int i = -1; i <= 1; i += 2)
    {
        glPushMatrix();
        glTranslatef(i * 1.5f, 0.3f, 0.0f);
        glScalef(0.4f, 0.6f, 0.3f);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_black);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    glPopMatrix();
}

// Draw EVA-style neck with armor and connectors
void draw_eva_neck()
{
    glPushMatrix();
    glRotatef(neck_angle, 0.0f, 1.0f, 0.0f);

    // Neck armor
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_black);
    glPushMatrix();
    glScalef(1.0f, 1.2f, 1.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Neck connector (green glowing)
    glPushMatrix();
    glScalef(0.7f, 0.7f, 0.7f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_green);
    glMaterialfv(GL_FRONT, GL_EMISSION, eva01_green);
    glutSolidSphere(0.6f, 10, 10);
    glMaterialfv(GL_FRONT, GL_EMISSION, eva01_black);
    glPopMatrix();

    glPopMatrix();
}

// Draw EVA-style torso with chest armor and core
void draw_eva_torso()
{
    glPushMatrix();

    // Chest armor (purple)
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_purple);
    glPushMatrix();
    glScalef(4.0f, 5.0f, 2.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Core (red glowing)
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 1.5f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_red);
    glMaterialfv(GL_FRONT, GL_EMISSION, eva01_red);
    glutSolidSphere(0.8f, 15, 15);
    glMaterialfv(GL_FRONT, GL_EMISSION, eva01_black);
    glPopMatrix();

    // Shoulder connectors (green)
    for (int i = -1; i <= 1; i += 2)
    {
        glPushMatrix();
        glTranslatef(i * 3.0f, 1.5f, 0.0f);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_green);
        glutSolidSphere(0.8f, 10, 10);
        glPopMatrix();
    }

    // Waist armor plate
    glPushMatrix();
    glTranslatef(0.0f, -3.0f, 0.0f);
    glScalef(4.5f, 1.0f, 3.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_black);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

// Draw EVA-style arm with shoulder, elbow, and hand
void draw_eva_arm(int is_left)
{
    glPushMatrix();

    float sign = is_left ? -1.0f : 1.0f;
    float shoulder_angle = is_left ? left_shoulder_angle : right_shoulder_angle;
    float elbow_angle = is_left ? left_elbow_angle : right_elbow_angle;

    // Shoulder joint
    glTranslatef(sign * 3.0f, 1.5f, 0.0f);
    glRotatef(sign * shoulder_angle, 0.0f, 1.0f, 0.0f);

    // Upper arm armor
    glPushMatrix();
    glScalef(1.5f, 1.5f, 1.5f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_purple);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Upper arm connector (green)
    glPushMatrix();
    glTranslatef(0.0f, -1.5f, 0.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_green);
    glutSolidSphere(0.6f, 8, 8);
    glPopMatrix();

    // Forearm
    glTranslatef(0.0f, -3.0f, 0.0f);
    glRotatef(elbow_angle, 1.0f, 0.0f, 0.0f);

    // Forearm armor
    glPushMatrix();
    glScalef(1.2f, 2.0f, 1.2f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_purple);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Elbow spike
    glPushMatrix();
    glTranslatef(0.0f, -1.5f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_yellow);
    draw_spike(0.2f, 1.0f);
    glPopMatrix();

    // Hand
    glPushMatrix();
    glTranslatef(0.0f, -3.0f, 0.0f);

    // Palm
    glPushMatrix();
    glScalef(1.0f, 0.8f, 0.5f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_black);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Four fingers
    for (int i = -1; i <= 2; i++)
    {
        glPushMatrix();
        glTranslatef(i * 0.25f, -0.7f, 0.0f);
        glScalef(0.18f, 0.5f, 0.18f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Thumb
    glPushMatrix();
    glTranslatef(-0.5f, -0.3f, 0.2f);
    glRotatef(45.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.18f, 0.35f, 0.18f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix(); // End hand

    glPopMatrix(); // End arm
}

// Draw EVA-style leg with hip, knee, and foot
void draw_eva_leg(int is_left)
{
    glPushMatrix();

    float sign = is_left ? -1.0f : 1.0f;
    float hip_angle = is_left ? left_hip_angle : right_hip_angle;
    float knee_angle = is_left ? left_knee_angle : right_knee_angle;

    // Hip joint
    glTranslatef(sign * 1.5f, -3.5f, 0.0f);
    glRotatef(hip_angle, 1.0f, 0.0f, 0.0f);

    // Thigh armor
    glPushMatrix();
    glScalef(1.8f, 3.0f, 2.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_purple);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Knee connector
    glPushMatrix();
    glTranslatef(0.0f, -3.0f, 0.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_green);
    glutSolidSphere(0.8f, 10, 10);
    glPopMatrix();

    // Shin
    glTranslatef(0.0f, -5.0f, 0.0f);
    glRotatef(knee_angle, 1.0f, 0.0f, 0.0f);

    // Shin armor
    glPushMatrix();
    glScalef(1.5f, 3.0f, 1.8f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_purple);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Knee spike
    glPushMatrix();
    glTranslatef(0.0f, -0.5f, 0.8f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_yellow);
    draw_spike(0.3f, 1.5f);
    glPopMatrix();

    // Ankle connector
    glPushMatrix();
    glTranslatef(0.0f, -3.0f, 0.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_green);
    glutSolidSphere(0.6f, 8, 8);
    glPopMatrix();

    // Foot
    glPushMatrix();
    glTranslatef(0.0f, -4.0f, 0.5f);

    // Foot plate
    glPushMatrix();
    glScalef(1.5f, 0.8f, 3.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_black);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Heel spike
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -1.8f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_yellow);
    draw_spike(0.2f, 1.0f);
    glPopMatrix();

    glPopMatrix(); // End foot

    glPopMatrix(); // End leg
}

// Draw backpack with entry plug connector and thrusters
void draw_eva_backpack()
{
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -2.0f);

    // Main backpack structure
    glPushMatrix();
    glScalef(4.0f, 5.0f, 1.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_black);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Entry plug connector (red)
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -1.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_red);
    glMaterialfv(GL_FRONT, GL_EMISSION, eva01_red);
    glutSolidSphere(0.8f, 12, 12);
    glMaterialfv(GL_FRONT, GL_EMISSION, eva01_black);
    glPopMatrix();

    // Weapon racks (yellow)
    for (int i = -1; i <= 1; i += 2)
    {
        glPushMatrix();
        glTranslatef(i * 1.5f, 1.0f, -0.5f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(0.5f, 3.0f, 0.5f);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_yellow);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Thrusters (green glowing)
    for (int i = -1; i <= 1; i += 2)
    {
        glPushMatrix();
        glTranslatef(i * 1.0f, -1.5f, -1.0f);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, eva01_green);
        glMaterialfv(GL_FRONT, GL_EMISSION, eva01_green);
        glutSolidCone(0.5f, 1.0f, 10, 5);
        glMaterialfv(GL_FRONT, GL_EMISSION, eva01_black);
        glPopMatrix();
    }

    glPopMatrix();
}

// Draw complete EVA robot by assembling all components
void draw_eva_robot()
{
    glPushMatrix();

    // Backpack first (drawn behind torso)
    draw_eva_backpack();

    // Torso (main body)
    draw_eva_torso();

    // Neck
    glPushMatrix();
    glTranslatef(0.0f, 3.8f, 0.0f);
    draw_eva_neck();
    glPopMatrix();

    // Head
    glPushMatrix();
    glTranslatef(0.0f, 5.8f, 0.0f);
    draw_eva_head();
    glPopMatrix();

    // Arms
    draw_eva_arm(1);  // Left arm
    draw_eva_arm(0);  // Right arm

    // Legs
    draw_eva_leg(1);  // Left leg
    draw_eva_leg(0);  // Right leg

    glPopMatrix();
}

// Draw help text in screen corner
void drawTextAtCorner() {
    if (!showHelp) return; // Skip if help is hidden

    // Save current projection and modelview matrices
    glPushAttrib(GL_TRANSFORM_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Set orthogonal projection matching window pixels
    gluOrtho2D(0, mainWindowWidth, 0, mainWindowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Disable depth test for 2D text rendering
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f); // White text

    // Text layout parameters
    int margin = 20;
    int lineHeight = 22;
    int startX = margin;
    int startY = mainWindowHeight - margin;
    void* font = GLUT_BITMAP_9_BY_15; // Compact font

    // Draw each line of help text
    for (int i = 0; helpText[i] != NULL; i++) {
        const char* line = helpText[i];
        glRasterPos2i(startX, startY - i * lineHeight);
        // Draw characters individually
        for (int j = 0; line[j] != '\0'; j++) {
            glutBitmapCharacter(font, line[j]);
        }
    }

    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

// ==============================================================================
// 7. Window and Input Callbacks
// ==============================================================================

// Handle window resize
void reShape(int w, int h) {
    if (h == 0) h = 1;
    float ratio = w * 1.0 / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 0.1f, 200.0f);
    glMatrixMode(GL_MODELVIEW);
}

// Keyboard input handling
void processNormalKeys(unsigned char key, int x, int y) {
    if (key == 27) exit(0); // ESC to exit
    float fraction = 0.5f;

    switch (key) {
    case 'w': if (viewMode == 0) camZ -= fraction; break; // Move forward
    case 's': if (viewMode == 0) camZ += fraction; break; // Move backward
    case 'a': if (viewMode == 0) camX -= fraction; break; // Move left
    case 'd': if (viewMode == 0) camX += fraction; break; // Move right
    case 'r': case 'R': // Cycle red component of light
        light_diffuse_color[0] = (light_diffuse_color[0] > 1.0f) ? 0.0f : light_diffuse_color[0] + 0.1f;
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse_color); break;
    case 'g': case 'G': // Cycle green component
        light_diffuse_color[1] = (light_diffuse_color[1] > 1.0f) ? 0.0f : light_diffuse_color[1] + 0.1f;
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse_color); break;
    case 'b': case 'B': // Cycle blue component
        light_diffuse_color[2] = (light_diffuse_color[2] > 1.0f) ? 0.0f : light_diffuse_color[2] + 0.1f;
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse_color); break;
    case 'c': case 'C': // Change light position
        light_x = (light_x >= 2.0f) ? 0.0f : light_x + 0.2f;
        { GLfloat lpos[] = { light_x, light_y, light_z, 0.0f }; glLightfv(GL_LIGHT0, GL_POSITION, lpos); }
        break;
    case '1': tunnelDisplayMode = 1; break; // Tunnel mode 1: intersection
    case '2': tunnelDisplayMode = 2; break; // Tunnel mode 2: torus only
    case '3': tunnelDisplayMode = 3; break; // Tunnel mode 3: cylinders only
    case '0': // Reset view
        viewMode = 0;
        verticalAngle = 0.0f; angle = 0.0f;
        camX = 0.0f; camY = 40.0f; camZ = 60.0f;
        break;
    case 'v': case 'V': // Toggle view mode
        viewMode = 1 - viewMode;
        break;
    case 'h': case 'H': // Toggle help display
        showHelp = !showHelp;
        break;
    }
    glutPostRedisplay();
}

// Mouse button press/release
void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseLeftPressed = true;
            mouseX = x; mouseY = y;
        }
        else {
            mouseLeftPressed = false;
        }
    }
}

// Mouse drag for camera rotation
void mouseMove(int x, int y) {
    if (mouseLeftPressed) {
        int deltaX = x - mouseX;
        int deltaY = y - mouseY;
        float sensitivity = 0.005f;

        angle += deltaX * sensitivity;
        verticalAngle -= deltaY * sensitivity;

        // Clamp vertical angle
        if (verticalAngle > 1.5f) verticalAngle = 1.5f;
        if (verticalAngle < -1.5f) verticalAngle = -1.5f;

        mouseX = x; mouseY = y;
        glutPostRedisplay();
    }
}

// Mouse wheel for zoom
void mouseWheel(int wheel, int direction, int x, int y) {
    camY += (direction > 0) ? 2.0f : -2.0f;
    glutPostRedisplay();
}

// Idle function for continuous animations
void when_in_mainloop() {
    if (viewMode == 1) {
        // Move scene in fixed view mode
        sceneZOffset -= sceneMoveSpeed;
        // Wrap-around for continuous motion
        if (sceneZOffset <= -300.0f) {
            sceneZOffset = 300.0f;
        }
        else if (sceneZOffset >= 300.0f) {
            sceneZOffset = -300.0f;
        }
    }
    glutPostRedisplay();
}

// Main rendering function
void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();

    // Update animation time and vehicle positions
    animationTime += 0.01f;
    updateVehicles();

    // Calculate camera based on view mode
    float currCamX, currCamY, currCamZ;
    float currAngle, currVerticalAngle;
    if (viewMode == 1) {
        // Fixed camera view
        currCamX = fixedCamX;
        currCamY = fixedCamY;
        currCamZ = fixedCamZ;
        currAngle = 0.0f;
        currVerticalAngle = 0.0f;
    }
    else {
        // Default interactive camera
        currCamX = camX;
        currCamY = camY;
        currCamZ = camZ;
        currAngle = angle;
        currVerticalAngle = verticalAngle;
    }

    // Calculate look-at point
    float lookAtX = currCamX + sin(currAngle) * cos(currVerticalAngle);
    float lookAtY = currCamY + sin(currVerticalAngle);
    float lookAtZ = currCamZ - cos(currAngle) * cos(currVerticalAngle);
    gluLookAt(currCamX, currCamY, currCamZ, lookAtX, lookAtY, lookAtZ, 0.0f, 1.0f, 0.0f);

    // In fixed view mode, offset entire scene
    if (viewMode == 1) {
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, sceneZOffset);
    }

    // Draw all scene elements
    drawGrid();
    drawBuildings();
    drawSuspendedTorus();
    drawXAxisCylinderTunnel();
    drawZAxisCylinderTunnel();
    drawBezierBillboard_Quad1_Building8();
    drawBezierBillboard_Quad2_Building8();
    drawSolidCapsuleOnQuad1Building6();
    drawWhiteSolidSphere();
    drawDiamondDecoration();
    for (const auto& vehicle : vehicles) {
        drawVehicle(vehicle);
    }

    // Pop scene offset matrix
    if (viewMode == 1) {
        glPopMatrix();
    }

    // Draw help text (2D overlay)
    drawTextAtCorner();

    glutSwapBuffers();
}

// ==============================================================================
// 8. Main Function
// ==============================================================================

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(mainWindowWidth, mainWindowHeight);
    glutCreateWindow("Future City(Press H for help)");

    // Initialize scene objects
    initBuildings();
    initVehicles();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background

    // Initialize OpenGL settings
    initLightingAndMaterial();
    glGenTextures(3, textures); // Generate texture IDs
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    // Load textures
    readMap("texture/1.bmp", 0); // Roof texture
    readMap("texture/2.bmp", 1); // Road texture
    makeChessboardTexture();     // Procedural chessboard texture

    // Register callback functions
    glutDisplayFunc(renderScene);
    glutReshapeFunc(reShape);
    glutIdleFunc(when_in_mainloop);
    glutKeyboardFunc(processNormalKeys);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMove);
    glutMouseWheelFunc(mouseWheel);

    // Hide cursor and enable depth testing
    glutSetCursor(GLUT_CURSOR_NONE);
    glEnable(GL_DEPTH_TEST);

    // Enter main loop
    glutMainLoop();

    return 1;
}