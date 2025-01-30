#include "core/gameobject.cpp"
#include "core/audio.cpp"
#include "core/shaders.cpp"

int main() {
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

	// Check if openxr_init() fails
	if (!openxr_init("Single file OpenXR", OPENGL_SWAPCHAIN_FORMAT)) {
		MessageBox(nullptr, _T("OpenXR initialization failed\n"), _T("Error"), MB_OK);
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	// show gpu and opengl info
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "GPU: %s\n" << std::endl;
	std::cout << "OpenGL Version: %s\n" << std::endl;

	openxr_make_actions();
	app_init();
	game.start();

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

	// Ensure we have a current OpenGL context here to check if the window did its job
	if (!wglGetCurrentContext()) {
		MessageBox(nullptr, _T("No current OpenGL context found\n"), _T("Error"), MB_OK);
		return false;
	}

	// Create the session with the existing OpenGL context for headset rendering
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

	// View configuration
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
	// Cleanup the OpenGL resources we've created
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
	std::vector<XrCompositionLayerProjectionView> views;
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

bool openxr_render_layer(XrTime predictedTime, std::vector<XrCompositionLayerProjectionView>& views, XrCompositionLayerProjection& layer) {

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

	// Main app shader setup
	Shaders defaultShaders("Shaders/default.vert", "Shaders/default.frag");
	app_shader_program = gl_create_program(defaultShaders.vertexShader, defaultShaders.fragmentShader);

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

	// Skybox/Cubemap setup
	Shaders skyboxShaders("Shaders/cubemap.vert", "Shaders/cubemap.frag");
	skyboxShaderProgram = gl_create_program(skyboxShaders.vertexShader, skyboxShaders.fragmentShader);

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
	std::vector<std::string> faces
	{
		"Resources/right.png",
		"Resources/left.png",
		"Resources/top.png",
		"Resources/bottom.png",
		"Resources/front.png",
		"Resources/back.png"
	};
	cubemapTexture = loadCubemap(faces);

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

	// Update the uniform buffer with viewproj and world
	// We'll draw each cube individually, updating the world matrix each time.

	transform_buffer.viewproj = mat_projection * mat_view;
	
	glBindBuffer(GL_UNIFORM_BUFFER, app_uniform_buffer);

	glBindVertexArray(app_vao);

	Model controllerModel; // Controller Model - Default are Vive Controllers
	controllerModel.loadModel("Resources/VRController.obj", "Resources/htc_vive_controller.jpeg"); // replace with own controller function (setController())

	// for each of the two controllers
	for (size_t i = 0; i < 2; i++) {
		glm::quat controller_orientation(app_controllers[i].orientation.w, app_controllers[i].orientation.x,
			app_controllers[i].orientation.y, app_controllers[i].orientation.z);
		glm::vec3 controller_pos(app_controllers[i].position.x, app_controllers[i].position.y, app_controllers[i].position.z);

		glm::mat4 mat_model = glm::translate(glm::mat4(1.0f), controller_pos) * glm::mat4_cast(controller_orientation) * glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));
		transform_buffer.world = mat_model;

		controllerModel.drawModel(mat4ToTransform(mat_model)); // draw the controller model at the controller's location and orientation
	}
	
	glUseProgram(app_shader_program);

	// Set a default transform
	defaultTransform = mat4ToTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)));

	// Render models in the game logic
	game.render();

	glBindVertexArray(0);
	glUseProgram(0);
}

///////////////////////////////////////////

void app_update() {
	// run update logic for the game class
	game.update();
}

bool inputDetected() {
	for (uint32_t i = 0; i < 2; i++) {
		if (xr_input.handSelect[i]) { 
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////

// Update the location of the hand cubes (Controllers that are viewable in-game)
void app_update_predicted() {
	// Update the location of the hand cubes. This is done after the inputs have been updated to 
	// use the predicted location, but during the render code, so we have the most up-to-date location.
	if (app_controllers.size() < 2)
		app_controllers.resize(2, xr_pose_identity);
	for (uint32_t i = 0; i < 2; i++) {
		app_controllers[i] = xr_input.renderHand[i] ? xr_input.handPose[i] : xr_pose_identity;
	}
}

///////////////////////////////////////////

void calculate_framerate() {
	// Update the time and counter
	currentTime = glfwGetTime();
	timeDifference = currentTime - priorTime;
	counter++;

	if (timeDifference >= 1.0 / 30.0)
	{
		// Update the window title
		std::string FPS = std::to_string(static_cast<int>(((1.0 / timeDifference) * counter)));
		std::string newTitle = "Chisel Engine - OpenGL 4.3 - " + FPS + "FPS";
		glfwSetWindowTitle(window, newTitle.c_str());

		// Lastly, reset the time and counter
		priorTime = currentTime;
		counter = 0;
	}
}

///////////////////////////////////////////

GLuint loadCubemap(std::vector<std::string> faces)
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
