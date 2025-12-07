#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger
{
private:
	std::ostream *_logStreamPtr;
	std::mutex _mutex;

public:
	Logger(const std::string &filename);
	Logger(std::ostream &stream);
	~Logger();

	void log(const std::string &category, const std::string &message);
	bool is_open() const;
	void close();
	
	// Static utility function for timestamp generation
	static std::string getTimestamp();
};
