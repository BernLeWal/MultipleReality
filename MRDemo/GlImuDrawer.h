// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// IMU ... inertial measurement unit (IMU)

#pragma once

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include "GlTypes.h"

class GlImuDrawer
{
public:
	void render(const rs2::motion_frame& frame, const rect& r);

	GLuint get_gl_handle() { return _gl_handle; }

private:
	GLuint _gl_handle = 0;

	void draw_motion(const rs2::motion_frame& f, const rect& r);

	//IMU drawing helper functions
	void multiply_vector_by_matrix(GLfloat vec[], GLfloat mat[], GLfloat* result);
	float2 xyz_to_xy(float x, float y, float z, GLfloat model[], GLfloat proj[], float vec_norm);

	void print_text_in_3d(float x, float y, float z, const char* text, bool center_text, GLfloat model[], GLfloat proj[], float vec_norm);

	static void  draw_axes(float axis_size = 1.f, float axisWidth = 4.f);

	// intensity is grey intensity
	static void draw_circle(float xx, float xy, float xz, float yx, float yy, float yz, float radius = 1.1, float3 center = { 0.0, 0.0, 0.0 }, float intensity = 0.5f);	
};

