/*
* This file is part of Touchy (https://github.com/uhhhci/Touchy).
* Copyright (c) 2015 Nicholas Katzakis
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#define dllexport __declspec(dllexport)

#include "HD/hd.h" //part of 3DS OpenHaptics
#include <HDU/hduVector.h>
#include <HDU/hduError.h>
#include "vector_math.h"
using namespace vmath;

//Global variable and handle declarations
HDSchedulerHandle hCallback;
//HDSchedulerHandle hForceCallback;
HHD hHD;

//Sphere globals (only one sphere for now)
double sphereRadius;
vec3<double> spherePosition;

/* Holds data retrieved from HDAPI. */
typedef struct
{
	HDboolean m_buttonState;       /* Has the device button has been pressed. */
	HDboolean m_buttonState2;       /* Has the device button2 has been pressed. */
	hduVector3Dd m_devicePosition; /* Current device coordinates. */
	HDErrorInfo m_error;
} DeviceData;

static DeviceData gServoDeviceData;

HDCallbackCode HDCALLBACK CallbackIdle(void *data)
{
	int nButtons = 0;
	// Get the position of the device.
	/*vec3<double> cursorPosition;*/

	hdBeginFrame(hHD);

	/* Retrieve the current button(s). */
	hdGetIntegerv(HD_CURRENT_BUTTONS, &nButtons);

	/* In order to get the specific button 1 state, we use a bitmask to
	test for the HD_DEVICE_BUTTON_1 bit. */
	gServoDeviceData.m_buttonState =
		(nButtons & HD_DEVICE_BUTTON_1) ? HD_TRUE : HD_FALSE;

	gServoDeviceData.m_buttonState2 =
		(nButtons & HD_DEVICE_BUTTON_2) ? HD_TRUE : HD_FALSE;
	/* Get the current location of the device (HD_GET_CURRENT_POSITION)
	We declare a vector of three doubles since hdGetDoublev returns
	the information in a vector of size 3. */
	hdGetDoublev(HD_CURRENT_POSITION, gServoDeviceData.m_devicePosition);

	/* Also check the error state of HDAPI. */
	gServoDeviceData.m_error = hdGetError();

	hdEndFrame(hHD);

	return HD_CALLBACK_CONTINUE;
}

/*******************************************************************************
Generates a force to drive the cursor to the center of the given coordinates.
*******************************************************************************/
HDCallbackCode HDCALLBACK CallbackToSphereCenter(void *data)
{


	// Stiffness, i.e. k value, of the sphere.  Higher stiffness results
	// in a harder surface.
	const double sphereStiffness = 1.f;

	hdBeginFrame(hHD);

	int nButtons = 0;

	/* Retrieve the current button(s). */
	hdGetIntegerv(HD_CURRENT_BUTTONS, &nButtons);

	/* In order to get the specific button 1 state, we use a bitmask to
	test for the HD_DEVICE_BUTTON_1 bit. */
	gServoDeviceData.m_buttonState =
		(nButtons & HD_DEVICE_BUTTON_1) ? HD_TRUE : HD_FALSE;

	gServoDeviceData.m_buttonState2 =
		(nButtons & HD_DEVICE_BUTTON_2) ? HD_TRUE : HD_FALSE;
	/* Get the current location of the device (HD_GET_CURRENT_POSITION)
	We declare a vector of three doubles since hdGetDoublev returns
	the information in a vector of size 3. */
	hdGetDoublev(HD_CURRENT_POSITION, gServoDeviceData.m_devicePosition);


	// Get the position of the device.
	vec3<double> cursorPosition;
	hdGetDoublev(HD_CURRENT_POSITION, cursorPosition);

	// Find the distance between the device and the center of the
	// sphere.
	vec3<double> newvec = (cursorPosition - spherePosition);
	double distance = length(newvec);

	// If the user is within the sphere -- i.e. if the distance from the user to
	// the center of the sphere is less than the sphere radius -- then the user
	// is penetrating the sphere and a force should be commanded to repel him
	// towards the surface.
	if (distance < sphereRadius)
	{
		// Calculate the penetration distance.
		double penetrationDistance = sphereRadius - distance;

		// Create a unit vector in the direction of the force, this will always
		// be outward from the center of the sphere through the user's
		// position.
		vec3<double> forceDirection = (spherePosition - cursorPosition) / distance;

		// Use F=kx to create a force vector that is towards the center of
		// the sphere and proportional to the penetration distance, and scaled
		// by the object stiffness.
		// Hooke's law explicitly:
		double k = sphereStiffness;
		vec3<double> x = penetrationDistance * forceDirection;
		vec3<double> f = k * x;
		hdSetDoublev(HD_CURRENT_FORCE, forceDirection);
	}


	hdEndFrame(hHD);

	HDErrorInfo error;
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		if (error.errorCode == HD_SCHEDULER_FULL)
		{
			return HD_CALLBACK_DONE;
		}
	}

	return HD_CALLBACK_CONTINUE;
}


