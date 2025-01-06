#include <iostream>
#include "JoyShockLibrary.h"
#include <windows.h>

struct EulerAngles {
	double Yaw;
	double Pitch;
	double Roll;
};

EulerAngles QuaternionToEulerAngle(double qW, double qX, double qY, double qZ) // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
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

double OffsetYPR2(double Angle1, double Angle2) // CalcMotionStick
{
	Angle1 -= Angle2;
	if (Angle1 < -3.14159265358979323846)
		Angle1 += 2 * 3.14159265358979323846;
	else if (Angle1 > 3.14159265358979323846)
		Angle1 -= 2 * 3.14159265358979323846;
	return Angle1;
}

float CalcMotionStick(float gravA, float gravB, float wheelAngle, float offsetAxis) {
	float angleRadians = wheelAngle * (3.14159f / 180.0f); // To radians

	float normalizedValue = OffsetYPR2(atan2f(gravA, gravB), offsetAxis) / angleRadians;

	if (normalizedValue > 1.0f)
		normalizedValue = 1.0f;
	else if (normalizedValue < -1.0f)
		normalizedValue = -1.0f;

	return normalizedValue;
}

float NormalizeHorizontalAngleToStick(float angle, float maxAngle)
{
	// Преобразуем угол в диапазон [-1, 1]
	float normalizedValue = angle / maxAngle;

	// Ограничиваем значение в пределах [-1, 1]
	if (normalizedValue > 1.0f)
		normalizedValue = 1.0f;
	else if (normalizedValue < -1.0f)
		normalizedValue = -1.0f;

	return normalizedValue;
}


void HandleDeviceConnections()
{
	int deviceHandles[16];
	int numConnectedDevices = JslGetConnectedDeviceHandles(deviceHandles, 16);

	const float maxAngleDegrees = 150.0f; // Wheel max angle

	float OffsetAxisX = 0;
	EulerAngles MotionAngles;

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
			MotionAngles = QuaternionToEulerAngle(motionState.quatW, motionState.quatZ, motionState.quatX, motionState.quatY);

			float WheelAngle = 150.0f;

			
			float stickValue = CalcMotionStick(motionState.gravX, motionState.gravZ, maxAngleDegrees, OffsetAxisX);

			//std::cout << "Device " << deviceIndex  std::endl;

			std::cout << "Wheel roll value: " << stickValue * 100 << "% " << "Yaw = " << RadToDeg(MotionAngles.Yaw) << ", Pitch = " << RadToDeg(MotionAngles.Pitch) << ", Roll = " << RadToDeg(MotionAngles.Roll) << std::endl; // Перестает работать влево мышки ???
		
			//std::cout << "Devices connected: " << deviceIndex << std::endl;
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

