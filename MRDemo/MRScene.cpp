#include <imgui/imgui.h>
#include "imgui/imgui_impl_glfw.h"

#define NOMINMAX
#include <Windows.h>				// GetTickCount(), PlaySound()
#pragma comment(lib, "Winmm.lib")	// PlaySound()

#include "MRScene.h"

#include <string>
#include <sstream>
#include <iostream>
#include <atomic>
#include <omp.h>


MRScene::MRScene(MRSettings& settings) : settings(settings)
{
}

void MRScene::preRenderPointCloud()
{
	animAgeMillis = 0;
	if (animStartMillis > 0) {
		animAgeMillis = GetTickCount() - animStartMillis;
	}
}

int MRScene::renderPointCloud(rs2::points points)
{
	auto vertices = points.get_vertices();              // get vertices
	auto tex_coords = points.get_texture_coordinates(); // and texture coordinates

	glBegin(GL_POINTS);

	//Method 1: Linear for-loop (on the CPU) --> 7.1/8.0 fps on 33% battery
	int totalPointCount = 0;
	for (unsigned int i = 0; i < points.size(); i++)
	{
		if (vertices[i].z > settings.scanMaxZ || vertices[i].z <= settings.scanMinZ)
			continue;
		if ((settings.density > 1) && (((int)(1000 * vertices[i].x) % settings.density) || ((int)(1000 * vertices[i].y) % settings.density)))
			continue;
		totalPointCount += renderPoint(vertices[i], tex_coords[i]);
	}

//	//Method 2: Using OpenMP to try to parallelise the loop --> 8.0/9.1 fps on 33% battery
//#define NUM_THREADS 4
//	const int block = points.size() / NUM_THREADS;
//	std::atomic<int> totalPointCount = 0;
//	#pragma omp parallel num_threads(NUM_THREADS) 
//	//for (int t = 0; t < NUM_THREADS; t++)
//	{
//		int t = omp_get_thread_num();
//		const int start = t;// *block;
//		//const int end = start + block;
//		int pc = 0;
//		for (unsigned int i = start; i < points.size(); i+=NUM_THREADS)
//		{
//			if (vertices[i].z > settings.scanMaxZ || vertices[i].z <= settings.scanMinZ)
//				continue;
//			if ((settings.density > 1) && (((int)(1000 * vertices[i].x) % settings.density) || ((int)(1000 * vertices[i].y) % settings.density)))
//				continue;
//			pc += renderPoint(vertices[i], tex_coords[i]);
//		}
//		totalPointCount += pc;
//	}

	glEnd();
	return totalPointCount;
}

int MRScene::renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord)
{
	// upload the point and texture coordinates only for points we have depth data for
	glVertex3fv(vertex);
	glTexCoord2fv(tex_coord);
	return 1;
}

void MRScene::activate()
{
	state = 0;
	animStartMillis = GetTickCount();
}

bool MRScene::action()
{
	return true;
}


/////////////////////////////////////////////////////////////////
const float MRSceneSetup::SLIDER_WINDOW_WIDTH = 30;
const int MRSceneSetup::SLIDER_PIXELS_TO_BOTTOM = 25;


MRSceneSetup::MRSceneSetup(MRSettings& settings) : MRScene(settings)
{
	memset( nrPointsPerZ, 0, sizeof(nrPointsPerZ) );
}

MRSceneSetup::~MRSceneSetup()
{
}

int MRSceneSetup::renderPointCloud(rs2::points points)
{
	memset(nrPointsPerZ, 0, sizeof(nrPointsPerZ));

	auto vertices = points.get_vertices();              // get vertices
	for (unsigned int i = 0; i < points.size(); i++)
	{
		int z = (int)(vertices[i].z * 100.f);
		if (z > 0 && z < 1000) {
			nrPointsPerZ[z]++;
		}
	}

	int pc = MRScene::renderPointCloud(points);

	glDrawGizmo();
	return pc;
}

void MRSceneSetup::renderImgUI(float window_w, float window_h, rs2::depth_frame depth, rs2::video_frame color)
{
	// Using ImGui library to provide a slide controller to select the depth clipping distance
	uiDrawZHist({ 5.f, 0, window_w, window_h }, settings.scanMaxZ);
	uiDrawSlider({ 0.f, 0, window_w, window_h }, settings.scanMaxZ);

	// It also renders the depth frame, as a picture-in-picture
	// Calculating the position to place the depth frame in the window
	rect pip_stream{ 0, 0, window_w / 5, window_h / 5 };
	pip_stream = pip_stream.adjust_ratio({ static_cast<float>(depth.get_width()),static_cast<float>(depth.get_height()) });
	pip_stream.x = window_w - pip_stream.w - (std::max(window_w, window_h) / 25);
	pip_stream.y = (std::max(window_w, window_h) / 25);
	// Render depth (as picture in pipcture)
	depthImage.upload(depthImageColorizer.process(depth));
	depthImage.show(pip_stream);

	MRScene::renderImgUI(window_w, window_h, depth, color);
}


