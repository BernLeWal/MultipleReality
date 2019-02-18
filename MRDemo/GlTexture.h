// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include "GlTypes.h"

////////////////////////
// Image display code //
////////////////////////
class GlTexture
{
	GLuint gl_handle = 0;
	rs2_stream stream = RS2_STREAM_ANY;

public:
	void render(const rs2::video_frame& frame, const rect& rect);
	void upload(const rs2::video_frame& frame);
	void uploadFile(char const *filename);
	void show(const rect& r) const;

	GLuint get_gl_handle() { return gl_handle; }
};

