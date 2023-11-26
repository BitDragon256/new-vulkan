#include "profiler.h"

#include <assert.h>

#include <iostream>

void Profiler::start_measure(std::string name)
{
	save_time(name);
}
float Profiler::end_measure(std::string name, bool print)
{
    //auto renderStart = std::chrono::high_resolution_clock::now();
	float time = measure(name);
	m_measures.erase(name);
	if (print)
		print_last_measure(name);
    //auto renderEnd = std::chrono::high_resolution_clock::now();
    //out_buf() << "measure time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(renderEnd - renderStart).count() / NANOSECONDS_PER_SECOND;
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
	out_buf() << m_lastMeasures[name] << " ms | " << name << "\n";
}

std::vector<std::string> Profiler::s_labels;
void Profiler::begin_label(std::string name)
{
	out_buf() << "--------------------" << name << "--------------------\n";
	s_labels.push_back(name);
	start_measure(name);
}
float Profiler::end_label()
{
	if (!s_labels.empty())
	{
		std::string name = s_labels.back();
		out_buf() << "---------------------------------------------------------\n";
		float time = end_measure(name, true);
		out_buf() << "---------------------------------------------------------\n";
		s_labels.pop_back();
		return time;
	}
	return 0.f;
}

void Profiler::save_time(std::string name)
{
	m_measures[name] = now();
}
float Profiler::measure(std::string name)
{
	assert(m_measures.contains(name));
	m_lastMeasures[name] = std::chrono::duration_cast<std::chrono::nanoseconds>(now() - m_measures[name]).count() / NANOSECONDS_PER_SECOND * 1000.f;
	return m_lastMeasures[name];
}
std::chrono::time_point<std::chrono::high_resolution_clock> Profiler::now()
{
	return std::chrono::high_resolution_clock::now();
}
std::stringstream Profiler::s_outS;
size_t Profiler::s_outBufWrites = 0;
std::stringstream& Profiler::out_buf()
{
	return s_outS;
	if (s_outBufWrites++ >= PROFILER_OUT_BUFFER_WRITES)
	{
		std::ios::sync_with_stdio(false);
		std::cout << s_outS.str();
		s_outS.clear();
		s_outBufWrites = 0;
	}
	std::string indent = "";
	for (int i = 0; i < s_labels.size(); i++)
		indent += "-";
	if (!s_labels.empty())
		indent += " ";
	s_outS << indent;
	return s_outS;
}
void Profiler::print_buf()
{
	s_outBufWrites = PROFILER_OUT_BUFFER_WRITES;
}