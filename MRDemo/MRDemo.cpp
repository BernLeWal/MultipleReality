#include <imgui/imgui.h>
#include "imgui/imgui_impl_glfw.h"

#define NOMINMAX
#include <Windows.h>			// GetTickCount()

#include "MRDemo.h"

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>            // std::min, std::max
#include <iomanip>
#include <cmath>
#include <map>
#include <filesystem>			// std::filesystem::current_path()


MRDemo::MRDemo() : GlWindow(1280, 720, "Multiple-Reality Demo")
, sceneSetup(settings)
, sceneSnap(settings)
, sceneIBC(settings)
, sceneTron(settings)
, sceneStartrek(settings)
{
	ImGui_ImplGlfw_Init(*this, false);      // ImGui library intializition
	// register callbacks to allow manipulation of the pointcloud
	glRegisterCallbacks();

	// Start streaming with default recommended configuration
    //Calling pipeline's start() without any additional parameters will start the first device
	// with its default streams.
	//The start function returns the pipeline profile which the pipeline used to start the device
	profile = pipe.start();

	pActScene = &sceneSetup;

	showSplashScreen = true;

	rotation_yaw = 0;
	rotation_yaw_delta = 0;
	rotation_max_angle = 15.0;
	rotation_velocity = 0.5f;
	rotation_last_tick = GetTickCount();

	char result[MAX_PATH];
	currentPath = std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
	std::cout << "Executed file is " << currentPath << std::endl;
	currentPath = currentPath.substr(0, currentPath.find_last_of("/\\"));
	std::string s =  currentPath + "\\SplashScreen.png";
	std::cout << "Loading splash image from " << s << std::endl;
	splashScreen.uploadFile(s.c_str());

	tick = GetTickCount();
	fps = 0.0f;
}


MRDemo::~MRDemo()
{
	//stbi_image_free(splashScreen);
}


bool MRDemo::run()
{
	// Wait for the next set of frames from the camera
	auto frames = pipe.wait_for_frames();
	// rs2::pipeline::wait_for_frames() can replace the device it uses in case of device error or disconnection.

	rs2::depth_frame depth = frames.get_depth_frame();
	if ( !depth )
		return true;		//If one of them is unavailable, continue iteration

	if (settings.density > 1) {
		depth = dec_filter.process(depth);
	}

	// Generate the pointcloud and texture mappings
	points = pc.calculate(depth);
	rs2::video_frame color = frames.get_color_frame();
	// For cameras that don't have RGB sensor, we'll map the pointcloud to infrared instead of color
	if (!color || !settings.colored )
		color = frames.get_infrared_frame();
	// Tell pointcloud object to map to this color frame
	pc.map_to(color);
	// Upload the color frame to OpenGL
	app_state.tex.upload(color);



	// Draw the pointcloud
	int pointCount = 0;
	if (points) {
		// Handles all the OpenGL calls needed to display the point cloud
		glPrepareScreen();
		pActScene->preRenderPointCloud();
		pointCount = pActScene->renderPointCloud(points);
		glCleanupScreen();
	}

	if (showSplashScreen) {
		splashScreen.show({ (width() - 1024.f) / 2.f, (height() - 564.f) / 2.f , 1024.f, 564.f });
	}

	// Using ImGui library to provide a GUI
	// Taking dimensions of the window for rendering purposes
	ImGui_ImplGlfw_NewFrame(1);

	// render the frame-rate
	long tick2 = GetTickCount();
	float fps = 1000.f / (tick2 - tick);
	tick = tick2;
	std::string status = std::to_string(pointCount / 1000) + "k points, "
		+ std::to_string((int)fps) + "." + std::to_string(((int)(fps*10.f)) % 10) + " fps";
	uiDrawText({ 30, height() - 30, 200, 30 }, status);

	pActScene->renderImgUI(width(), height(), depth, color);

	ImGui::Render();

	return true;
}


void MRDemo::glPrepareScreen()
{
	glLoadIdentity();
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glClearColor(153.f / 255, 153.f / 255, 153.f / 255, 1);
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	gluPerspective(60, width() / height(), 0.01f, 10.0f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	gluLookAt(0, 0, 0, 0, 0, 1, 0, -1, 0);

	glTranslatef(0, 0, +0.5f + app_state.offset_y*0.05f);
	glRotated(app_state.pitch, 1, 0, 0);
	// implement animated rotation
	if( settings.auto_rotation )
	{
		unsigned int rotation_curr_tick = GetTickCount();
		if (rotation_curr_tick - rotation_last_tick > 40) {
			if (fabs(rotation_yaw_delta) > rotation_max_angle)
				rotation_velocity = -rotation_velocity;
			rotation_yaw_delta += rotation_velocity;
			rotation_last_tick = rotation_curr_tick;
		}
	}
	glRotated(app_state.yaw + rotation_yaw_delta, 0, 1, 0);
	glTranslatef(0, 0, -0.5f);

	glPointSize(width() / 640);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, app_state.tex.get_gl_handle());
	float tex_border_color[] = { 0.8f, 0.8f, 0.8f, 0.8f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); // GL_CLAMP_TO_EDGE
}


void MRDemo::glCleanupScreen()
{
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
}