void MRSceneSetup::glDrawGizmo()
{
	glLineWidth(2.5);

	// x-axis
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glEnd();

	// y-axis
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.1f, 0.0f);
	glEnd();

	// z-axis
	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.1f);
	glEnd();

	// 1x square
	for (float d = 1.0; d <= settings.scanMaxZ; d += 1.0)
	{
		glDrawGizmoScope(d);
	}
}


void MRSceneSetup::glDrawGizmoScope(float distance)
{
	float half = distance / 2.0f;
	glColor3f(half, half, half);
	glBegin(GL_LINE_LOOP);
	glVertex3f(-half, -half, distance);
	glVertex3f(half, -half, distance);
	glVertex3f(half, half, distance);
	glVertex3f(-half, half, distance);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(-half, -half, distance);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(half, -half, distance);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(half, half, distance);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(-half, half, distance);
	glEnd();
}

void MRSceneSetup::uiDrawSlider(rect location, float& clipping_dist)
{
	// Some trickery to display the control nicely
	static const int flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove;

	ImGui::SetNextWindowPos({ location.x, location.y + SLIDER_PIXELS_TO_BOTTOM });
	ImGui::SetNextWindowSize({ SLIDER_WINDOW_WIDTH + 20, location.h - (SLIDER_PIXELS_TO_BOTTOM * 2) });

	ImGui::Begin("slider", nullptr, flags);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
	auto slider_size = ImVec2(SLIDER_WINDOW_WIDTH / 2, location.h - (SLIDER_PIXELS_TO_BOTTOM * 2) - 20);

	//Render the vertical slider
	ImGui::VSliderFloat("", slider_size, &clipping_dist, 0.0f, 6.0f, "", 1.0f, true);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Depth Clipping Distance: %.3f", clipping_dist);
	ImGui::PopStyleColor(3);

	//Display bars next to slider
	float bars_dist = (slider_size.y / 6.0f);
	for (int i = 0; i <= 6; i++)
	{
		ImGui::SetCursorPos({ slider_size.x, i * bars_dist });
		std::string bar_text = "- " + std::to_string(6 - i) + "m";
		ImGui::Text("%s", bar_text.c_str());
	}
	ImGui::End();
}


void MRSceneSetup::uiDrawZHist(rect location, float& clipping_dist)
{
	// Some trickery to display the control nicely
	static const int flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove;

	ImGui::SetNextWindowPos({ location.x, location.y + SLIDER_PIXELS_TO_BOTTOM });
	ImGui::SetNextWindowSize({ SLIDER_WINDOW_WIDTH*5 + 20, location.h - (SLIDER_PIXELS_TO_BOTTOM * 2) });

	ImGui::Begin("z-hist", nullptr, flags);

	//Render the Z-histogram
	for (int i = 1; i < 599; i++)
	{
		ImU32 color = ( ((float)i/100.f)<clipping_dist ) ? IM_COL32(160, 160, 235, 128) : IM_COL32(128, 128, 128, 128);
		float y = location.y + SLIDER_PIXELS_TO_BOTTOM + (location.h - (SLIDER_PIXELS_TO_BOTTOM * 2) - 20)/600.f*(600.f-i);
		int amount = (nrPointsPerZ[i]+ nrPointsPerZ[i-1]+ nrPointsPerZ[i+1])/3;	// Glättung des Wertes (Spitzen- & Rauschunterdrückung)
		ImGui::GetWindowDrawList()->AddLine({ 20.f + SLIDER_WINDOW_WIDTH, y }, { 20.f + +SLIDER_WINDOW_WIDTH + (float)nrPointsPerZ[i]/100.f, y }, color);
	}	
	//ImGui::GetWindowDrawList()->AddLine({ 20.f + +SLIDER_WINDOW_WIDTH, 0.f }, { SLIDER_WINDOW_WIDTH * 5 + 20, location.h - (SLIDER_PIXELS_TO_BOTTOM * 2) }, IM_COL32(160, 160, 235, 128));
	ImGui::End();
}


/////////////////////////////////////////////////////////////////
MRSceneSnapshot::MRSceneSnapshot(MRSettings& settings) : MRScene(settings) {
	snapshot = NULL;
	snaphotIndex = 0;
}

MRSceneSnapshot::~MRSceneSnapshot()
{
	if (snapshot) {
		delete snapshot;
		snapshot = NULL;
	}
}

