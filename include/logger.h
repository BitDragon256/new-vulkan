#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <nve_types.h>

namespace logger
{

	void log(std::string str);
	void log(int i);
	void log(uint32_t i);
	void log(size_t i);
	void log(float f);
	void log(Vector2 vec);
	void log(Vector3 vec);
	template<typename T> inline void log(std::vector<T> vec)
	{
		for (const T t : vec)
		{
			std::cout << t << ", ";
		}
		std::cout << "\n";
	}
	template<typename T> inline void log(std::string label, T data)
	{
		std::cout << label << ": ";
		log(data);
	}

	void log_cond(bool cond, std::string str);
	void log_now(std::string str);

	void log_err(std::string err);
	void log_cond_err(bool cond, std::string err);

} // namespace logger