/*******************************************************************************
Generates an opposing force if the device attempts to penetrate the sphere.
*******************************************************************************/
HDCallbackCode HDCALLBACK FrictionlessSphereCallback(void *data)
{
	// Stiffness, i.e. k value, of the sphere.  Higher stiffness results
	// in a harder surface.
	const double sphereStiffness = 1.0f;

	hdBeginFrame(hHD);

	int nButtons = 0;

	/* Retrieve the current button(s). */
	hdGetIntegerv(HD_CURRENT_BUTTONS, &nButtons);

	/* In order to get the specific button 1 state, we use a bitmask to
	test for the HD_DEVICE_BUTTON_1 bit. */
	gServoDeviceData.m_buttonState =
		(nButtons & HD_DEVICE_BUTTON_1) ? HD_TRUE : HD_FALSE;

	gServoDeviceData.m_buttonState2 =
		(nButtons & HD_DEVICE_BUTTON_2) ? HD_TRUE : HD_FALSE;
	/* Get the current location of the device (HD_GET_CURRENT_POSITION)
	We declare a vector of three doubles since hdGetDoublev returns
	the information in a vector of size 3. */
	hdGetDoublev(HD_CURRENT_POSITION, gServoDeviceData.m_devicePosition);


	// Get the position of the device.
	vec3<double> cursorPosition;
	hdGetDoublev(HD_CURRENT_POSITION, cursorPosition);

	// Find the distance between the device and the center of the
	// sphere.
	vec3<double> newvec = (cursorPosition - spherePosition);
	double distance = length(newvec);

	// If the user is within the sphere -- i.e. if the distance from the user to
	// the center of the sphere is less than the sphere radius -- then the user
	// is penetrating the sphere and a force should be commanded to repel him
	// towards the surface.
	if (distance < sphereRadius)
	{
		// Calculate the penetration distance.
		double penetrationDistance = sphereRadius - distance;

		// Create a unit vector in the direction of the force, this will always
		// be outward from the center of the sphere through the user's
		// position.
		vec3<double> forceDirection = (cursorPosition - spherePosition) / distance;
		// Use F=kx to create a force vector that is away from the center of
		// the sphere and proportional to the penetration distance, and scaled
		// by the object stiffness.
		// Hooke's law explicitly:
		double k = sphereStiffness;
		vec3<double> x = penetrationDistance * forceDirection;
		vec3<double> f = k * x;
		hdSetDoublev(HD_CURRENT_FORCE, f);
	}


	hdEndFrame(hHD);


	HDErrorInfo error;
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		if (error.errorCode == HD_SCHEDULER_FULL)
		{
			return HD_CALLBACK_DONE;
		}
	}

	return HD_CALLBACK_CONTINUE;
}

//For Force by jaccen
HDCallbackCode HDCALLBACK ForceCallback(void *data)
{
	// Stiffness, i.e. k value, of the sphere.  Higher stiffness results
	// in a harder surface.
	const double sphereStiffness = 0.025f;

	hdBeginFrame(hHD);

	int nButtons = 0;

	/* Retrieve the current button(s). */
	hdGetIntegerv(HD_CURRENT_BUTTONS, &nButtons);

	/* In order to get the specific button 1 state, we use a bitmask to
	test for the HD_DEVICE_BUTTON_1 bit. */
	gServoDeviceData.m_buttonState =
		(nButtons & HD_DEVICE_BUTTON_1) ? HD_TRUE : HD_FALSE;

	gServoDeviceData.m_buttonState2 =
		(nButtons & HD_DEVICE_BUTTON_2) ? HD_TRUE : HD_FALSE;
	/* Get the current location of the device (HD_GET_CURRENT_POSITION)
	We declare a vector of three doubles since hdGetDoublev returns
	the information in a vector of size 3. */
	hdGetDoublev(HD_CURRENT_POSITION, gServoDeviceData.m_devicePosition);


	// Get the position of the device.
	vec3<double> cursorPosition;
	hdGetDoublev(HD_CURRENT_POSITION, cursorPosition);

	// Find the distance between the device and the center of the
	// sphere.
	vec3<double> newvec = (cursorPosition - spherePosition);
	newvec.x = 0.0;
	newvec.y = 0.0;
	double distance = length(newvec);

	// If the user is within the sphere -- i.e. if the distance from the user to
	// the center of the sphere is less than the sphere radius -- then the user
	// is penetrating the sphere and a force should be commanded to repel him
	// towards the surface.
	if (distance < sphereRadius)
	{
		// Calculate the penetration distance.
		double penetrationDistance = sphereRadius - distance;

		// Create a unit vector in the direction of the force, this will always
		// be outward from the center of the sphere through the user's
		// position.
		vec3<double> forceDirection = (cursorPosition - spherePosition) / distance;
		forceDirection.x = 0.0;
		forceDirection.y = 0.0;
		if (forceDirection.z < 0)
		{
			forceDirection.z = 0;
		}
		
		// Use F=kx to create a force vector that is away from the center of
		// the sphere and proportional to the penetration distance, and scaled
		// by the object stiffness.
		// Hooke's law explicitly:
		double k = sphereStiffness;
		vec3<double> x = penetrationDistance * forceDirection;
		vec3<double> f = k * x;
		hdSetDoublev(HD_CURRENT_FORCE, f);
	}
	hdEndFrame(hHD);

	HDErrorInfo error;
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		if (error.errorCode == HD_SCHEDULER_FULL)
		{
			return HD_CALLBACK_DONE;
		}
	}

	return HD_CALLBACK_CONTINUE;
}

