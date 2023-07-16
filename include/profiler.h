#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#define NANOSECONDS_PER_SECOND 1000000000.f

class Profiler
{
public:

	void start_measure(std::string name);
	float end_measure(std::string name);

	float passing_measure(std::string name);
	void print_last_measure(std::string name);

private:

	std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> m_measures;
	std::unordered_map<std::string, float> m_lastMeasures;

	void save_time(std::string name);
	std::chrono::time_point<std::chrono::high_resolution_clock> now();
	float measure(std::string name);

};