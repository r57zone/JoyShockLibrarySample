#include <iostream>
#include "JoyShockLibrary.h"
#include <vector>
#include <windows.h>
#include <math.h>

struct EulerAngles {
	double Yaw;
	double Pitch;
	double Roll;
};

EulerAngles QuaternionToEulerAngle(double qW, double qX, double qY, double qZ)
{
	EulerAngles resAngles;
	// roll (x-axis rotation)
	double sinr = +2.0 * (qW * qX + qY * qZ);
	double cosr = +1.0 - 2.0 * (qX * qX + qY * qY);
	resAngles.Roll = atan2(sinr, cosr);

	// pitch (y-axis rotation)
	double sinp = +2.0 * (qW * qY - qZ * qX);
	if (fabs(sinp) >= 1)
		resAngles.Pitch = copysign(3.14159265358979323846 / 2, sinp); // use 90 degrees if out of range
	else
		resAngles.Pitch = asin(sinp);

	// yaw (z-axis rotation)
	double siny = +2.0 * (qW * qZ + qX * qY);
	double cosy = +1.0 - 2.0 * (qY * qY + qZ * qZ);
	resAngles.Yaw = atan2(siny, cosy);

	return resAngles;
}

double RadToDeg(double Rad)
{
	return Rad / 3.14159265358979323846 * 180.0;
}

int main()
{
	SetConsoleTitle("JoyShock Library Sample");

	// https://github.com/JibbSmart/JoyShockLibrary

	int сontrollersCount = JslConnectDevices();
	std::cout << сontrollersCount << "\n";

	int deviceID[4]; // max playes 12

	JslGetConnectedDeviceHandles(deviceID, сontrollersCount);

	JOY_SHOCK_STATE InputState;
	MOTION_STATE MotionState;
	TOUCH_STATE TouchState;

	while ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) == false)
	{
		InputState = JslGetSimpleState(deviceID[0]);
		MotionState = JslGetMotionState(deviceID[0]);
		
		EulerAngles MotionAngles;
		MotionAngles = QuaternionToEulerAngle(MotionState.quatW, MotionState.quatX, MotionState.quatY, MotionState.quatZ);
		
		if (JslGetControllerType(deviceID[0]) == JS_TYPE_DS) {
			TouchState = JslGetTouchState(deviceID[0]);
			printf("Touch:\r\n");
			if (TouchState.t0Down)
				printf("First = %5.2f\t%5.2f\r\n", TouchState.t0X, TouchState.t0Y);
			if (TouchState.t1Down)
				printf("Second = %5.2f\t%5.2f\r\n", TouchState.t1X, TouchState.t1Y);
		}
		
		printf("\r\nMotion:\r\n");
		printf("Euler: %5.2f\t%5.2f\t%5.2f\r\n", RadToDeg(MotionAngles.Yaw), RadToDeg(MotionAngles.Pitch), RadToDeg(MotionAngles.Roll));
		printf("Quaternion: %5.2f\t%5.2f\t%5.2f\t%5.2f\r\n", MotionState.quatW, MotionState.quatX, MotionState.quatY, MotionState.quatZ);

		if (InputState.buttons & JSMASK_ZR) {
			//JslSetLightColour(deviceID[0], 255);
			//JslSetPlayerNumber(deviceID[0], 1);
			JslSetRumble(deviceID[0], 255, 255);
		}
		
		printf("\r\nButtons:\r\n");
		if (InputState.buttons & JSMASK_S) std::cout << "X pressed\n";

		Sleep(1);
		system("cls");
	}

	JslDisconnectAndDisposeAll();
}

