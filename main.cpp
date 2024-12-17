// Tell OpenXR what platform code we'll be using
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#define OPENGL_SWAPCHAIN_FORMAT 0x8C43 // GL_SRGB8_ALPHA8 for OpenXR/DX11 

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

using namespace std; // standard - std lib

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

XrFormFactor            app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

///////////////////////////////////////////

GLuint app_shader_program = 0;
GLuint cube_shader_program = 0;
GLuint app_uniform_buffer = 0;

GLuint app_vao; // VAO (Vertex Array Object) for input layout

GLuint app_ubo; // Uniform Buffer Object for constant data
GLuint app_vbo; // Vertex Buffer Object
GLuint app_ebo; // Element Buffer Object (for indices) - NEED FOR OPENGL VERTICES OPTIMIZATION

vector<XrPosef> app_cubes; // Cube positions

///////////////////////////////////////////

GLuint skyboxShaderProgram = 0; 
GLuint skyboxVAO;
GLuint skyboxVBO;
GLuint skyboxEBO;
GLuint cubemapTexture = 0; 
GLuint loadCubemap(vector<string> faces);

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

vector<XrView>                  xr_views;
vector<XrViewConfigurationView> xr_config_views;
vector<swapchain_t>             xr_swapchains;

bool openxr_init(const char* app_name, int64_t swapchain_format);
void openxr_make_actions();
void openxr_shutdown();
void openxr_poll_events(bool& exit);
void openxr_poll_actions();
void openxr_poll_predicted(XrTime predicted_time);
void openxr_render_frame();
bool openxr_render_layer(XrTime predictedTime, vector<XrCompositionLayerProjectionView>& projectionViews, XrCompositionLayerProjection& layer);
void gl_swapchain_destroy(swapchain_t& swapchain);
void gl_render_layer(XrCompositionLayerProjectionView& view, swapchain_surfdata_t& surface, GLFWwindow* window);
swapchain_surfdata_t gl_make_surface_data(XrBaseInStructure& swapchain_img, int32_t width, int32_t height);

///////////////////////////////////////////

double prevTime = 0.0;
double crntTime = 0.0;
double timeDiff;
unsigned int counter = 0;
void calculate_framerate();

///////////////////////////////////////////

struct Texture {
	GLuint id;
	string type;
	string path;
};

struct Model {
	GLuint vao;       // Vertex Array Object
	GLuint vbo;       // Vertex Buffer Object
	GLuint ebo;       // Element Buffer Object
	size_t indexCount; // Number of indices
	GLuint textureID; // Add a texture ID
};


GLuint loadTexture(const string& path);
Model loadModelWithTexture(const string& objPath, const string& texturePath); // old model loading function - only for models without textures
void drawModel(const Model& model, const glm::mat4& viewProj, const glm::mat4& modelMatrix);
void cleanupModel(const Model& model);

///////////////////////////////////////////

const char* cube_vertex_glsl = R"(
#version 450 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;

layout(std140) uniform TransformBuffer {
    mat4 world;
    mat4 viewproj;
};

out vec3 vColor;

void main() {
    vec4 worldPos = world * vec4(in_pos,1.0);
    gl_Position = viewproj * worldPos;

    vec3 normal = normalize((world * vec4(in_norm,0.0)).xyz);
    vColor = max(dot(normal, vec3(0,1,0)), 0.0) * vec3(1.0,1.0,1.0);
}
)";

const char* cube_fragment_glsl = R"(
#version 450 core
in vec3 vColor;
out vec4 fragColor;

void main() {
    fragColor = vec4(vColor,1.0);
}
)";

const char* vertex_glsl = R"(
#version 450 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoords; // Input texture coordinates

out vec2 TexCoords; // Pass texture coordinates to fragment shader

layout(std140) uniform TransformBuffer {
    mat4 world;
    mat4 viewproj;
};

void main() {
    TexCoords = in_texCoords;
    gl_Position = viewproj * world * vec4(in_pos, 1.0);
}

)";

const char* fragment_glsl = R"(
#version 450 core
in vec2 TexCoords; 
out vec4 fragColor;

uniform sampler2D texture_diffuse; 

void main() {
	fragColor = texture(texture_diffuse, TexCoords);
}
)";

const char* cubemap_vertex_glsl = R"(
#version 450 core
layout (location = 0) in vec3 aPos;

uniform mat4 uProjection;
uniform mat4 uView;

out vec3 TexCoords;

void main() {
    TexCoords = aPos;
    vec4 pos = uProjection * uView * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // trick to ensure depth = 1.0 and we don't discard skybox
}
)";

const char* cubemap_fragment_glsl = R"(
#version 450 core
in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main() {    
    FragColor = texture(skybox, TexCoords);
}
)";

///////////////////////////////////////////
// Demo Cube Model                       //
///////////////////////////////////////////

float app_verts[] = {
	-1,-1,-1, -1,-1,-1, // Bottom verts
	 1,-1,-1,  1,-1,-1,
	 1, 1,-1,  1, 1,-1,
	-1, 1,-1, -1, 1,-1,
	-1,-1, 1, -1,-1, 1, // Top verts
	 1,-1, 1,  1,-1, 1,
	 1, 1, 1,  1, 1, 1,
	-1, 1, 1, -1, 1, 1,
};

