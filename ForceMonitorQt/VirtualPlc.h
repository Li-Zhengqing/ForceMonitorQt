#pragma once

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "winsock2.h"
#include "Ws2tcpip.h"

#pragma comment(lib, "ws2_32.lib")

#include "Plc.h"

#define REMOTE_PLC_PORT 41720
// #define VIRTUAL_PLC_RECV_BUFFER_SIZE 1000000
#define VIRTUAL_PLC_RECV_BUFFER_SIZE 800000
#define VIRTUAL_PLC_TRAN_BUFFER_SIZE 100

enum PlcServiceCommand { ECHO = 1, START, STOP, QUERY, QUIT };

// class VirtualPlc : public Plc {
class VirtualPlc {
private:
	SOCKET client_sock;
	char* recv_buff;
	char* tran_buff;

    char selected_variable_grp_id;

    PLC_BUFFER_TYPE current_raw_data[3];
    PLC_BUFFER_TYPE data_offset[3];
	
public:
	VirtualPlc();
	~VirtualPlc();

    long connectPlc(unsigned char target_address[4]);

    long connectLocalPlc();

    long disconnectPlc();

    // long getPlcInfo();

    long startPlc();

    long stopPlc();

    // long initPlcVarHdl();

    // long queryPlcData();

    size_t fetchDataFromPlc(PLC_BUFFER_TYPE* dst);

    void copyDataToClient(PLC_BUFFER_TYPE** dst, unsigned int* ret);

    void setVariableGroupId(char selected_varaible_grp_id);

    void resetDataOffset(int* flag, PLC_BUFFER_TYPE* ref_value);
};