extern "C" dllexport int init() {

	HDErrorInfo error;
	// Initialize the default haptic device.
	hHD = hdInitDevice(HD_DEFAULT_DEVICE);
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		return(error.errorCode);
	}

	// Start the servo scheduler and enable forces.
	hdEnable(HD_FORCE_OUTPUT);
	hdStartScheduler();
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		return(error.errorCode);
	}

	return(HD_SUCCESS);
}

extern "C" dllexport int startIdleCallback() {
	HDErrorInfo error;
	hCallback = hdScheduleAsynchronous(CallbackIdle, 0, HD_DEFAULT_SCHEDULER_PRIORITY);
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		return(error.errorCode);
	}

	return HD_SUCCESS;
}


extern "C" dllexport int startCenterCallback(double radius, double x, double y, double z) {
	sphereRadius = radius;
	spherePosition.x = x;
	spherePosition.y = y;
	spherePosition.z = z;

	// Application loop - schedule our call to the main callback.
	hCallback = hdScheduleAsynchronous(CallbackToSphereCenter, 0, HD_MAX_SCHEDULER_PRIORITY);

	//Return success/error code
	HDErrorInfo error;
	error = hdGetError();
	return(error.errorCode);
}

extern "C" dllexport int startSphereCallback(double radius, double x, double y, double z) {
	sphereRadius = radius;
	spherePosition.x = x;
	spherePosition.y = y;
	spherePosition.z = z;

	// Application loop - schedule our call to the main callback.
	hCallback = hdScheduleAsynchronous(FrictionlessSphereCallback, 0, HD_MAX_SCHEDULER_PRIORITY);

	//Return success/error code
	HDErrorInfo error;
	error = hdGetError();
	return(error.errorCode);
}

extern "C" dllexport int startForceCallback(double radius, double z)
{
	sphereRadius = radius;
	spherePosition.x = 0;
	spherePosition.y = 0;
	spherePosition.z = z;

	// Application loop - schedule our call to the main callback.
	hCallback = hdScheduleAsynchronous(ForceCallback, 0, HD_MAX_SCHEDULER_PRIORITY);

	//Return success/error code
	HDErrorInfo error;
	error = hdGetError();
	return(error.errorCode);
}

extern "C" dllexport int getLastError() {
	HDErrorInfo error;
	error = hdGetError();
	return(error.errorCode);
}

extern "C" dllexport int stopCallback() {
	HDErrorInfo error;

	// For cleanup, unschedule our callbacks and stop the servo loop.
	hdWaitForCompletion(hCallback, HD_WAIT_CHECK_STATUS);

	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		return(error.errorCode);
	}

	//unschedule the callback
	hdUnschedule(hCallback);

	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		return(error.errorCode);
	}

	return HD_SUCCESS;
}

extern "C" dllexport void setSphereRadius(double radius)
{
	sphereRadius = radius;
}

extern "C" dllexport double getSphereRadius()
{
	return sphereRadius;
}

extern "C" dllexport void setSpherePosition(double x, double y, double z)
{
	spherePosition.x = x;
	spherePosition.y = y;
	spherePosition.z = z;
}

extern "C" dllexport void getEEPosition(double positions[3])
{
	positions[0] = gServoDeviceData.m_devicePosition[0];
	positions[1] = gServoDeviceData.m_devicePosition[1];
	positions[2] = gServoDeviceData.m_devicePosition[2];
}

extern "C" dllexport bool getButtonState()
{
	return gServoDeviceData.m_buttonState;
}

extern "C" dllexport bool getButtonState2()
{
	return gServoDeviceData.m_buttonState2;
}

extern "C" dllexport int shutdown() {
	HDErrorInfo error;

	// For cleanup, unschedule our callbacks and stop the servo loop.
	/*hdWaitForCompletion(hCallback, HD_WAIT_CHECK_STATUS);
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
	return(error.errorCode+1000);
	}*/
	hdStopScheduler();
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		return(error.errorCode + 2000);
	}
	/*hdUnschedule(hCallback);
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
	return(error.errorCode+3000);
	}*/
	hdDisableDevice(hHD);
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		return(error.errorCode + 4000);
	}

	return HD_SUCCESS;
}