uint16_t app_inds[] = {
	1,2,0, 2,3,0, 4,6,5, 7,6,4,
	6,2,1, 5,6,1, 3,7,4, 0,3,4,
	4,5,1, 0,4,1, 2,7,3, 2,6,7,
};

Model rockModel;
Model sceneModel;
GLuint sandTexture;
GLuint rockTexture;

///////////////////////////////////////////
// SkyBox - Cubemap                      //
///////////////////////////////////////////

float skyboxVertices[] =
{
	//   Coordinates
	-1.0f, -1.0f,  1.0f,//        7--------6
	 1.0f, -1.0f,  1.0f,//       /|       /|
	 1.0f, -1.0f, -1.0f,//      4--------5 |
	-1.0f, -1.0f, -1.0f,//      | |      | |
	-1.0f,  1.0f,  1.0f,//      | 3------|-2
	 1.0f,  1.0f,  1.0f,//      |/       |/
	 1.0f,  1.0f, -1.0f,//      0--------1
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
// Main                                  //
///////////////////////////////////////////

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
	// Initialize GLFW (creates the window and OpenGL context)
	if (!glfwInit()) {
		MessageBox(nullptr, _T("GLFW initialization failed\n"), _T("Error"), MB_OK);
		return 1;
	}

	// Create a GLFW window with an OpenGL context - OpenGL 4.3 Core
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(desktopWidth, desktopHeight, "Chisel Engine - OpenGL 4.3", nullptr, nullptr);

	if (!window) {
		MessageBox(nullptr, _T("Window creation failed\n"), _T("Error"), MB_OK);
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);

	// Now that we have a context, we can load OpenGL functions with glad
	if (!gladLoadGL()) {
		MessageBox(nullptr, _T("Failed to load OpenGL functions\n"), _T("Error"), MB_OK);
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	if (!openxr_init("Single file OpenXR", OPENGL_SWAPCHAIN_FORMAT)) {
		MessageBox(nullptr, _T("OpenXR initialization failed\n"), _T("Error"), MB_OK);
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	openxr_make_actions();
	app_init();

	// Load rock model - replace this with a much more scalable model loading system - Prehaps run Game class (Game.init() or Game.Start()) to load all models
	rockModel = loadModelWithTexture("Resources/rock_for_david19k.obj", "Resources/rock_texture.jpeg");
	sceneModel = loadModelWithTexture("Resources/SANDnSTONE_simplified.obj", "Resources/SANDnSTONE_simplified.jpeg");

	// Enable depth 
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {

		// Calculate framerate and update window title with it
		calculate_framerate();

		// Poll for events (Windows and/or GLFW)
		glfwPollEvents();

		// assign depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Poll input actions at start of frame
		openxr_poll_events(quit);

		if (xr_running) {
			// Poll input actions and update (e.g. hand tracking)
			openxr_poll_actions();
			app_update();

			// Render the VR frame into the XR swapchains
			openxr_render_frame();

			// Swap the GLFW buffers
			glfwSwapBuffers(window);

			// If the XR session is not visible or focused, sleep a bit to reduce CPU usage
			if (xr_session_state != XR_SESSION_STATE_VISIBLE &&
				xr_session_state != XR_SESSION_STATE_FOCUSED) {
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
		}
	}

	cleanupModel(rockModel);
	cleanupModel(sceneModel);
	openxr_shutdown();
	opengl_shutdown();

	return 0;
}

///////////////////////////////////////////
// OpenXR code                           //
///////////////////////////////////////////

bool openxr_init(const char* app_name, int64_t swapchain_format) {
	// Extensions we want to use
	const char* ask_extensions[] = {
		XR_KHR_OPENGL_ENABLE_EXTENSION_NAME, // Use OpenGL for rendering
		XR_EXT_DEBUG_UTILS_EXTENSION_NAME,   // Debug utils for extra info
	};

	// Enumerate and check for available extensions
	uint32_t ext_count = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
	std::vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
	xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());

	std::vector<const char*> use_extensions;
	for (size_t i = 0; i < xr_exts.size(); i++) {
		printf("- %s\n", xr_exts[i].extensionName);

		// Check if we're asking for this extension
		for (int32_t ask = 0; ask < _countof(ask_extensions); ask++) {
			if (strcmp(ask_extensions[ask], xr_exts[i].extensionName) == 0) {
				use_extensions.push_back(ask_extensions[ask]);
				break;
			}
		}
	}

	// Ensure the OpenGL extension is available
	if (!std::any_of(use_extensions.begin(), use_extensions.end(),
		[](const char* ext) {
			return strcmp(ext, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0;
		})) {
		MessageBox(nullptr, _T("XR_KHR_opengl_enable not available"), _T("Error"), MB_OK);
		return false;
	}

	// Create the OpenXR instance
	XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	createInfo.enabledExtensionCount = (uint32_t)use_extensions.size();
	createInfo.enabledExtensionNames = use_extensions.data();
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(createInfo.applicationInfo.applicationName, app_name);

	XrResult res = xrCreateInstance(&createInfo, &xr_instance);
	if (XR_FAILED(res) || xr_instance == nullptr) {
		MessageBox(nullptr, _T("Failed to create XR instance\n"), _T("Error"), MB_OK);
		return false;
	}

	// Load required extension functions
	xrGetInstanceProcAddr(xr_instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrCreateDebugUtilsMessengerEXT));
	xrGetInstanceProcAddr(xr_instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrDestroyDebugUtilsMessengerEXT));
	xrGetInstanceProcAddr(xr_instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&ext_xrGetOpenGLGraphicsRequirementsKHR));

	// Set up verbose debug logging
	XrDebugUtilsMessengerCreateInfoEXT debug_info = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debug_info.messageTypes =
		XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
	debug_info.messageSeverities =
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT* msg, void* user_data) {
		printf("%s: %s\n", msg->functionName, msg->message);
		char text[512];
		sprintf_s(text, "%s: %s", msg->functionName, msg->message);
		OutputDebugStringA(text);
		return (XrBool32)XR_FALSE;
		};

	if (ext_xrCreateDebugUtilsMessengerEXT)
		ext_xrCreateDebugUtilsMessengerEXT(xr_instance, &debug_info, &xr_debug);

	// Get the system (device) for the desired form factor
	XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
	systemInfo.formFactor = app_config_form;
	res = xrGetSystem(xr_instance, &systemInfo, &xr_system_id);
	if (XR_FAILED(res)) {
		MessageBox(nullptr, _T("Failed to get XR system\n"), _T("Error"), MB_OK);
		return false;
	}

	// Get a valid blend mode
	uint32_t blend_count = 0;
	xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, app_config_view, 1, &blend_count, &xr_blend);

	// Retrieve the OpenGL graphics requirements
	XrGraphicsRequirementsOpenGLKHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
	res = ext_xrGetOpenGLGraphicsRequirementsKHR(xr_instance, xr_system_id, &requirement);
	if (XR_FAILED(res)) {
		MessageBox(nullptr, _T("Failed to get OpenGL graphics requirements\n"), _T("Error"), MB_OK);
		return false;
	}

	// Ensure we have a current OpenGL context here
	if (!wglGetCurrentContext()) {
		MessageBox(nullptr, _T("No current OpenGL context found\n"), _T("Error"), MB_OK);
		return false;
	}

	// Create the session with the OpenGL context
	XrGraphicsBindingOpenGLWin32KHR graphicsBinding = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
	graphicsBinding.hDC = wglGetCurrentDC();
	graphicsBinding.hGLRC = wglGetCurrentContext();

	XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
	sessionInfo.next = &graphicsBinding;
	sessionInfo.systemId = xr_system_id;

	res = xrCreateSession(xr_instance, &sessionInfo, &xr_session);
	if (XR_FAILED(res) || xr_session == XR_NULL_HANDLE) {
		MessageBox(nullptr, _T("Failed to create OpenXR session\n"), _T("Error"), MB_OK);
		return false;
	}

	// Create a reference space
	XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	ref_space.poseInReferenceSpace = xr_pose_identity;
	ref_space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	xrCreateReferenceSpace(xr_session, &ref_space, &xr_app_space);

	// Query view configuration
	uint32_t view_count = 0;
	xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, 0, &view_count, nullptr);
	xr_config_views.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	xr_views.resize(view_count, { XR_TYPE_VIEW });
	xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, view_count, &view_count, xr_config_views.data());

	// Create OpenGL swapchains for each view
	for (uint32_t i = 0; i < view_count; i++) {
		XrViewConfigurationView& view = xr_config_views[i];
		XrSwapchainCreateInfo swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		XrSwapchain handle;
		swapchain_info.arraySize = 1;
		swapchain_info.mipCount = 1;
		swapchain_info.faceCount = 1;
		swapchain_info.format = swapchain_format;
		swapchain_info.width = view.recommendedImageRectWidth;
		swapchain_info.height = view.recommendedImageRectHeight;
		swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
		swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

		res = xrCreateSwapchain(xr_session, &swapchain_info, &handle);
		if (XR_FAILED(res)) {
			MessageBox(nullptr, _T("Failed to create OpenXR swapchain\n"), _T("Error"), MB_OK);
			return false;
		}

		uint32_t surface_count = 0;
		xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

		swapchain_t swapchain = {};
		swapchain.width = swapchain_info.width;
		swapchain.height = swapchain_info.height;
		swapchain.handle = handle;
		swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
		swapchain.surface_data.resize(surface_count);

		xrEnumerateSwapchainImages(swapchain.handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());
		for (uint32_t img_i = 0; img_i < surface_count; img_i++) {
			swapchain.surface_data[img_i] = gl_make_surface_data((XrBaseInStructure&)swapchain.surface_images[img_i], swapchain.width, swapchain.height);
		}
		xr_swapchains.push_back(swapchain);
	}

	return true;
}


