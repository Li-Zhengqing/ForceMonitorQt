#include "VirtualPlc.h"

VirtualPlc::VirtualPlc() {
	this->recv_buff = new char[VIRTUAL_PLC_RECV_BUFFER_SIZE];
	this->tran_buff = new char[VIRTUAL_PLC_TRAN_BUFFER_SIZE];

	for (int _channel = 0; _channel < 3; _channel++) {
		this->current_raw_data[_channel] = 0;
		this->data_offset[_channel] = 0;
	}

	this->selected_variable_grp_id = 0;
}

VirtualPlc::~VirtualPlc() {
	delete[] this->recv_buff;
	delete[] this->tran_buff;
}

long VirtualPlc::connectPlc(unsigned char target_address[4]) {
	// TODO:
	char target_address_str[100];
	sprintf_s(target_address_str, "%d.%d.%d.%d", target_address[0], target_address[1], target_address[2], target_address[3]);
	// sprintf(target_address_str, "%d.%d.%d.%d", target_address[0], target_address[1], target_address[2], target_address[3]);
	// 初始化 WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		std::cout << "WSAStartup error!" << std::endl;
		return 1;
	}

	// 创建套接字
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		std::cout << "create socket error!" << std::endl;
		return 1;
	}

	// 绑定IP和端口
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(REMOTE_PLC_PORT);
	// sin.sin_addr.S_un.S_addr = inet_addr("192.168.0.42");
	// sin.sin_addr.S_un.S_addr = inet_addr(target_address_str);
	inet_pton(AF_INET, target_address_str, &sin.sin_addr.S_un.S_addr);

	if (connect(clientSocket, (sockaddr*)&sin, sizeof(sockaddr)) == INVALID_SOCKET) {
		cout << "Failed to connect with remote PLC!" << std::endl;
		return 1;
	}
	else {
		this->client_sock = clientSocket;
	}

	return 0;
}

long VirtualPlc::connectLocalPlc() {
	// TODO:
	unsigned char localhost_addr[4] = {127, 0, 0, 1};
	long _res = this->connectPlc(localhost_addr);

	return _res;
}

long VirtualPlc::disconnectPlc() {
	// TODO:
	memset(this->tran_buff, 0, VIRTUAL_PLC_TRAN_BUFFER_SIZE);
	memset(this->recv_buff, 0, VIRTUAL_PLC_RECV_BUFFER_SIZE);
	this->tran_buff[0] = PlcServiceCommand::QUIT;
	send(this->client_sock, tran_buff, 1, 0);
	recv(this->client_sock, recv_buff, VIRTUAL_PLC_RECV_BUFFER_SIZE, 0);

	// closesocket(this->client_sock);
	return 0;
}

long VirtualPlc::startPlc() {
	// TODO:
	memset(this->tran_buff, 0, VIRTUAL_PLC_TRAN_BUFFER_SIZE);
	memset(this->recv_buff, 0, VIRTUAL_PLC_RECV_BUFFER_SIZE);
	this->tran_buff[0] = PlcServiceCommand::START;
	this->tran_buff[1] = this->selected_variable_grp_id;
	// send(this->client_sock, tran_buff, 1, 0);
	send(this->client_sock, tran_buff, 2, 0);
	recv(this->client_sock, recv_buff, VIRTUAL_PLC_RECV_BUFFER_SIZE, 0);

	return 0;
}

long VirtualPlc::stopPlc() {
	// TODO:
	memset(this->tran_buff, 0, VIRTUAL_PLC_TRAN_BUFFER_SIZE);
	memset(this->recv_buff, 0, VIRTUAL_PLC_RECV_BUFFER_SIZE);
	this->tran_buff[0] = PlcServiceCommand::STOP;
	send(this->client_sock, tran_buff, 1, 0);
	recv(this->client_sock, recv_buff, VIRTUAL_PLC_RECV_BUFFER_SIZE, 0);

	return 0;
}

size_t VirtualPlc::fetchDataFromPlc(PLC_BUFFER_TYPE* dst) {
	// TODO: ptr dst is not used.
	// cout << "Client fetching data." << std::endl;
	memset(this->tran_buff, 0, VIRTUAL_PLC_TRAN_BUFFER_SIZE);
	memset(this->recv_buff, 0, VIRTUAL_PLC_RECV_BUFFER_SIZE);
	this->tran_buff[0] = PlcServiceCommand::QUERY;
	send(this->client_sock, tran_buff, 1, 0);
	// cout << "Request sent." << std::endl;
	int _data_length = recv(this->client_sock, recv_buff, VIRTUAL_PLC_RECV_BUFFER_SIZE, 0);
	if ((_data_length == 1) && (recv_buff[0] == 0)) {
		// cout << "Null datagram received" << std::endl;
		return 0;
	}
	// cout << "Client data acquired." << std::endl;

	return _data_length;
}

void VirtualPlc::copyDataToClient(PLC_BUFFER_TYPE** dst, unsigned int* ret) {
	ret[0] = 0;
	ret[1] = 0;
	ret[2] = 0;
	memset(this->tran_buff, 0, VIRTUAL_PLC_TRAN_BUFFER_SIZE);
	memset(this->recv_buff, 0, VIRTUAL_PLC_RECV_BUFFER_SIZE);
	this->tran_buff[0] = PlcServiceCommand::QUERY;
	send(this->client_sock, tran_buff, 1, 0);
	// cout << "Request sent." << std::endl;
	// int _data_length = recv(this->client_sock, recv_buff, VIRTUAL_PLC_RECV_BUFFER_SIZE, 0);
	int _data_length = recv(this->client_sock, recv_buff, VIRTUAL_PLC_RECV_BUFFER_SIZE / 2, 0);
	if ((_data_length == 1) && (recv_buff[0] == 0)) {
		// cout << "Null datagram received" << std::endl;
		return;
	}
	// cout << "Client data acquired." << std::endl;

	LOG(INFO) << "Length of byte received by client: " << _data_length << std::endl;
	_data_length = _data_length / sizeof(PLC_BUFFER_TYPE);
	cout << "Length of data received by client: " << _data_length << std::endl;
	LOG(INFO) << "Length of data received by client: " << _data_length << std::endl;
	int _channel = 0;
	int _channel_index = 0;
	for (int i = 0; i < _data_length; i++) {
		_channel = (i % 3);
		_channel_index = (i / 3);
		dst[_channel][_channel_index] = ((double*)recv_buff)[i] - this->data_offset[_channel];
		this->current_raw_data[_channel] = ((double*)recv_buff)[i];
		ret[_channel] += 1;
	}
}

void VirtualPlc::setVariableGroupId(char selected_variable_grp_id) {
	this->selected_variable_grp_id = selected_variable_grp_id;
}

void VirtualPlc::resetDataOffset(int* flag, PLC_BUFFER_TYPE* ref_value) {
	for (int _channel = 0; _channel < 3; _channel++) {
		if (flag[_channel]) {
			this->data_offset[_channel] = this->current_raw_data[_channel] - ref_value[_channel];
		}
	}
}
