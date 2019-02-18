// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>

#include "GlTypes.h"
#include "GlTexture.h"
#include "GlImuDrawer.h"

// Struct for managing rotation of pointcloud view
struct glfw_state {
	glfw_state() : yaw(15.0), pitch(15.0), last_x(0.0), last_y(0.0),
		ml(false), offset_x(2.f), offset_y(2.f), tex() {}
	double yaw;
	double pitch;
	double last_x;
	double last_y;
	bool ml;
	float offset_x;
	float offset_y;
	GlTexture tex;
};

////////////////////////////////////
// Window for displaying OpenGL   //
////////////////////////////////////

class GlWindow
{
public:
	std::function<void(bool)>           on_left_mouse = [](bool) {};
	std::function<void(double, double)> on_mouse_scroll = [](double, double) {};
	std::function<void(double, double)> on_mouse_move = [](double, double) {};
	std::function<void(int)>            on_key_release = [](int) {};

	GlWindow(int width, int height, const char* title);

	float width() const { return float(_width); }
	float height() const { return float(_height); }

	operator bool();
	
	~GlWindow();


	void show(rs2::frame frame);
	void show(const rs2::frame& frame, const rect& rect);

	operator GLFWwindow*() { return win; }

private:
	GLFWwindow * win;
	std::map<int, GlTexture> _textures;
	std::map<int, GlImuDrawer> _imus;
	int _width, _height;

	void render_video_frame(const rs2::video_frame& f, const rect& r);
	void render_motoin_frame(const rs2::motion_frame& f, const rect& r);
	void render_frameset(const rs2::frameset& frames, const rect& r);
	bool can_render(const rs2::frame& f) const;
	rect calc_grid(rect r, int streams);
	std::vector<rect> calc_grid(rect r, std::vector<rs2::frame>& frames);
};