int MRSceneSnapshot::renderPointCloud(rs2::points points)
{
	if (takeSnapshot)
	{
		if (snapshot) {
			takeSnapshot = false;
			delete snapshot;
			snapshot = NULL;
		}
		else
		{
			snapshot = new TPoint[points.size()];
		}
		snaphotIndex = 0;
	}

	int pc = MRScene::renderPointCloud(points);

	if (takeSnapshot)
	{
		snapshot[pc].v.z = 0;
		takeSnapshot = false;
	}

	// draw points from snapshot
	if (snapshot)
	{
		glBegin(GL_POINTS);
		snaphotIndex = 0;
		while (snapshot[snaphotIndex].v.z)
		{
			float c = (settings.scanMaxZ - snapshot[snaphotIndex].v.z) / settings.scanMaxZ;
			glColor3f(c, c, c);
			glVertex3fv(snapshot[snaphotIndex].v);
			//glTexCoord2fv(snapshot[i].t);
			snaphotIndex++;
		}
		glEnd();
		return pc + snaphotIndex;
	}
	return pc;
}

int MRSceneSnapshot::renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord)
{
	int nr = MRScene::renderPoint(vertex, tex_coord);
	if (takeSnapshot) {
		snapshot[snaphotIndex].v = vertex;
		//unsigned int c = 0;
		//glReadPixels(tex_coords[i].u, tex_coords[i].v, 1, 1, GL_RGB, GL_UNSIGNED_INT, &c);
		//snapshot[pointCount].c = c;
		snaphotIndex++;
	}
	return nr;
}

bool MRSceneSnapshot::action()
{
	takeSnapshot = true;
	PlaySound("camera.wav", GetModuleHandle(NULL), SND_FILENAME | SND_ASYNC);
	return true;
}


/////////////////////////////////////////////////////////////////
MRSceneIBC::MRSceneIBC(MRSettings& settings) : MRScene(settings)
{
	// state: 0..no; 1..water: 2..splash
	iceStartY = 0.500f;		// m
	iceAnimDY = 0.0f;			// m
	iceAnimSpeed = 0.750f;    // m/s
	iceAnimAccel = 0.002f;	// m/s
}

MRSceneIBC::~MRSceneIBC()
{
}

void MRSceneIBC::preRenderPointCloud()
{
	MRScene::preRenderPointCloud();
	iceAnimDY = (float)(animAgeMillis) / 1000.0f * pow(iceAnimSpeed, 1.0f + animAgeMillis / 10000.0f*iceAnimAccel);
}

int MRSceneIBC::renderPointCloud(rs2::points points)
{
	return MRScene::renderPointCloud(points);
}

int MRSceneIBC::renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord)
{
	rs2::vertex realWorldPoint = vertex;
	rs2::vertex icePoint = vertex;
	icePoint.y = iceStartY - iceAnimDY + std::rand() / ((RAND_MAX + 1u) / (10.0f + animAgeMillis / 2.0f));

	if (icePoint.y <= realWorldPoint.y)
		realWorldPoint.y = icePoint.y;
	glVertex3fv(realWorldPoint);
	glTexCoord2fv(tex_coord);

	glVertex3fv(icePoint);
	glTexCoord2fv(tex_coord);
	return 2;
}

bool MRSceneIBC::action()
{
	state++;
	switch (state) {
	case 1:  // water
		animStartMillis = 0;
		iceAnimDY = 0.0f;
		PlaySound("water.wav", GetModuleHandle(NULL), SND_FILENAME | SND_ASYNC | SND_LOOP);
		break;
	case 2:  // splash
		animStartMillis = GetTickCount();
		iceAnimDY = 0.0f;
		PlaySound("splash.wav", GetModuleHandle(NULL), SND_FILENAME | SND_ASYNC);
		break;
	case 0:  // no animation / reset
	default:
		state = 0;
		return false;
	}
	return true;
}

void MRSceneIBC::incWaterYPosition()
{
	iceStartY += 0.010f;
	std::cout << "iceStartY=" << iceStartY << " //incremented" << std::endl;
}

void MRSceneIBC::decWaterYPosition()
{
	iceStartY -= 0.010f;
	std::cout << "iceStartY=" << iceStartY << " //decremented" << std::endl;
}


/////////////////////////////////////////////////////////////////
MRSceneTron::MRSceneTron(MRSettings& settings) : MRScene(settings)
{
	laserPointIndex = 0;
	currentPointIndex = 0;
	lastPointCount = 0;
}

MRSceneTron::~MRSceneTron()
{
}

void MRSceneTron::preRenderPointCloud()
{
	MRScene::preRenderPointCloud();
	laserPointIndex = lastPointCount * animAgeMillis / 100 / 100;	// total animation lasts 10s (ms / 1000 * 10)
																		// -->0..100% of pointCount
}