///////////////////////////////////////////

void openxr_make_actions() {
	XrActionSetCreateInfo actionset_info = { XR_TYPE_ACTION_SET_CREATE_INFO };
	strcpy_s(actionset_info.actionSetName, "gameplay");
	strcpy_s(actionset_info.localizedActionSetName, "Gameplay");
	xrCreateActionSet(xr_instance, &actionset_info, &xr_input.actionSet);
	xrStringToPath(xr_instance, "/user/hand/left", &xr_input.handSubactionPath[0]);
	xrStringToPath(xr_instance, "/user/hand/right", &xr_input.handSubactionPath[1]);

	// Create an action to track the position and orientation of the hands! This is
	// the controller location, or the center of the palms for actual hands.
	XrActionCreateInfo action_info = { XR_TYPE_ACTION_CREATE_INFO };
	action_info.countSubactionPaths = _countof(xr_input.handSubactionPath);
	action_info.subactionPaths = xr_input.handSubactionPath;
	action_info.actionType = XR_ACTION_TYPE_POSE_INPUT;
	strcpy_s(action_info.actionName, "hand_pose");
	strcpy_s(action_info.localizedActionName, "Hand Pose");
	xrCreateAction(xr_input.actionSet, &action_info, &xr_input.poseAction);

	// Create an action for listening to the select action! This is primary trigger
	// on controllers, and an airtap on HoloLens
	action_info.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy_s(action_info.actionName, "select");
	strcpy_s(action_info.localizedActionName, "Select");
	xrCreateAction(xr_input.actionSet, &action_info, &xr_input.selectAction);

	// Bind the actions we just created to specific locations on the Khronos simple_controller
	// definition! These are labeled as 'suggested' because they may be overridden by the runtime
	// preferences. For example, if the runtime allows you to remap buttons, or provides input
	// accessibility settings.
	XrPath profile_path;
	XrPath pose_path[2];
	XrPath select_path[2];
	xrStringToPath(xr_instance, "/user/hand/left/input/grip/pose", &pose_path[0]);
	xrStringToPath(xr_instance, "/user/hand/right/input/grip/pose", &pose_path[1]);
	xrStringToPath(xr_instance, "/user/hand/left/input/select/click", &select_path[0]);
	xrStringToPath(xr_instance, "/user/hand/right/input/select/click", &select_path[1]);
	xrStringToPath(xr_instance, "/interaction_profiles/khr/simple_controller", &profile_path);
	XrActionSuggestedBinding bindings[] = {
		{ xr_input.poseAction,   pose_path[0]   },
		{ xr_input.poseAction,   pose_path[1]   },
		{ xr_input.selectAction, select_path[0] },
		{ xr_input.selectAction, select_path[1] }, };
	XrInteractionProfileSuggestedBinding suggested_binds = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
	suggested_binds.interactionProfile = profile_path;
	suggested_binds.suggestedBindings = &bindings[0];
	suggested_binds.countSuggestedBindings = _countof(bindings);
	xrSuggestInteractionProfileBindings(xr_instance, &suggested_binds);

	// Create frames of reference for the pose actions
	for (int32_t i = 0; i < 2; i++) {
		XrActionSpaceCreateInfo action_space_info = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
		action_space_info.action = xr_input.poseAction;
		action_space_info.poseInActionSpace = xr_pose_identity;
		action_space_info.subactionPath = xr_input.handSubactionPath[i];
		xrCreateActionSpace(xr_session, &action_space_info, &xr_input.handSpace[i]);
	}

	// Attach the action set we just made to the session
	XrSessionActionSetsAttachInfo attach_info = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
	attach_info.countActionSets = 1;
	attach_info.actionSets = &xr_input.actionSet;
	xrAttachSessionActionSets(xr_session, &attach_info);
}

