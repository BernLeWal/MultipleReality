// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "GlTypes.h"
#include "GlWindow.h"

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include "MRScene.h"

class MRDemo : public GlWindow
{
private:
	glfw_state app_state;
	std::string currentPath;

	rs2::pointcloud pc;	// Pointcloud object, for calculating pointclouds and texture mappings
	rs2::points points;	// We want the points object to be persistent so we can display the last cloud when a frame drops
	rs2::pipeline pipe;	// RealSense pipeline, encapsulating the actual device and sensors
	rs2::pipeline_profile profile;

	MRSettings settings;
	MRSceneSetup sceneSetup;
	MRSceneSnapshot sceneSnap;
	MRSceneIBC sceneIBC;
	MRSceneTron sceneTron;
	MRSceneStartrek sceneStartrek;
	MRScene* pActScene = NULL;

	bool showSplashScreen;
	GlTexture splashScreen;

	double rotation_yaw;
	double rotation_yaw_delta;
	double rotation_max_angle;
	double rotation_velocity;
	unsigned int rotation_last_tick;

	long tick = 0;		// measure the frames-per-second
	float fps = 0.0f;	// stores the last calculated fps

private:
	// Helper functions

	// OpenGL drawing directly with glfw3
	void glPrepareScreen();		// OpenGL commands that prep screen for the pointcloud
	void glCleanupScreen();
	void glRegisterCallbacks();	// Registers the state variable and callbacks to allow mouse control of the pointcloud

	// ImGUI functions
	void uiDrawText(rect location, std::string& caption);

public:
	MRDemo();
	~MRDemo();

	bool run();
};

