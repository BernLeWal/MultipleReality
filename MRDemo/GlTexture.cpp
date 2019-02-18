#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "GlTexture.h"

void GlTexture::render(const rs2::video_frame& frame, const rect& rect)
{
	upload(frame);
	show(rect.adjust_ratio({ (float)frame.get_width(), (float)frame.get_height() }));
}

void GlTexture::upload(const rs2::video_frame& frame)
{
	if (!frame) return;

	if (!gl_handle)
		glGenTextures(1, &gl_handle);
	GLenum err = glGetError();

	auto format = frame.get_profile().format();
	auto width = frame.get_width();
	auto height = frame.get_height();
	stream = frame.get_profile().stream_type();

	glBindTexture(GL_TEXTURE_2D, gl_handle);

	switch (format)
	{
	case RS2_FORMAT_RGB8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.get_data());
		break;
	case RS2_FORMAT_RGBA8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.get_data());
		break;
	case RS2_FORMAT_Y8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.get_data());
		break;
	default:
		throw std::runtime_error("The requested format is not supported by this demo!");
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GlTexture::uploadFile(char const *filename)
{
	if (!filename)
		return;

	if (!gl_handle)
		glGenTextures(1, &gl_handle);

	int w, h, n;
	void* data = stbi_load(filename, &w, &h, &n, 4);

	glBindTexture(GL_TEXTURE_2D, gl_handle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GlTexture::show(const rect& r) const
{
	if (!gl_handle)
		return;

	set_viewport(r);

	glBindTexture(GL_TEXTURE_2D, gl_handle);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(0, 0);
	glTexCoord2f(0, 1); glVertex2f(0, r.h);
	glTexCoord2f(1, 1); glVertex2f(r.w, r.h);
	glTexCoord2f(1, 0); glVertex2f(r.w, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	draw_text((int)(0.05 * r.w), (int)(r.h - 0.05*r.h), rs2_stream_to_string(stream));
}
