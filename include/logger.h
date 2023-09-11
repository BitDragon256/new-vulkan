#pragma once

#include <string>
#include <nve_types.h>

namespace logger
{

	void log(std::string str);
	void log(int i);
	void log(uint32_t i);
	void log(float f);
	void log(Vector2 vec);
	void log(Vector3 vec);
	void log_cond(bool cond, std::string str);
	void log_now(std::string str);

	void log_err(std::string err);
	void log_cond_err(bool cond, std::string err);

} // namespace logger