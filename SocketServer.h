/*
	This file contains the necessary utility functions to be able to establish connection with socket client
	It crates a socket server that listens to only one client
*/
#pragma once

#include <thread>

#define BUFFER_SIZE 1024

class SocketServer
{
public:
	void StartServer();

	SocketServer();

	~SocketServer();

	bool HasClient;

	int Mood;

private:
	const int PORT = 7748;
	std::thread _clientThread;
	bool _ready;

	void HandleClientMessages(int server);

	void BuildMessage(const char* message);

	int ProcessMessage(const int& messageValue);
};

