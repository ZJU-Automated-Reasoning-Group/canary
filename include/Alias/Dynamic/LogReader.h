#pragma once

#include "Alias/Dynamic/LogRecord.h"

#include <boost/optional.hpp>
#include <fstream>
#include <vector>

namespace dynamic
{

class EagerLogReader
{
public:
	EagerLogReader() = delete;

	static std::vector<LogRecord> readLogFromFile(const char* fileName);
};

class LazyLogReader
{
private:
	std::ifstream ifs;
public:
	LazyLogReader(const char* fileName);

	boost::optional<LogRecord> readLogRecord();
};

}
