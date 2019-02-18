#include "GlImuDrawer.h"

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <map>

void GlImuDrawer::render(const rs2::motion_frame& frame, const rect& r)
{
	draw_motion(frame, r.adjust_ratio({ IMU_FRAME_WIDTH, IMU_FRAME_HEIGHT }));
}

void GlImuDrawer::draw_motion(const rs2::motion_frame& f, const rect& r)
{
	if (!_gl_handle)
		glGenTextures(1, &_gl_handle);

	set_viewport(r);
	draw_text((int)(0.05 * r.w), (int)(r.h - 0.1*r.h), f.get_profile().stream_name().c_str());

	auto md = f.get_motion_data();
	auto x = md.x;
	auto y = md.y;
	auto z = md.z;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glOrtho(-2.8, 2.8, -2.4, 2.4, -7, 7);

	glRotatef(25, 1.0f, 0.0f, 0.0f);

	glTranslatef(0, -0.33f, -1.f);

	glRotatef(-135, 0.0f, 1.0f, 0.0f);

	glRotatef(180, 0.0f, 0.0f, 1.0f);
	glRotatef(-90, 0.0f, 1.0f, 0.0f);

	draw_axes();

	draw_circle(1, 0, 0, 0, 1, 0);
	draw_circle(0, 1, 0, 0, 0, 1);
	draw_circle(1, 0, 0, 0, 0, 1);

	const auto canvas_size = 230;
	const auto vec_threshold = 0.01f;
	float norm = std::sqrt(x * x + y * y + z * z);
	if (norm < vec_threshold)
	{
		const auto radius = 0.05;
		static const int circle_points = 100;
		static const float angle = 2.0f * 3.1416f / circle_points;

		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_POLYGON);
		double angle1 = 0.0;
		glVertex2d(radius * cos(0.0), radius * sin(0.0));
		int i;
		for (i = 0; i < circle_points; i++)
		{
			glVertex2d(radius * cos(angle1), radius *sin(angle1));
			angle1 += angle;
		}
		glEnd();
	}
	else
	{
		auto vectorWidth = 5.f;
		glLineWidth(vectorWidth);
		glBegin(GL_LINES);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(x / norm, y / norm, z / norm);
		glEnd();

		// Save model and projection matrix for later
		GLfloat model[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, model);
		GLfloat proj[16];
		glGetFloatv(GL_PROJECTION_MATRIX, proj);

		glLoadIdentity();
		glOrtho(-canvas_size, canvas_size, -canvas_size, canvas_size, -1, +1);

		std::ostringstream s1;
		const auto precision = 3;

		glRotatef(180, 1.0f, 0.0f, 0.0f);

		s1 << "(" << std::fixed << std::setprecision(precision) << x << "," << std::fixed << std::setprecision(precision) << y << "," << std::fixed << std::setprecision(precision) << z << ")";
		print_text_in_3d(x, y, z, s1.str().c_str(), false, model, proj, 1 / norm);

		std::ostringstream s2;
		s2 << std::setprecision(precision) << norm;
		print_text_in_3d(x / 2, y / 2, z / 2, s2.str().c_str(), true, model, proj, 1 / norm);
	}
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

//IMU drawing helper functions
void GlImuDrawer::multiply_vector_by_matrix(GLfloat vec[], GLfloat mat[], GLfloat* result)
{
	const auto N = 4;
	for (int i = 0; i < N; i++)
	{
		result[i] = 0;
		for (int j = 0; j < N; j++)
		{
			result[i] += vec[j] * mat[N*j + i];
		}
	}
	return;
}

float2 GlImuDrawer::xyz_to_xy(float x, float y, float z, GLfloat model[], GLfloat proj[], float vec_norm)
{
	GLfloat vec[4] = { x, y, z, 0 };
	float tmp_result[4];
	float result[4];

	const auto canvas_size = 230;

	multiply_vector_by_matrix(vec, model, tmp_result);
	multiply_vector_by_matrix(tmp_result, proj, result);

	return{ canvas_size * vec_norm *result[0], canvas_size * vec_norm *result[1] };
}

void GlImuDrawer::print_text_in_3d(float x, float y, float z, const char* text, bool center_text, GLfloat model[], GLfloat proj[], float vec_norm)
{
	auto xy = xyz_to_xy(x, y, z, model, proj, vec_norm);
	auto w = (center_text) ? stb_easy_font_width((char*)text) : 0;
	glColor3f(1.0f, 1.0f, 1.0f);
	draw_text((int)(xy.x - w / 2), (int)xy.y, text);
}

void  GlImuDrawer::draw_axes(float axis_size, float axisWidth)
{
	// Triangles For X axis
	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(axis_size * 1.1f, 0.f, 0.f);
	glVertex3f(axis_size, -axis_size * 0.05f, 0.f);
	glVertex3f(axis_size, axis_size * 0.05f, 0.f);
	glVertex3f(axis_size * 1.1f, 0.f, 0.f);
	glVertex3f(axis_size, 0.f, -axis_size * 0.05f);
	glVertex3f(axis_size, 0.f, axis_size * 0.05f);
	glEnd();

	// Triangles For Y axis
	glBegin(GL_TRIANGLES);
	glColor3f(0.f, 1.f, 0.f);
	glVertex3f(0.f, axis_size * 1.1f, 0.0f);
	glVertex3f(0.f, axis_size, 0.05f * axis_size);
	glVertex3f(0.f, axis_size, -0.05f * axis_size);
	glVertex3f(0.f, axis_size * 1.1f, 0.0f);
	glVertex3f(0.05f * axis_size, axis_size, 0.f);
	glVertex3f(-0.05f * axis_size, axis_size, 0.f);
	glEnd();

	// Triangles For Z axis
	glBegin(GL_TRIANGLES);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 1.1f * axis_size);
	glVertex3f(0.0f, 0.05f * axis_size, 1.0f * axis_size);
	glVertex3f(0.0f, -0.05f * axis_size, 1.0f * axis_size);
	glVertex3f(0.0f, 0.0f, 1.1f * axis_size);
	glVertex3f(0.05f * axis_size, 0.f, 1.0f * axis_size);
	glVertex3f(-0.05f * axis_size, 0.f, 1.0f * axis_size);
	glEnd();

	glLineWidth(axisWidth);

	// Drawing Axis
	glBegin(GL_LINES);
	// X axis - Red
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(axis_size, 0.0f, 0.0f);

	// Y axis - Green
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, axis_size, 0.0f);

	// Z axis - Blue
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, axis_size);
	glEnd();
}

// intensity is grey intensity
void GlImuDrawer::draw_circle(float xx, float xy, float xz, float yx, float yy, float yz, float radius, float3 center, float intensity)
{
	const auto N = 50;
	glColor3f(intensity, intensity, intensity);
	glLineWidth(2);
	glBegin(GL_LINE_STRIP);

	for (int i = 0; i <= N; i++)
	{
		const double theta = (2 * PI / N) * i;
		const auto cost = static_cast<float>(cos(theta));
		const auto sint = static_cast<float>(sin(theta));
		glVertex3f(
			center.x + radius * (xx * cost + yx * sint),
			center.y + radius * (xy * cost + yy * sint),
			center.z + radius * (xz * cost + yz * sint)
		);
	}
	glEnd();
}

