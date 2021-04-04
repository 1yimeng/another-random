#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <list>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>

using namespace std;

int main()
{

	int sock_desc_client;
	struct sockaddr_in server_address;

	if ((sock_desc_client = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cerr << "Error Creating Socket" << endl;
		return 0;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(8888);

	if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
	{
		cerr << "Invalid Address" << endl;
		return 0;
	}

	if (connect(sock_desc_client, (sockaddr *) &server_address, sizeof(server_address)) < 0)
	{
		cerr << "Connection Failed" << endl;
		return 0;
	}
	
	char buffer[512] = {0};
	string line;
	while (getline (cin, line))
	{
		write(sock_desc_client, line.c_str(), strlen(line.c_str()));
		cout << "Message Sent" << endl;
		read(sock_desc_client, buffer, 512);
		cout << buffer << endl;
	}

	return 0;
}
