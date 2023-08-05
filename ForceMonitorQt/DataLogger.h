#pragma once

#include <iostream>
#include <string>
#include "Plc.h"

using namespace std;

class DataLogger {
private:
	string log_file_directory;
	string log_file_name;
	FILE* fp;
public:
	DataLogger();
	DataLogger(string directory, string filename);
	~DataLogger();

	bool checkFile();
	void writeDataToFile(PLC_BUFFER_TYPE** data, unsigned int* length);
};

