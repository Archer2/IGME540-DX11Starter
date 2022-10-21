#pragma once

#include "Types.h"

// Define integers for different Light Types
//#define LIGHT_TYPE_DIRECTIONAL	0
//#define LIGHT_TYPE_POINT		1
//#define LIGHT_TYPE_SPOT			2

// --------------------------------------------------------
// Enum to dictate different types of Lights, rather than
// basic #define macros
//	- Cast to an int to pass to D3D - still is 4 bytes
// --------------------------------------------------------
enum LightType {
	Directional = 0,
	Point = 1,
	Spot = 2
};


// Basic Light struct defining all basic Light type variables. Padded for use in cbuffers
struct BasicLight {
	LightType Type;		// Directional, Point, or Spot
	Vector3 Position;	// World Position - Point and Spot
	float Range;		// Attenuation Distance - Point and Spot
	Vector3 Direction;	// Normalized Direction - Directional and Spot
	float Intensity;	// Intensity Scalar - All Types
	Vector3 Color;		// Light Color - All Types
	float SpotAngle;	// Cone Angle - Spot
	Vector3 _padding;	// Line this struct up to a 16-byte multiple size
};