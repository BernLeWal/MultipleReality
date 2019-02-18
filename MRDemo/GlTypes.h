// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#define PI 3.14159265358979323846
#define IMU_FRAME_WIDTH 1280
#define IMU_FRAME_HEIGHT 720
//////////////////////////////
// Basic Data Types         //
//////////////////////////////

struct float3 { float x, y, z; };
struct float2 { float x, y; };

struct rect
{
	float x, y;
	float w, h;

	// Create new rect within original boundaries with give aspect ration
	rect adjust_ratio(float2 size) const
	{
		auto H = static_cast<float>(h), W = static_cast<float>(h) * size.x / size.y;
		if (W > w)
		{
			auto scale = w / W;
			W *= scale;
			H *= scale;
		}

		return{ x + (w - W) / 2, y + (h - H) / 2, W, H };
	}
};

//////////////////////////////
// Simple font loading code //
//////////////////////////////

#include <stb_easy_font.h>

inline void draw_text(int x, int y, const char * text) 
{
	char buffer[60000]; // ~300 chars
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 16, buffer);
	glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print((float)x, (float)(y - 7), (char *)text, nullptr, buffer, sizeof(buffer)));
	glDisableClientState(GL_VERTEX_ARRAY);
}

inline void set_viewport(const rect& r) 
{
	glViewport((GLint)r.x, (GLint)r.y, (GLsizei)r.w, (GLsizei)r.h);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, r.w, r.h, 0, -1, +1);
}

