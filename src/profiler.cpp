#include "profiler.h"

#include <assert.h>

#include <iostream>

void Profiler::start_measure(std::string name)
{
	save_time(name);
}
float Profiler::end_measure(std::string name, bool print)
{
	float time = measure(name);
	m_measures.erase(name);
	if (print)
		print_last_measure(name);
	return time;
}
float Profiler::passing_measure(std::string name)
{
	float time{ 0 };
	if (m_measures.contains(name))
		time = measure(name);
	save_time(name);
	return time;
}
void Profiler::print_last_measure(std::string name)
{
	std::cout << name << " measured " << m_lastMeasures[name] << " seconds\n";
}

void Profiler::save_time(std::string name)
{
	m_measures[name] = now();
}
float Profiler::measure(std::string name)
{
	assert(m_measures.contains(name));
	m_lastMeasures[name] = std::chrono::duration_cast<std::chrono::nanoseconds>(now() - m_measures[name]).count() / NANOSECONDS_PER_SECOND;
	return m_lastMeasures[name];
}
std::chrono::time_point<std::chrono::high_resolution_clock> Profiler::now()
{
	return std::chrono::high_resolution_clock::now();
}