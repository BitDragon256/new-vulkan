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
	std::string indent = "";
	for (int i = 0; i < m_labels.size(); i++)
		indent += "-";
	if (!m_labels.empty())
		indent += " ";
	out_buf() << indent << name << " measured " << m_lastMeasures[name] << " seconds\n";
}
void Profiler::begin_label(std::string name)
{
	out_buf() << "----------" << name << "----------\n";
	m_labels.push_back(name);
	start_measure(name);
}
float Profiler::end_label()
{
	if (!m_labels.empty())
	{
		std::string name = m_labels.back();
		m_outS << "---";
		float time = end_measure(name, true);
		m_labels.pop_back();
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
	m_lastMeasures[name] = std::chrono::duration_cast<std::chrono::nanoseconds>(now() - m_measures[name]).count() / NANOSECONDS_PER_SECOND;
	return m_lastMeasures[name];
}
std::chrono::time_point<std::chrono::high_resolution_clock> Profiler::now()
{
	return std::chrono::high_resolution_clock::now();
}
std::stringstream& Profiler::out_buf()
{
	if (m_outBufWrites++ >= PROFILER_OUT_BUFFER_WRITES)
	{
		std::ios::sync_with_stdio(false);
		std::cout << m_outS.str();
		m_outS.clear();
		m_outBufWrites = 0;
	}
	return m_outS;
}