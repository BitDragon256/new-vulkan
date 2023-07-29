#include "flags.h"

#include <sstream>

void FlagHandler::save(std::string filename)
{
	mINI::INIFile file(filename);
	mINI::INIStructure ini;

	for (const auto& [key, value] : m_bools)
		ini[FLAG_INI_HEADER_BOOLS][key] = to_string(value);
	for (const auto& [key, value] : m_ints)
		ini[FLAG_INI_HEADER_INTS][key] = to_string(value);
	for (const auto& [key, value] : m_floats)
		ini[FLAG_INI_HEADER_FLOATS][key] = to_string(value);

	file.generate(ini, true);
}
void FlagHandler::load(std::string filename)
{
	mINI::INIFile file(filename);
	mINI::INIStructure ini;

	file.read(ini);

	m_bools.clear();
	m_ints.clear();
	m_floats.clear();

	if (ini.has(FLAG_INI_HEADER_BOOLS))
		for (auto const& it : ini[FLAG_INI_HEADER_BOOLS])
			m_bools[it.first] = to_bool(it.second);
	if (ini.has(FLAG_INI_HEADER_INTS))
		for (auto const& it : ini[FLAG_INI_HEADER_INTS])
			m_ints[it.first] = to_int(it.second);
	if (ini.has(FLAG_INI_HEADER_FLOATS))
		for (auto const& it : ini[FLAG_INI_HEADER_FLOATS])
			m_floats[it.first] = to_float(it.second);
}

bool FlagHandler::to_bool(std::string s)
{
	return s == "true";
}
int FlagHandler::to_int(std::string s)
{
	return stoi(s);
}
float FlagHandler::to_float(std::string s)
{
	return stof(s);
}

std::string FlagHandler::to_string(bool b)
{
	if (b)
		return "true";
	return "false";
}
std::string FlagHandler::to_string(int i)
{
	return std::to_string(i);
}
std::string FlagHandler::to_string(float f)
{
	std::ostringstream ss;
	ss << f;
	return ss.str();
}