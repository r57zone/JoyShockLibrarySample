// Based on https://gist.github.com/JibbSmart/8cbaba568c1c2e1193771459aa5385df

#include <iostream>
#include "JoyShockLibrary.h"
#include <windows.h>

// Settings
int _gyroSpace = 2;
float _sensitivity = 0.5f;
float _tightening = 2.f;
int _defaultCalibrationMode = 1;

// Callback for applying settings to new connections
void ConnectCallback(int deviceId)
{
	JslSetGyroSpace(deviceId, _gyroSpace);
	if (_defaultCalibrationMode > 0)
	{
		JslSetAutomaticCalibration(deviceId, true);
	}
}

void MoveMouse(float x, float y)
{
	static float accumulatedX = 0.f;
	static float accumulatedY = 0.f;

	accumulatedX += x;
	accumulatedY += y;

	int applicableX = (int)accumulatedX;
	int applicableY = (int)accumulatedY;

	accumulatedX -= applicableX;
	accumulatedY -= applicableY;

	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.mouseData = 0;
	input.mi.time = 0;
	input.mi.dx = applicableX;
	input.mi.dy = applicableY;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &input, sizeof(input));
}

void CalculateYawPitchRoll(float quatW, float quatX, float quatY, float quatZ, float& yaw, float& pitch, float& roll)
{
	yaw = atan2f(2.0f * (quatW * quatZ + quatX * quatY), 1.0f - 2.0f * (quatY * quatY + quatZ * quatZ));
	pitch = asinf(2.0f * (quatW * quatY - quatZ * quatX));
	roll = atan2f(2.0f * (quatW * quatX + quatY * quatZ), 1.0f - 2.0f * (quatX * quatX + quatY * quatY));
}

void HandleDeviceConnections()
{
	int deviceHandles[16];
	int numConnectedDevices = JslGetConnectedDeviceHandles(deviceHandles, 16);

	if (numConnectedDevices > 0)
	{
		for (int deviceIndex = 0; deviceIndex < numConnectedDevices; ++deviceIndex)
		{
			const int deviceHandle = deviceHandles[deviceIndex];
			float velocityX, velocityY, velocityZ;
			JslGetAndFlushAccumulatedGyro(deviceHandle, velocityX, velocityY, velocityZ);

			float frameTime = 1.f / 60.f;

			const float inputSize = sqrtf(velocityX * velocityX + velocityY * velocityY + velocityZ * velocityZ);
			float tightenedSensitivity = _sensitivity * 50.f;
			if (inputSize < _tightening && _tightening > 0)
			{
				tightenedSensitivity *= inputSize / _tightening;
			}
			MoveMouse(-velocityY * tightenedSensitivity * frameTime, -velocityX * tightenedSensitivity * frameTime);

		
			MOTION_STATE motionState = JslGetMotionState(deviceHandle);
			float yaw, pitch, roll;
			CalculateYawPitchRoll(motionState.quatW, motionState.quatX, motionState.quatY, motionState.quatZ, yaw, pitch, roll);

			//std::cout << "Device " << deviceIndex << " connected." << std::endl;
			//std::cout << "Gravity = (" << motionState.gravX << ", " << motionState.gravY << ", " << motionState.gravZ << ")" << std::endl;
			//std::cout << "Acceleration = (" << motionState.accelX << ", " << motionState.accelY << ", " << motionState.accelZ << ")" << std::endl;
			
			//std::cout << "Yaw = " << yaw << ", Pitch = " << pitch << ", Roll = " << roll << std::endl; // Перестает работать влево мышки ???
		
			std::cout << "Devices connected: " << deviceIndex << std::endl;
		}
	}
	else
	{
		std::cout << "Not devices." << std::endl;
	}
}

int main()
{
	SetConsoleTitle("JoyShock Library Sample");
	JslSetConnectCallback(ConnectCallback);
	JslConnectDevices();

	while (true)
	{
		HandleDeviceConnections();

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
			break;

		Sleep(1);
	}

	return 0;
}

