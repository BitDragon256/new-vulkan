#pragma once

#include <chrono>
#include <string>
#include <sstream>
#include <unordered_map>

#define NANOSECONDS_PER_SECOND 1000000000.f
#define PROFILER_OUT_BUFFER_WRITES 50000 

class Profiler;

class Profiler
{
public:

	void start_measure(std::string name);
	float end_measure(std::string name, bool print = false);

	float passing_measure(std::string name);
	void print_last_measure(std::string name);
	void begin_label(std::string name);
	float end_label();

	std::stringstream& out_buf();
	static void print_buf();

private:

	std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> m_measures;
	std::unordered_map<std::string, float> m_lastMeasures;
	static std::vector<std::string> s_labels;

	void save_time(std::string name);
	std::chrono::time_point<std::chrono::high_resolution_clock> now();
	float measure(std::string name);

	static std::stringstream s_outS;
	static size_t s_outBufWrites;

};