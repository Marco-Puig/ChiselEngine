// Tell OpenXR what platform code we'll be using
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#define OPENGL_SWAPCHAIN_FORMAT 0x8C43

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // Windows functions such as Window creation

// OpenXR libs and includes
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// OpenGL libs and includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// For texture loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> 

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/quaternion.hpp>

// Model loading
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <thread> // sleep_for
#include <vector> // dynamic array management for cube positions
#include <algorithm> // any_of

#include <tchar.h> // TCHAR type for parsing vertex and frag shader code

#include <cstdio> // parse - debug
#include <cmath> // sin, cos

#include <iostream> // std namespace
#include <sstream> // string conversions
#include <map> // key-value pairs
#include <string> // string manipulation

///////////////////////////////////////////

struct swapchain_surfdata_t {
	GLuint fbo;
	GLuint depthbuffer;
};

struct swapchain_t {
	XrSwapchain handle;
	int32_t width;
	int32_t height;
	std::vector<XrSwapchainImageOpenGLKHR> surface_images;
	std::vector<swapchain_surfdata_t>      surface_data;
};

struct input_state_t {
	XrActionSet actionSet;
	XrAction    poseAction;
	XrAction    selectAction;
	XrPath   handSubactionPath[2];
	XrSpace  handSpace[2];
	XrPosef  handPose[2];
	XrBool32 renderHand[2];
	XrBool32 handSelect[2];
};

///////////////////////////////////////////

// Function pointers for some OpenXR extension methods we'll use.
PFN_xrGetOpenGLGraphicsRequirementsKHR ext_xrGetOpenGLGraphicsRequirementsKHR = nullptr;
PFN_xrCreateDebugUtilsMessengerEXT    ext_xrCreateDebugUtilsMessengerEXT = nullptr;
PFN_xrDestroyDebugUtilsMessengerEXT   ext_xrDestroyDebugUtilsMessengerEXT = nullptr;

///////////////////////////////////////////

struct app_transform_buffer_t {
	glm::mat4 world;
	glm::mat4 viewproj;
};

app_transform_buffer_t transform_buffer;

XrFormFactor            app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

///////////////////////////////////////////

GLuint app_shader_program = 0;
GLuint app_uniform_buffer = 0;

GLuint app_vao; // VAO (Vertex Array Object) for input layout

GLuint app_ubo; // Uniform Buffer Object for constant data
GLuint app_vbo; // Vertex Buffer Object
GLuint app_ebo; // Element Buffer Object (for indices) - NEED FOR OPENGL VERTICES OPTIMIZATION

std::vector<XrPosef> app_controllers; // Controller positions

///////////////////////////////////////////

GLuint skyboxShaderProgram = 0;
GLuint skyboxVAO;
GLuint skyboxVBO;
GLuint skyboxEBO;
GLuint cubemapTexture = 0;
GLuint loadCubemap(std::vector<std::string> faces);

HWND g_hWnd = nullptr; // Window handle

GLFWwindow* window; // Assume we have a valid GLFWwindow*
int desktopWidth = 1160;
int desktopHeight = 1100;

///////////////////////////////////////////

uint32_t leftEyeImageIndex; // Set during xrAcquireSwapchainImage calls for left eye
int left_eye_index = 0; // left eye at index 0
extern std::vector<swapchain_t> xr_swapchains;
extern GLFWwindow* window;
extern int desktopWidth, desktopHeight;

///////////////////////////////////////////

void app_init();
void app_draw(XrCompositionLayerProjectionView& layerView);
void app_update();
void app_update_predicted();
void opengl_shutdown();

///////////////////////////////////////////

const XrPosef  xr_pose_identity = { {0,0,0,1}, {0,0,0} };
XrInstance     xr_instance = {};
XrSession      xr_session = {};
XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
bool           xr_running = false;
bool		   quit = false;
XrSpace        xr_app_space = {};
XrSystemId     xr_system_id = XR_NULL_SYSTEM_ID;
input_state_t  xr_input = { };
XrEnvironmentBlendMode   xr_blend = {};
XrDebugUtilsMessengerEXT xr_debug = {};

std::vector<XrView>                  xr_views;
std::vector<XrViewConfigurationView> xr_config_views;
std::vector<swapchain_t>             xr_swapchains;

bool openxr_init(const char* app_name, int64_t swapchain_format);
void openxr_make_actions();
void openxr_shutdown();
void openxr_poll_events(bool& exit);
void openxr_poll_actions();
void openxr_poll_predicted(XrTime predicted_time);
void openxr_render_frame();
bool openxr_render_layer(XrTime predictedTime, std::vector<XrCompositionLayerProjectionView>& projectionViews, XrCompositionLayerProjection& layer);
void gl_swapchain_destroy(swapchain_t& swapchain);
void gl_render_layer(XrCompositionLayerProjectionView& view, swapchain_surfdata_t& surface, GLFWwindow* window);
swapchain_surfdata_t gl_make_surface_data(XrBaseInStructure& swapchain_img, int32_t width, int32_t height);

///////////////////////////////////////////

double priorTime = 0.0;
double currentTime = 0.0;
double timeDifference;
unsigned int counter = 0;
void calculate_framerate();

///////////////////////////////////////////

class Game {
public:
	void start();
	void update();
	void render();
};

Game game;

///////////////////////////////////////////

// SkyBox - Cubemap  

float skyboxVertices[] =
{
	//   Coordinates
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f
};

unsigned int skyboxIndices[] =
{
	// Right
	1, 2, 6,
	6, 5, 1,
	// Left
	0, 4, 7,
	7, 3, 0,
	// Top
	4, 5, 6,
	6, 7, 4,
	// Bottom
	0, 3, 2,
	2, 1, 0,
	// Back
	0, 1, 5,
	5, 4, 0,
	// Front
	3, 7, 6,
	6, 2, 3
};

///////////////////////////////////////////