///////////////////////////////////////////

void openxr_shutdown() {
	// We used a graphics API to initialize the swapchain data, so we'll
	// give it a chance to release anythig here!
	for (int32_t i = 0; i < xr_swapchains.size(); i++) {
		xrDestroySwapchain(xr_swapchains[i].handle);
		gl_swapchain_destroy(xr_swapchains[i]);
	}
	xr_swapchains.clear();

	// Release all the other OpenXR resources that we've created!
	// What gets allocated, must get deallocated!
	if (xr_input.actionSet != XR_NULL_HANDLE) {
		if (xr_input.handSpace[0] != XR_NULL_HANDLE) xrDestroySpace(xr_input.handSpace[0]);
		if (xr_input.handSpace[1] != XR_NULL_HANDLE) xrDestroySpace(xr_input.handSpace[1]);
		xrDestroyActionSet(xr_input.actionSet);
	}
	if (xr_app_space != XR_NULL_HANDLE) xrDestroySpace(xr_app_space);
	if (xr_session != XR_NULL_HANDLE) xrDestroySession(xr_session);
	if (xr_debug != XR_NULL_HANDLE) ext_xrDestroyDebugUtilsMessengerEXT(xr_debug);
	if (xr_instance != XR_NULL_HANDLE) xrDestroyInstance(xr_instance);
}

void opengl_shutdown() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