void MRDemo::glRegisterCallbacks()
{
	on_left_mouse = [&](bool pressed)
	{
		if (showSplashScreen)
			showSplashScreen = false;
		else
			app_state.ml = pressed;
	};

	on_mouse_scroll = [&](double xoffset, double yoffset)
	{
		app_state.offset_x -= static_cast<float>(xoffset);
		app_state.offset_y -= static_cast<float>(yoffset);
	};

	on_mouse_move = [&](double x, double y)
	{
		if (app_state.ml && ( pActScene->type() != EMRSceneType::SETUP || x > (MRSceneSetup::SLIDER_WINDOW_WIDTH +20) ) )
		{
			app_state.yaw = rotation_yaw;
			app_state.yaw -= (x - app_state.last_x);
			app_state.yaw = std::max(app_state.yaw, -120.0);
			app_state.yaw = std::min(app_state.yaw, +120.0);
			app_state.pitch += (y - app_state.last_y);
			app_state.pitch = std::max(app_state.pitch, -80.0);
			app_state.pitch = std::min(app_state.pitch, +80.0);
			rotation_yaw = app_state.yaw;
			rotation_yaw_delta = 0;
		}
		app_state.last_x = x;
		app_state.last_y = y;
	};

	on_key_release = [&](int key)
	{
		if (showSplashScreen) {
			showSplashScreen = false;
		}
		else if (key == GLFW_KEY_SPACE)
		{
			app_state.yaw = app_state.pitch = 0; app_state.offset_x = app_state.offset_y = 0.0;
			rotation_yaw = 0;
			rotation_yaw_delta = 0;
			settings.reset();
			if (pActScene->type() != EMRSceneType::SETUP) {
				pActScene = &sceneSnap;
			}
			std::cout << "reset to initial view" << std::endl;
		}
		else if (key == GLFW_KEY_SLASH /* DE:[-] */ || key == GLFW_KEY_KP_SUBTRACT) {
			if (settings.density < 8) {
				settings.density++;
				dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, settings.density);
			}
			std::cout << "density=" << settings.density << " //incremented" << std::endl;
		}
		else if (key == GLFW_KEY_RIGHT_BRACKET /* DE:[+] */ || key == GLFW_KEY_KP_ADD) {
			if (settings.density > 1) {
				settings.density--;
				dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, settings.density);
			}
			std::cout << "density=" << settings.density << " //decremented" << std::endl;
		}

		else if (key == GLFW_KEY_0 || key == GLFW_KEY_KP_0) {
			if (pActScene->type() == EMRSceneType::SETUP)
				pActScene = &sceneSnap;
			else
				pActScene = &sceneSetup;
		}
		else if (key == GLFW_KEY_1 || key == GLFW_KEY_KP_1) {
			// take a picture
			std::cout << "set mode=0 //take a picture" << std::endl;
			if (pActScene != &sceneSnap)
				pActScene = &sceneSnap;
			else
				pActScene->action();
		}
		else if (key == GLFW_KEY_2 || key == GLFW_KEY_KP_2)
		{
			// ice-bucket-challenge
			std::cout << "set mode=1 //ice-bucket-challenge" << std::endl;
			if(pActScene != &sceneIBC)
				pActScene = &sceneIBC;
			else if (!pActScene->action()) {
				pActScene = &sceneSnap;
			}
		}
		else if (key == GLFW_KEY_3 || key == GLFW_KEY_KP_3) {
			std::cout << "set mode=2 //tron laser" << std::endl;
			// tron laser
			if (pActScene != &sceneTron) {
				pActScene = &sceneTron;
				pActScene->activate();
			}
			if (!pActScene->action()) {
				pActScene = &sceneSnap;
			}
		}
		else if (key == GLFW_KEY_4 || key == GLFW_KEY_KP_4) {
			// star trek beaming
			std::cout << "set mode=3 //star trek beaming" << std::endl;
			if (pActScene != &sceneStartrek) {
				pActScene = &sceneStartrek;
				pActScene->activate();
			}
			if (!pActScene->action()) {
				pActScene = &sceneSnap;
			}
		}
		else if (key == GLFW_KEY_W) {
			MRSceneIBC* pSceneIBC = dynamic_cast<MRSceneIBC*>(pActScene);
			pSceneIBC->incWaterYPosition();
		}
		else if (key == GLFW_KEY_S) {
			MRSceneIBC* pSceneIBC = dynamic_cast<MRSceneIBC*>(pActScene);
			pSceneIBC->decWaterYPosition();
		}
		else if (key == GLFW_KEY_R) {
			settings.auto_rotation = !settings.auto_rotation;
		}

		else if (key == GLFW_KEY_ESCAPE)
		{
			exit(EXIT_SUCCESS);
		}
		else if (key == GLFW_KEY_BACKSPACE)
		{
			showSplashScreen = !showSplashScreen;
		}
		else
		{
			std::cout << "on_key_selease() unknown key=" << key << std::endl;
		}
	};
}

void MRDemo::uiDrawText(rect location, std::string& caption)
{
	// Some trickery to display the control nicely
	static const int flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove;

	ImGui::SetNextWindowPos({ location.x, location.y });
	ImGui::SetNextWindowSize({ location.w, location.h });

	ImGui::Begin("label", nullptr, flags);
	ImGui::Text("%s", caption.c_str());
	ImGui::End();
}

