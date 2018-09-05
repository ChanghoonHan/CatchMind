#pragma once

#include "Server.h"
#include <random>

int main()
{
	Server server;
	server.GetWords();
	server.StartServer(9000);

	return 0;
}