// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "GlTypes.h"
#include "GlWindow.h"

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

enum EMRSceneType
{
	SETUP = -1,
	NONE = 0,
	SNAP = 1,
	IBC = 2,
	TRON = 3,
	STARTREK = 4
};

class MRSettings
{
public:
	int density;
	bool colored;
	float scanMinZ;		// m
	float scanMaxZ;		// m
	bool auto_rotation;

	MRSettings() {
		reset();
	}

	void reset() {
		density = 2;
		colored = true;
		scanMinZ = 0.0f;		// m
		scanMaxZ = 1.0f;		// m default=1.73f
		auto_rotation = true;
	}
};

class MRScene
{
protected:
	MRSettings& settings;

	int state = 0;
	long animStartMillis = 0;
	long animAgeMillis = 0;

public:
	MRScene(MRSettings& settings);
	virtual ~MRScene() {};

	virtual EMRSceneType type() { return EMRSceneType::NONE; }

	// rendering:
	virtual void preRenderPointCloud();
	virtual int renderPointCloud(rs2::points points);
	virtual int renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord);
	virtual void renderImgUI(float window_w, float window_h, rs2::depth_frame depth, rs2::video_frame color) {}

	// interaction:
	virtual void activate();
	virtual bool action();	// returns false if scene ended (return to default-scene)
};


/////////////////////////////////////////////////////////////////
typedef struct {
	rs2::vertex v;	// vertex x/y/z-coordinates
	int c;		// color
} TPoint;

class MRSceneSnapshot : public MRScene
{
private:
	bool takeSnapshot = false;
	TPoint* snapshot = NULL;
	unsigned int snaphotIndex = 0;

public:
	MRSceneSnapshot(MRSettings& settings);
	virtual ~MRSceneSnapshot();

	virtual EMRSceneType type() { return EMRSceneType::SNAP; }

	virtual int renderPointCloud(rs2::points points);
	virtual int renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord);

	virtual bool action();	// returns false if scene ended (return to default-scene)
};


/////////////////////////////////////////////////////////////////
class MRSceneSetup : public MRScene
{
private:
	GlTexture depthImage;                   // Helper for renderig images
	rs2::colorizer depthImageColorizer;     // Helper to colorize depth images
	int nrPointsPerZ[1000];				    // counts points per Z-coordinate (centimeter)

public:
	MRSceneSetup(MRSettings& settings);
	virtual ~MRSceneSetup();

	virtual EMRSceneType type() { return EMRSceneType::SETUP; }
	static const float SLIDER_WINDOW_WIDTH;
	static const int SLIDER_PIXELS_TO_BOTTOM;

	virtual int renderPointCloud(rs2::points points);
	virtual void renderImgUI(float window_w, float window_h, rs2::depth_frame depth, rs2::video_frame color);

private:
	// Helper functions
	void glDrawGizmo();
	void glDrawGizmoScope(float distance);

	// ImgUI:
	void uiDrawSlider(rect location, float& clipping_dist);
	void uiDrawZHist(rect location, float& clipping_dist);
};


// ice-bucket-challenge 
class MRSceneIBC : public MRScene
{
	// state: 0..no; 1..water: 2..splash
private:
	float iceStartY = 0.500f;		// m
	float iceAnimDY = 0.0f;			// m
	float iceAnimSpeed = 0.750f;    // m/s
	float iceAnimAccel = 0.002f;	// m/s

public:
	MRSceneIBC(MRSettings& settings);
	virtual ~MRSceneIBC();

	virtual EMRSceneType type() { return EMRSceneType::IBC; }

	virtual void preRenderPointCloud();
	virtual int renderPointCloud(rs2::points points);
	virtual int renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord);

	virtual bool action();	// returns false if scene ended (return to default-scene)

	void incWaterYPosition();
	void decWaterYPosition();
};

// tron laser
class MRSceneTron : public MRScene
{
private:
	rs2::vertex tronLaserPoint;
	unsigned int currentPointIndex;
	unsigned int laserPointIndex;
	int lastPointCount;

public:
	MRSceneTron(MRSettings& settings);
	virtual ~MRSceneTron();

	virtual EMRSceneType type() { return EMRSceneType::TRON; }

	virtual void preRenderPointCloud();
	virtual int renderPointCloud(rs2::points points);
	virtual int renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord);

	virtual bool action();	// returns false if scene ended (return to default-scene)
};

class MRSceneStartrek : public MRScene
{
private:
	int limit;

public:
	MRSceneStartrek(MRSettings& settings);
	virtual ~MRSceneStartrek();

	virtual EMRSceneType type() { return EMRSceneType::STARTREK; }

	virtual void preRenderPointCloud();
	virtual int renderPoint(const rs2::vertex& vertex, const rs2::texture_coordinate& tex_coord);

	virtual bool action();	// returns false if scene ended (return to default-scene)
};
