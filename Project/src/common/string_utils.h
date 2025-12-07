#pragma once
#include <string>

inline std::string trimString(const std::string &str)
{
	if (str.empty())
		return str;

	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";

	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}

inline bool isValidCommand(const std::string &command)
{
	return !command.empty() && command.length() > 0;
}