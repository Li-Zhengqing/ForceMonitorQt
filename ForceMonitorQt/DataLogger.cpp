#include "DataLogger.h"

DataLogger::DataLogger() {
	this->log_file_directory = ".";
	this->log_file_name = "data.csv";

	string _file = this->log_file_directory + "/" + this->log_file_name;
	this->fp = fopen(_file.c_str(), "w");
	fprintf(this->fp, "Force Sensor Record");
	fclose(fp);
}

DataLogger::DataLogger(string directory, string filename) {
	this->log_file_directory = directory;
	if (filename.length() != 0) {
		this->log_file_name = filename;
	}
	else {
		this->log_file_name = "data.csv";
	}

	string _file = this->log_file_directory + "/" + this->log_file_name;
	this->fp = fopen(_file.c_str(), "w");
	fprintf(this->fp, "Force Sensor Record");
	fclose(fp);

}

DataLogger::~DataLogger() {
	// fclose(fp);
}

// FIXME: Cannot detect new file name?
bool DataLogger::checkFile() {
	string _file = this->log_file_directory + "/" + this->log_file_name;
	FILE* _fp = NULL;
	if (_fp = fopen(_file.c_str(), "r")) {
		fclose(_fp);
		return true;
	}
	else {
		return false;
	}
}

void DataLogger::writeDataToFile(PLC_BUFFER_TYPE** data, unsigned int* length) {
	string _file = this->log_file_directory + "/" + this->log_file_name;
	this->fp = fopen(_file.c_str(), "a");
	// fp = fopen();
	unsigned int _length = length[0];
	for (int _channel = 1; _channel < 3; _channel++) {
		if (length[_channel] < _length) {
			_length = length[_channel];
		}
	}

	for (int i = 0; i < _length; i++) {
		fprintf(this->fp, "%lf, %lf, %lf, \n", data[0][i], data[1][i], data[2][i]);
	}

	fclose(fp);
}