int MRSceneTron::renderPointCloud(rs2::points points)
{
	auto vertices = points.get_vertices();              // get vertices
	auto tex_coords = points.get_texture_coordinates(); // and texture coordinates

	currentPointIndex = 0;
	glBegin(GL_POINTS);
	// ATTENTION: due to usage of lastPointCount and laserPointIndex the following for-loop must be executed in linear mode
	// and can't paralellized by OpenMP
	for (unsigned int i = 0; i < points.size(); i++)
	{
		if (vertices[i].z > settings.scanMaxZ || vertices[i].z <= settings.scanMinZ)
			continue;
		if ((settings.density > 1) && (((int)(1000 * vertices[i].x) % settings.density) || ((int)(1000 * vertices[i].y) % settings.density)))
			continue;
		currentPointIndex += renderPoint(vertices[i], tex_coords[i]);
	}
	glEnd();
	lastPointCount = currentPointIndex;

	if (state == 2 && animAgeMillis > 10000)
		state = 0;	// reset to the beginning

	if (state != 0 && tronLaserPoint.z != 0.0f && animAgeMillis < 10000) {
		// draw the laser-beam
		glLineWidth(5);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_LINES);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(tronLaserPoint.x, tronLaserPoint.y, tronLaserPoint.z);
		glEnd();
	}
	return lastPointCount;
}

int MRSceneTron::renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord)
{
	if (animAgeMillis > 0) {
		if (state == 1 /* disappear */)
		{
			if (currentPointIndex > laserPointIndex) {
				glVertex3fv(vertex);
				glTexCoord2fv(tex_coord);
			}
			else if (currentPointIndex == laserPointIndex) {
				tronLaserPoint = vertex;
			}
		}
		else if (state == 2 /* appear */)
		{
			if (currentPointIndex < laserPointIndex) {
				glVertex3fv(vertex);
				glTexCoord2fv(tex_coord);
			}
			else if (currentPointIndex == laserPointIndex) {
				tronLaserPoint = vertex;
			}
		}
		else
		{
			MRScene::renderPoint(vertex, tex_coord);
		}
	}
	return 1;
}

bool MRSceneTron::action()
{
	state++;
	switch (state) {
	case 1:  // laser animation: disappearing
		animStartMillis = GetTickCount();
		PlaySound("laser.wav", GetModuleHandle(NULL), SND_FILENAME | SND_ASYNC);
		break;
	case 2:  // laser animation: appearing
		animStartMillis = GetTickCount();
		PlaySound("laser.wav", GetModuleHandle(NULL), SND_FILENAME | SND_ASYNC);
		break;
	case 0:  // no animation, reset
	default:
		state = 0;
		tronLaserPoint.x = 0.0f;
		tronLaserPoint.y = 0.0f;
		tronLaserPoint.z = 0.0f;
		return false;
	}
	return true;
}


/////////////////////////////////////////////////////////////////
MRSceneStartrek::MRSceneStartrek(MRSettings& settings) : MRScene(settings)
{
	limit = 0;
}

MRSceneStartrek::~MRSceneStartrek()
{
}

void MRSceneStartrek::preRenderPointCloud()
{
	MRScene::preRenderPointCloud();
	float anim = (animAgeMillis < 5000) ? ((float)animAgeMillis / 5000.f) : 1.f;	// total animation lasts 5s (ms / 1000 * 5)
	float rel = 1.f;
	if (state == 1)
		rel = (float)(1.f - anim) ;
	else if (state == 2)
		rel = (float)(anim);
	limit = (int)((float)RAND_MAX * rel * rel * rel);
}

int MRSceneStartrek::renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord)
{
	if (animAgeMillis > 0) {
		if (((state == 1 /* disappear */ || state == 2 /* appear */) && (std::rand() < limit))
			|| (state == 0)) {
			glVertex3fv(vertex);
			glTexCoord2fv(tex_coord);
		}
	}
	else
	{
		MRScene::renderPoint(vertex, tex_coord);
	}
	return 1;
}

bool MRSceneStartrek::action()
{
	state++;
	switch (state) {
	case 1:  // beam down
		animStartMillis = GetTickCount();
		PlaySound("startrek.wav", GetModuleHandle(NULL), SND_FILENAME | SND_ASYNC);
		break;
	case 2:  // beam up
		animStartMillis = GetTickCount();
		PlaySound("startrek.wav", GetModuleHandle(NULL), SND_FILENAME | SND_ASYNC);
		break;
	case 0:  // no animation, reset
	default:
		state = 0;
		return false;
	}
	return true;
}