#include "logger.h"

Logger::Logger(const std::string &filename)
{
	_logStreamPtr = new std::ofstream(filename, std::ios::app);
}
Logger::Logger(std::ostream &stream)
{
	_logStreamPtr = &stream;
}

Logger::~Logger()
{
	if (_logStreamPtr && _logStreamPtr != &std::cout)
	{
		delete _logStreamPtr;
	}
}

void Logger::log(const std::string &category, const std::string &message)
{
	std::lock_guard<std::mutex> lock(_mutex);

	if (_logStreamPtr)
	{
		auto now = std::chrono::system_clock::now();
		auto time_t = std::chrono::system_clock::to_time_t(now);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		*_logStreamPtr << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
									 << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
									 << "[" << category << "] " << message << std::endl;
		_logStreamPtr->flush();
	}
}

bool Logger::is_open() const
{
	if (_logStreamPtr)
	{
		if (auto fileStream = dynamic_cast<std::ofstream *>(_logStreamPtr))
		{
			return fileStream->is_open();
		}
		return true; // For other stream types like std::cout
	}
	return false;
}

void Logger::close()
{
	if (_logStreamPtr && _logStreamPtr != &std::cout)
	{
		static_cast<std::ofstream *>(_logStreamPtr)->close();
	}
}

std::string Logger::getTimestamp()
{
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
	return ss.str();
}