///////////////////////////////////////////
void openxr_poll_events(bool& exit) {
	exit = false;

	XrEventDataBuffer event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };

	while (xrPollEvent(xr_instance, &event_buffer) == XR_SUCCESS) {
		switch (event_buffer.type) {
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
			XrEventDataSessionStateChanged* changed = (XrEventDataSessionStateChanged*)&event_buffer;
			xr_session_state = changed->state;

			// Session state change is where we can begin and end sessions, as well as find quit messages!
			switch (xr_session_state) {
			case XR_SESSION_STATE_READY: {
				XrSessionBeginInfo begin_info = { XR_TYPE_SESSION_BEGIN_INFO };
				begin_info.primaryViewConfigurationType = app_config_view;
				xrBeginSession(xr_session, &begin_info);
				xr_running = true;
			} break;
			case XR_SESSION_STATE_STOPPING: {
				xr_running = false;
				xrEndSession(xr_session);
			} break;
			case XR_SESSION_STATE_EXITING:      exit = true;              break;
			case XR_SESSION_STATE_LOSS_PENDING: exit = true;              break;
			}
		} break;
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: exit = true; return;
		}
		event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };
	}
}

///////////////////////////////////////////

void openxr_poll_actions() {
	if (xr_session_state != XR_SESSION_STATE_FOCUSED)
		return;

	// Update our action set with up-to-date input data!
	XrActiveActionSet action_set = { };
	action_set.actionSet = xr_input.actionSet;
	action_set.subactionPath = XR_NULL_PATH;

	XrActionsSyncInfo sync_info = { XR_TYPE_ACTIONS_SYNC_INFO };
	sync_info.countActiveActionSets = 1;
	sync_info.activeActionSets = &action_set;

	xrSyncActions(xr_session, &sync_info);

	// Now we'll get the current states of our actions, and store them for later use
	for (uint32_t hand = 0; hand < 2; hand++) {
		XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
		get_info.subactionPath = xr_input.handSubactionPath[hand];

		XrActionStatePose pose_state = { XR_TYPE_ACTION_STATE_POSE };
		get_info.action = xr_input.poseAction;
		xrGetActionStatePose(xr_session, &get_info, &pose_state);
		xr_input.renderHand[hand] = pose_state.isActive;

		// Events come with a timestamp
		XrActionStateBoolean select_state = { XR_TYPE_ACTION_STATE_BOOLEAN };
		get_info.action = xr_input.selectAction;
		xrGetActionStateBoolean(xr_session, &get_info, &select_state);
		xr_input.handSelect[hand] = select_state.currentState && select_state.changedSinceLastSync;

		// If we have a select event, update the hand pose to match the event's timestamp
		if (xr_input.handSelect[hand]) {
			XrSpaceLocation space_location = { XR_TYPE_SPACE_LOCATION };
			XrResult        res = xrLocateSpace(xr_input.handSpace[hand], xr_app_space, select_state.lastChangeTime, &space_location);
			if (XR_UNQUALIFIED_SUCCESS(res) &&
				(space_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
				(space_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
				xr_input.handPose[hand] = space_location.pose;
			}
		}
	}
}

///////////////////////////////////////////

void openxr_poll_predicted(XrTime predicted_time) {
	if (xr_session_state != XR_SESSION_STATE_FOCUSED)
		return;

	// Update hand position based on the predicted time of when the frame will be rendered! This 
	// should result in a more accurate location, and reduce perceived lag.
	for (size_t i = 0; i < 2; i++) {
		if (!xr_input.renderHand[i])
			continue;
		XrSpaceLocation spaceRelation = { XR_TYPE_SPACE_LOCATION };
		XrResult        res = xrLocateSpace(xr_input.handSpace[i], xr_app_space, predicted_time, &spaceRelation);
		if (XR_UNQUALIFIED_SUCCESS(res) &&
			(spaceRelation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
			(spaceRelation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
			xr_input.handPose[i] = spaceRelation.pose;
		}
	}
}

///////////////////////////////////////////

void openxr_render_frame() {
	// Block until the previous frame is finished displaying, and is ready for another one.
	// Also returns a prediction of when the next frame will be displayed, for use with predicting
	// locations of controllers, viewpoints, etc.
	XrFrameState frame_state = { XR_TYPE_FRAME_STATE };
	xrWaitFrame(xr_session, nullptr, &frame_state);
	// Must be called before any rendering is done! This can return some interesting flags, like 
	// XR_SESSION_VISIBILITY_UNAVAILABLE, which means we could skip rendering this frame and call
	// xrEndFrame right away.
	xrBeginFrame(xr_session, nullptr);

	// Execute any code that's dependant on the predicted time, such as updating the location of
	// controller models.
	openxr_poll_predicted(frame_state.predictedDisplayTime);
	app_update_predicted();

	// If the session is active, lets render our layer in the compositor!
	XrCompositionLayerBaseHeader* layer = nullptr;
	XrCompositionLayerProjection             layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	vector<XrCompositionLayerProjectionView> views;
	bool session_active = xr_session_state == XR_SESSION_STATE_VISIBLE || xr_session_state == XR_SESSION_STATE_FOCUSED;
	if (session_active && openxr_render_layer(frame_state.predictedDisplayTime, views, layer_proj)) {
		layer = (XrCompositionLayerBaseHeader*)&layer_proj;
	}

	// We're finished with rendering our layer, so send it off for display!
	XrFrameEndInfo end_info{ XR_TYPE_FRAME_END_INFO };
	end_info.displayTime = frame_state.predictedDisplayTime;
	end_info.environmentBlendMode = xr_blend;
	end_info.layerCount = layer == nullptr ? 0 : 1;
	end_info.layers = &layer;
	xrEndFrame(xr_session, &end_info);
}

///////////////////////////////////////////

bool openxr_render_layer(XrTime predictedTime, vector<XrCompositionLayerProjectionView>& views, XrCompositionLayerProjection& layer) {

	// Find the state and location of each viewpoint at the predicted time
	uint32_t         view_count = 0;
	XrViewState      view_state = { XR_TYPE_VIEW_STATE };
	XrViewLocateInfo locate_info = { XR_TYPE_VIEW_LOCATE_INFO };
	locate_info.viewConfigurationType = app_config_view;
	locate_info.displayTime = predictedTime;
	locate_info.space = xr_app_space;
	xrLocateViews(xr_session, &locate_info, &view_state, (uint32_t)xr_views.size(), &view_count, xr_views.data());
	views.resize(view_count);

	// And now we'll iterate through each viewpoint, and render it!
	for (uint32_t i = 0; i < view_count; i++) {

		// We need to ask which swapchain image to use for rendering! Which one will we get?
		// Who knows! It's up to the runtime to decide.
		uint32_t                    img_id;
		XrSwapchainImageAcquireInfo acquire_info = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
		xrAcquireSwapchainImage(xr_swapchains[i].handle, &acquire_info, &img_id);

		// Wait until the image is available to render to. The compositor could still be
		// reading from it.
		XrSwapchainImageWaitInfo wait_info = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		wait_info.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(xr_swapchains[i].handle, &wait_info);

		// Set up our rendering information for the viewpoint we're using right now!
		views[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
		views[i].pose = xr_views[i].pose;
		views[i].fov = xr_views[i].fov;
		views[i].subImage.swapchain = xr_swapchains[i].handle;
		views[i].subImage.imageRect.offset = { 0, 0 };
		views[i].subImage.imageRect.extent = { xr_swapchains[i].width, xr_swapchains[i].height };

		// Call the rendering callback with our view and swapchain info
		gl_render_layer(views[i], xr_swapchains[i].surface_data[img_id], window);

		// And tell OpenXR we're done with rendering to this one!
		XrSwapchainImageReleaseInfo release_info = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(xr_swapchains[i].handle, &release_info);
	}

	layer.space = xr_app_space;
	layer.viewCount = (uint32_t)views.size();
	layer.views = views.data();
	return true;
}

///////////////////////////////////////////
// OpenGL code                          //
///////////////////////////////////////////

// Compile a GLSL shader and return its ID
GLuint gl_compile_shader(GLenum type, const char* source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[512];
		glGetShaderInfoLog(shader, 512, NULL, log);
		printf("Shader compile error: %s\n", log);
	}
	return shader;
}

// Link vertex and fragment shader into a program
GLuint gl_create_program(const char* vs_src, const char* fs_src) {
	GLuint vs = gl_compile_shader(GL_VERTEX_SHADER, vs_src);
	GLuint fs = gl_compile_shader(GL_FRAGMENT_SHADER, fs_src);

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);

	glDeleteShader(vs);
	glDeleteShader(fs);

	GLint success = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (!success) {
		char log[512];
		glGetProgramInfoLog(prog, 512, NULL, log);
		printf("Program link error: %s\n", log);
	}

	return prog;
}

// Convert from XrFovf to a projection matrix using glm
glm::mat4 gl_xr_projection(XrFovf fov, float clip_near, float clip_far) {
	float left = clip_near * tanf(fov.angleLeft);
	float right = clip_near * tanf(fov.angleRight);
	float down = clip_near * tanf(fov.angleDown);
	float up = clip_near * tanf(fov.angleUp);

	// Construct an off-center projection matrix
	return glm::frustum(left, right, down, up, clip_near, clip_far);
}

swapchain_surfdata_t gl_make_surface_data(XrBaseInStructure& swapchain_img, int32_t width, int32_t height) {
	swapchain_surfdata_t result = {};

	// Cast to the OpenGL swapchain image type
	XrSwapchainImageOpenGLKHR& gl_swapchain_img = (XrSwapchainImageOpenGLKHR&)swapchain_img;
	GLuint colorTexture = gl_swapchain_img.image;

	// Create an FBO and attach the colorTexture and a depth buffer
	glGenFramebuffers(1, &result.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, result.fbo);

	glGenRenderbuffers(1, &result.depthbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, result.depthbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, result.depthbuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("Failed to create framebuffer for swapchain image\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return result;
}

// Swaping between the headset and desktop window framebuffers
void gl_render_layer(XrCompositionLayerProjectionView& view, swapchain_surfdata_t& surface, GLFWwindow* window) {
	// Render to the headset's framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, surface.fbo);
	glViewport(view.subImage.imageRect.offset.x, view.subImage.imageRect.offset.y,
		view.subImage.imageRect.extent.width, view.subImage.imageRect.extent.height); // set the viewport to headset
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	extern void app_draw(XrCompositionLayerProjectionView & view);
	app_draw(view); // first draw for the headset
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Render to the desktop window's default framebuffer
	glfwMakeContextCurrent(window); // make the desktop window current context
	int windowWidth, windowHeight;
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	app_draw(view); // second draw for the desktop window
}


void gl_swapchain_destroy(swapchain_t& swapchain) {
	for (auto& surf : swapchain.surface_data) {
		glDeleteRenderbuffers(1, &surf.depthbuffer);
		glDeleteFramebuffers(1, &surf.fbo);
	}
}

///////////////////////////////////////////
// App                                   //
///////////////////////////////////////////

void app_init() {
	app_shader_program = gl_create_program(vertex_glsl, fragment_glsl);

	// Create VAO/VBO/EBO
	glGenVertexArrays(1, &app_vao);
	glGenBuffers(1, &app_vbo);
	glGenBuffers(1, &app_ebo);

	glBindVertexArray(app_vao);

	glBindBuffer(GL_ARRAY_BUFFER, app_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(app_verts), app_verts, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(app_inds), app_inds, GL_STATIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	// Normal attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	// Create a UBO for transform data
	glGenBuffers(1, &app_uniform_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, app_uniform_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(app_transform_buffer_t), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// The binding point 0 matches layout(binding = 0) in the shader if used,
	// or you can use glGetUniformBlockIndex/glUniformBlockBinding to link them.
	GLuint blockIndex = glGetUniformBlockIndex(app_shader_program, "TransformBuffer");
	glUniformBlockBinding(app_shader_program, blockIndex, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, app_uniform_buffer);

	// ----------------------------
	// SKYBOX SETUP
	// ----------------------------
	skyboxShaderProgram = gl_create_program(cubemap_vertex_glsl, cubemap_fragment_glsl);

	// Create skybox VAO/VBO/EBO
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glGenBuffers(1, &skyboxEBO);
	glBindVertexArray(skyboxVAO);

	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// Load the cubemap textures
	vector<string> faces
	{
		"Resources/right.png",
		"Resources/left.png",
		"Resources/top.png",
		"Resources/bottom.png",
		"Resources/front.png",
		"Resources/back.png"
	};
	cubemapTexture = loadCubemap(faces);

	cube_shader_program = gl_create_program(cube_vertex_glsl, cube_fragment_glsl);

	glBindVertexArray(0);
}

void app_draw(XrCompositionLayerProjectionView& view) {
	// Compute view and projection matrices
	glm::mat4 mat_projection = gl_xr_projection(view.fov, 0.05f, 100.0f);
	glm::quat orientation(view.pose.orientation.w, view.pose.orientation.x, view.pose.orientation.y, view.pose.orientation.z);
	glm::vec3 position(view.pose.position.x, view.pose.position.y, view.pose.position.z);

	glm::mat4 mat_view = glm::inverse(glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(orientation));

	// Draw SKYBOX
	glDepthFunc(GL_LEQUAL); // Ensure skybox passes depth test
	glUseProgram(skyboxShaderProgram);

	// Remove translation from view matrix for skybox
	glm::mat4 mat_view_skybox = glm::mat4(glm::mat3(mat_view));

	GLuint uProjLoc = glGetUniformLocation(skyboxShaderProgram, "uProjection");
	GLuint uViewLoc = glGetUniformLocation(skyboxShaderProgram, "uView");
	glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, &mat_projection[0][0]);
	glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, &mat_view_skybox[0][0]);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawElements(GL_TRIANGLES, sizeof(skyboxIndices) / sizeof(skyboxIndices[0]), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	glDepthFunc(GL_LESS); // Reset to default depth func

	// DRAW CUBES 
	glUseProgram(cube_shader_program);

	// Update the uniform buffer with viewproj and world
	// We'll draw each cube individually, updating the world matrix each time.
	
	app_transform_buffer_t transform_buffer;
	transform_buffer.viewproj = mat_projection * mat_view;
	
	glBindBuffer(GL_UNIFORM_BUFFER, app_uniform_buffer);

	glBindVertexArray(app_vao);

	for (size_t i = 0; i < app_cubes.size(); i++) {
		glm::quat cube_orientation(app_cubes[i].orientation.w, app_cubes[i].orientation.x,
			app_cubes[i].orientation.y, app_cubes[i].orientation.z);
		glm::vec3 cube_pos(app_cubes[i].position.x, app_cubes[i].position.y, app_cubes[i].position.z);

		glm::mat4 mat_model = glm::translate(glm::mat4(1.0f), cube_pos) * glm::mat4_cast(cube_orientation) * glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));
		transform_buffer.world = mat_model;

		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(app_transform_buffer_t), &transform_buffer);
		glDrawElements(GL_TRIANGLES, (GLsizei)(sizeof(app_inds) / sizeof(app_inds[0])), GL_UNSIGNED_SHORT, 0);
	}

	// TODO - FOREACH LOOP FOR DRAWING MODELS FROM A LIST (VECTOR) OF MODELS FOR SCALABILITY

	glUseProgram(app_shader_program);

	// DRAW ROCK MODEL and SCENE MODEL
	glm::mat4 modelMatrix = 
		glm::translate(glm::mat4(1.0f), 
		glm::vec3(0.0f, 0.0f, -5.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

	drawModel(rockModel, transform_buffer.viewproj, modelMatrix);
	drawModel(sceneModel, transform_buffer.viewproj, modelMatrix);

	glBindVertexArray(0);
	glUseProgram(0);
}

///////////////////////////////////////////

// If user hits trigger, spawn a cube at the hand's location
void app_update() {
	// If the user presses the select action, lets add a cube at that location!
	for (uint32_t i = 0; i < 2; i++) {
		if (xr_input.handSelect[i])
			app_cubes.push_back(xr_input.handPose[i]);
	}
}

///////////////////////////////////////////

// Update the location of the hand cubes (Controllers that are viewable in-game)
void app_update_predicted() {
	// Update the location of the hand cubes. This is done after the inputs have been updated to 
	// use the predicted location, but during the render code, so we have the most up-to-date location.
	if (app_cubes.size() < 2)
		app_cubes.resize(2, xr_pose_identity);
	for (uint32_t i = 0; i < 2; i++) {
		app_cubes[i] = xr_input.renderHand[i] ? xr_input.handPose[i] : xr_pose_identity;
	}
}

///////////////////////////////////////////

void calculate_framerate() {
	// Update the time and counter
	crntTime = glfwGetTime();
	timeDiff = crntTime - prevTime;
	counter++;

	if (timeDiff >= 1.0 / 30.0)
	{
		// Update the window title
		string FPS = std::to_string(static_cast<int>(((1.0 / timeDiff) * counter)));
		string newTitle = "Chisel Engine - OpenGL 4.3 - " + FPS + "FPS";
		glfwSetWindowTitle(window, newTitle.c_str());

		// Lastly, reset the time and counter
		prevTime = crntTime;
		counter = 0;
	}
}

///////////////////////////////////////////

GLuint loadCubemap(vector<string> faces)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	stbi_set_flip_vertically_on_load(false); // Typically not flipping for cube maps
	int width, height, nrChannels;
	for (GLuint i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			// JPG = GL_RGB and PNG = GL_RGBA
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			MessageBox(nullptr, _T("Cubemap texture failed to load"), _T("Error"), MB_OK);
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

///////////////////////////////////////////

Model loadModel(const std::string& path) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	// if the model did not load properly or has no meshes, exit
	if (!scene || !scene->HasMeshes()) {
		exit(EXIT_FAILURE);
	}

	aiMesh* mesh = scene->mMeshes[0];

	std::vector<float> vertices;
	std::vector<uint32_t> indices;

	// Extract vertex data
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		aiVector3D pos = mesh->mVertices[i];
		aiVector3D norm = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0.0f, 0.0f, 0.0f);
		vertices.insert(vertices.end(), { pos.x, pos.y, pos.z, norm.x, norm.y, norm.z });
	}

	// Extract index data
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}

	// OpenGL buffers
	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

	// Set vertex attributes (position and normal)
	glEnableVertexAttribArray(0); // Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1); // Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	return { vao, vbo, ebo, indices.size() };
}

void drawModel(const Model& model, const glm::mat4& viewProj, const glm::mat4& modelMatrix) {
	glUseProgram(app_shader_program);

	// Update transformation matrices
	app_transform_buffer_t transformBuffer;
	transformBuffer.viewproj = viewProj;
	transformBuffer.world = modelMatrix;

	glBindBuffer(GL_UNIFORM_BUFFER, app_uniform_buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(app_transform_buffer_t), &transformBuffer);

	// Bind the model's texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, model.textureID);
	glUniform1i(glGetUniformLocation(app_shader_program, "texture_diffuse"), 0);

	// Draw the model
	glBindVertexArray(model.vao);
	glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glUseProgram(0);
}


void cleanupModel(const Model& model) {
	glDeleteBuffers(1, &model.vbo);
	glDeleteBuffers(1, &model.ebo);
	glDeleteVertexArrays(1, &model.vao);
}

///////////////////////////////////////////

Model loadModelWithTexture(const std::string& objPath, const std::string& texturePath) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(objPath,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

	if (!scene || !scene->HasMeshes()) {
		std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
		exit(EXIT_FAILURE);
	}

	aiMesh* mesh = scene->mMeshes[0];

	std::vector<float> vertices;
	std::vector<uint32_t> indices;

	// Extract vertex data: position (3), normal (3), tex coords (2)
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		aiVector3D pos = mesh->mVertices[i];
		aiVector3D norm = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0.0f, 0.0f, 0.0f);
		aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);

		vertices.insert(vertices.end(), {
			pos.x, pos.y, pos.z,       // Position
			norm.x, norm.y, norm.z,    // Normal
			texCoord.x, texCoord.y     // Texture coordinates
			});
	}

	// Extract indices
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}

	// Load texture
	GLuint textureID = loadTexture(texturePath);

	// Generate VAO, VBO, and EBO
	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	// Bind VAO
	glBindVertexArray(vao);

	// Bind and fill VBO
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Bind and fill EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

	// Configure vertex attributes
	glEnableVertexAttribArray(0); // Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1); // Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(2); // Texture coordinate attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	// Unbind VAO (optional, to avoid accidental changes)
	glBindVertexArray(0);

	return { vao, vbo, ebo, indices.size(), textureID };
}


GLuint loadTexture(const string& path) {
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

	if (data) {
		GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	stbi_image_free(data);
	return textureID;
}



