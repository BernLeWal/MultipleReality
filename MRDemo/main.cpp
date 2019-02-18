// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include "GlWindow.h"

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include <string>
#include <iostream>
#include <ctime>

#include "MRDemo.h"


int main(int argc, char * argv[]) try
{
	std::srand((unsigned int)std::time(nullptr)); // use current time as seed for random generator

	// Create a simple OpenGL window for rendering:
	// Construct an object to manage view state
	MRDemo app;
	while (app) // Application still alive?
	{
		if (!app.run())
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
	std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	return EXIT_FAILURE;
}
catch (const std::exception & e)
{
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}

