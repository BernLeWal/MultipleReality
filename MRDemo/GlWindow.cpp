#include "GlWindow.h"

GlWindow::GlWindow(int width, int height, const char* title)
: _width(width), _height(height)
{
	glfwInit();
	win = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!win)
		throw std::runtime_error("Could not open OpenGL window, please check your graphic drivers or use the textual SDK tools");
	glfwMakeContextCurrent(win);

	glfwSetWindowUserPointer(win, this);
	glfwSetMouseButtonCallback(win, [](GLFWwindow * w, int button, int action, int mods)
	{
		auto s = (GlWindow*)glfwGetWindowUserPointer(w);
		if (button == 0) s->on_left_mouse(action == GLFW_PRESS);
	});

	glfwSetScrollCallback(win, [](GLFWwindow * w, double xoffset, double yoffset)
	{
		auto s = (GlWindow*)glfwGetWindowUserPointer(w);
		s->on_mouse_scroll(xoffset, yoffset);
	});

	glfwSetCursorPosCallback(win, [](GLFWwindow * w, double x, double y)
	{
		auto s = (GlWindow*)glfwGetWindowUserPointer(w);
		s->on_mouse_move(x, y);
	});

	glfwSetKeyCallback(win, [](GLFWwindow * w, int key, int scancode, int action, int mods)
	{
		auto s = (GlWindow*)glfwGetWindowUserPointer(w);
		if (0 == action) // on key release
		{
			s->on_key_release(key);
		}
	});
}


GlWindow::~GlWindow()
{
	glfwDestroyWindow(win);
	glfwTerminate();
}


GlWindow::operator bool()
{
	glPopMatrix();
	glfwSwapBuffers(win);

	auto res = !glfwWindowShouldClose(win);

	glfwPollEvents();
	glfwGetFramebufferSize(win, &_width, &_height);

	// Clear the framebuffer
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, _width, _height);

	// Draw the images
	glPushMatrix();
	glfwGetWindowSize(win, &_width, &_height);
	glOrtho(0, _width, _height, 0, -1, +1);

	return res;
}

void GlWindow::show(rs2::frame frame)
{
	show(frame, { 0, 0, (float)_width, (float)_height });
}

void GlWindow::show(const rs2::frame& frame, const rect& rect)
{
	if (auto fs = frame.as<rs2::frameset>())
		render_frameset(fs, rect);
	if (auto vf = frame.as<rs2::video_frame>())
		render_video_frame(vf, rect);
	if (auto mf = frame.as<rs2::motion_frame>())
		render_motoin_frame(mf, rect);
}

void GlWindow::render_video_frame(const rs2::video_frame& f, const rect& r)
{
	auto& t = _textures[f.get_profile().unique_id()];
	t.render(f, r);
}

void GlWindow::render_motoin_frame(const rs2::motion_frame& f, const rect& r)
{
	auto& i = _imus[f.get_profile().unique_id()];
	i.render(f, r);
}

void GlWindow::render_frameset(const rs2::frameset& frames, const rect& r)
{
	std::vector<rs2::frame> supported_frames;
	for (auto f : frames)
	{
		if (can_render(f))
			supported_frames.push_back(f);
	}
	if (supported_frames.empty())
		return;

	std::sort(supported_frames.begin(), supported_frames.end(), [](rs2::frame first, rs2::frame second)
	{ return first.get_profile().stream_type() < second.get_profile().stream_type();  });

	auto image_grid = calc_grid(r, supported_frames);

	int image_index = 0;
	for (auto f : supported_frames)
	{
		auto r = image_grid.at(image_index);
		show(f, r);
		image_index++;
	}
}

bool GlWindow::can_render(const rs2::frame& f) const
{
	auto format = f.get_profile().format();
	switch (format)
	{
	case RS2_FORMAT_RGB8:
	case RS2_FORMAT_RGBA8:
	case RS2_FORMAT_Y8:
	case RS2_FORMAT_MOTION_XYZ32F:
		return true;
	default:
		return false;
	}
}

rect GlWindow::calc_grid(rect r, int streams)
{
	if (r.w <= 0 || r.h <= 0 || streams <= 0)
		throw std::runtime_error("invalid window configuration request, failed to calculate window grid");
	float ratio = r.w / r.h;
	auto x = sqrt(ratio * (float)streams);
	auto y = (float)streams / x;
	auto w = round(x);
	auto h = round(y);
	if (w == 0 || h == 0)
		throw std::runtime_error("invalid window configuration request, failed to calculate window grid");
	while (w*h > streams)
		h > w ? h-- : w--;
	while (w*h < streams)
		h > w ? w++ : h++;
	auto new_w = round(r.w / w);
	auto new_h = round(r.h / h);
	// column count, line count, cell width cell height
	return rect{ static_cast<float>(w), static_cast<float>(h), static_cast<float>(new_w), static_cast<float>(new_h) };
}

std::vector<rect> GlWindow::calc_grid(rect r, std::vector<rs2::frame>& frames)
{
	auto grid = calc_grid(r, frames.size());

	std::vector<rect> rv;
	int curr_line = -1;

	for (unsigned int i = 0; i < frames.size(); i++)
	{
		auto mod = i % (int)grid.x;
		float fw = IMU_FRAME_WIDTH;
		float fh = IMU_FRAME_HEIGHT;
		if (auto vf = frames[i].as<rs2::video_frame>())
		{
			fw = (float)vf.get_width();
			fh = (float)vf.get_height();
		}
		float cell_x_postion = (float)(mod * grid.w);
		if (mod == 0) curr_line++;
		float cell_y_position = curr_line * grid.h;
		float2 margin = { grid.w * 0.02f, grid.h * 0.02f };
		auto r = rect{ cell_x_postion + margin.x, cell_y_position + margin.y, grid.w - 2 * margin.x, grid.h };
		rv.push_back(r.adjust_ratio(float2{ fw, fh }));
	}

	return rv;
}
