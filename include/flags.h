#pragma once

#include <string>
#include <unordered_map>

#include <ini.h>

#include "logger.h"

#define FLAG_INI_HEADER_BOOLS "flags_bool"
#define FLAG_INI_HEADER_INTS "flags_int"
#define FLAG_INI_HEADER_FLOATS "flags_float"

#define FLAG_DEFAULT_FILE "flags.ini"

class FlagHandler
{
public:
	template<typename T> void set(std::string name, T value)
	{
		const char* typeName = typeid(T).name();

		if (typeName == "bool")
			m_bools[name] = value;
		else if (typeName == "int")
			m_ints[name] = value;
		else if (typeName == "float")
			m_floats[name] = value;
		else
			logger::log("FlagHandler: no valid type");
	}
	template<typename T> T get(std::string name)
	{
		const char* typeName = typeid(T).name();

		if (typeName == "bool")
			return m_bools[name];
		if (typeName == "int")
			return m_ints[name];
		if (typeName == "float")
			return m_floats[name];
		
		logger::log("FlagHandler: no valid type");
		return T();
	}
	void save(std::string filename = FLAG_DEFAULT_FILE);
	void load(std::string filename = FLAG_DEFAULT_FILE);
private:
	std::unordered_map<std::string, bool> m_bools;
	std::unordered_map<std::string, int> m_ints;
	std::unordered_map<std::string, float> m_floats;

	bool to_bool(std::string s);
	int to_int(std::string s);
	float to_float(std::string s);

	std::string to_string(bool b);
	std::string to_string(int i);
	std::string to_string(float f);
};