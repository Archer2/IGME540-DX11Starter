#pragma once

#include <string>

// Helpers for determining the actual path to the executable
std::wstring GetExePath();
std::wstring FixPath(const std::wstring& relativeFilePath);
std::string WideToNarrow(const std::wstring& str);
std::wstring NarrowToWide(const std::string& str);

// Misc. Helpers

// ----------------------------------------------------
//  Helper function to generate a random float value
//	between 0 and 1. Inflined if possible
// ----------------------------------------------------
inline float GenerateRandomFloat() { return (float)std::rand() / (float)RAND_MAX; };

// ----------------------------------------------------
//  Helper function to generate a random float value
//	between a given min and max. Inlined if possible
// ----------------------------------------------------
inline float GenerateRandomFloat(float min, float max) { return GenerateRandomFloat() * (max - min) + min; };