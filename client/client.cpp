// Name: Yi Meng Wang, Zong Lin Yu
// ID: 1618855, 1614934
// Assignment1b: Trivial Navigation System
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <cstring>
#include <arpa/inet.h>
#include <vector>
#include <sys/time.h>

#define MAX_SIZE 1024
#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"

using namespace std;

int create_and_open_fifo(const char * pname, int mode) 
{
    // creating a fifo special file in the current working directory
    // with read-write permissions for communication with the plotter
    // both proecsses must open the fifo before they can perform
    // read and write operations on it
    if (mkfifo(pname, 0666) == -1) 
    {
        cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
        exit(-1);
    }

    // opening the fifo for read-only or write-only access
    // a file descriptor that refers to the open file description is
    // returned
    int fd = open(pname, mode);

    if (fd == -1) 
    {
        cout << "Error: failed on opening named pipe." << endl;
        exit(-1);
    }

    return fd;
}

int main(int argc, char const *argv[]) 
{
    const char *inpipe = "../inpipe";
    const char *outpipe = "../outpipe";

    int in = create_and_open_fifo(inpipe, O_RDONLY);
    cout << "inpipe opened..." << endl;
    int out = create_and_open_fifo(outpipe, O_WRONLY);
    cout << "outpipe opened..." << endl;

    char ack[] = "RECEIVED";

    // taking port number and ip addresses from command line 
	string port_str = argv[1];
	string ip = argv[2];
	
	int PORT = stoi(port_str);

    // sockaddr_in is the address sturcture used for IPv4 
    // sockaddr is the protocol independent address structure
    struct sockaddr_in my_addr, peer_addr;

    // zero out the structor variable because it has an unused part
    memset(&my_addr, '\0', sizeof my_addr);

    // Declare socket descriptor
    int socket_desc;

    char outbound[MAX_SIZE] = {0};
    char inbound[MAX_SIZE] = {0};
	char from_plot[MAX_SIZE] = {0};
    char outbound2[MAX_SIZE] = {0};

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) 
    {
        cerr << "Listening socket creation failed!\n";
        return 1;
    }

    // Prepare sockaddr_in structure variable
    peer_addr.sin_family = AF_INET;                         
    peer_addr.sin_port = htons(PORT);                
    peer_addr.sin_addr.s_addr = inet_addr(ip.c_str());    
                                                                                           
    // connecting to the server socket
    if (connect(socket_desc, (struct sockaddr *) &peer_addr, sizeof peer_addr) == -1) 
    {
        cerr << "Cannot connect to the host!\n";
        close(socket_desc);
        return 1;
    }
    cout << "Connection established with " << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << "\n";
	
    // setting timer to 1s for timeout
    struct timeval timer = {.tv_sec = 1};
    if (setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (void *) &timer, sizeof(timer)) == -1) 
    {
        cerr << "Cannot set socket options!\n";
        close(socket_desc);
        return 1;
    }

    bool quit = false;

	vector<double> coords = {};
    while (!quit) 
    {
		int read_in;
        // Our code works with plotter whether it sends coordinates once or twice
		while (coords.size() < 4)
		{   
            // Read coordinates of start and end points from inpipe
			read_in = read(in, outbound, MAX_SIZE);
			if (read_in == 0) // plotter sends nothing, go back to the beginning
			{    
				continue;
			}
			if (outbound[0] == 'Q') // plotter sending quit, quit client as well
			{    
				quit = true;
				break;
			}
			// cout << outbound << endl; //debugging messages
			char num_buffer[24] = {0};
			int k = 0;
            
            // The for loop below converts and sorts the coordinates (char array)
            // into a vector of doubles 
			for (int i = 0; i < MAX_SIZE; i++)
			{
				if (outbound[i] == '\n' || outbound[i] == ' ')
				{
					num_buffer[k] = '\0';
					coords.push_back((double)atof(num_buffer));
					k = 0;
					continue;
				}
				if (outbound[i] == '\0') 
				{
					if (k != 0)
					{
						//cout << num_buffer << endl;
						num_buffer[k] = '\0';
						coords.push_back((double)atof(num_buffer));
						k = 0;
					}
					break;
				}
				num_buffer[k] = outbound[i];
				k++;
			}
			memset(outbound, 0, MAX_SIZE);
			memset(num_buffer, 0, 24);
		}
		if (quit)
		{
			string q_msg = "Q";
			send(socket_desc, q_msg.c_str(), q_msg.size(), 0);
			break;
		}

        // creating a string starting with letter R to send to plotter
		string params = "R ";
		params += to_string(static_cast<long long> (coords[0]*100000)) + " ";
		params += to_string(static_cast<long long> (coords[1]*100000)) + " ";
		params += to_string(static_cast<long long> (coords[2]*100000)) + " ";
		params += to_string(static_cast<long long> (coords[3]*100000));

        // cout << params << endl; //debug

        // send coordinates via the socket to the server
        send(socket_desc, params.c_str(), params.size(), 0);
        
        // receiving the first message from the socket
        int rec_size = recv(socket_desc, inbound, MAX_SIZE, 0); 

        // rec_size would be equal to -1 if time out happens
        if (rec_size == -1) 
        { 
            cout << "Timeout occurred... still waiting!\n";
            continue;
        }
        // cout << "Received: " << inbound << endl; //coordinates of locations
      	string val_n_str = (inbound + 2);
		int val_n = stoi(val_n_str);
		// cout << val_n << endl;

        // handlind the edge case of "N 0"
		if (val_n == 0)
		{
			string init_coords = to_string(coords[0]) + " "
							+ to_string(coords[1]) + '\n';
			init_coords += init_coords;
			init_coords += "E\n";
			write(out, init_coords.c_str(), init_coords.size());
			coords = {};
			continue;
		}
	    
		bool signal = true;
        // putting all the coordinates into a vector so it is easier to send
		vector<string> send_to_plotter; 
        while (true) 
        {
            char request[] = {'A'};
			string temp_storage;
            // requesting each node coordinates via socket
            send(socket_desc, request, strlen(request) + 1, 0);
            memset(inbound, 0, MAX_SIZE);
            // receiving node coordinates
			int rec_size = recv(socket_desc, inbound, MAX_SIZE, 0); 
            if (rec_size == -1) 
            {
                cout << "Timeout occurred... still waiting!\n";
                signal = false;
				break;
            }  
            // cout << "Received: " << inbound << endl;
            // the path ends and quit this loop
            if (strcmp("E", inbound) == 0) 
            {
                string end = "E\n";
				send_to_plotter.push_back(end);
				coords = {};
                break;
            }
            // handling invalid responses from the socket
            if (inbound[0] != 'W') 
            {
                cerr << "Error: invalid response from the serve" << endl;
                signal = false;
				break;
            }
			
			// All below are guaranteed to be W cases
            // Converting the received coordinates into string so plotter can read properly
            string str = "";
            vector<long long> received_coords;
            for (int i = 2; i < MAX_SIZE; i++) 
            {
                if (inbound[i] == ' ') 
                {
                    received_coords.push_back(stoll(str));
                    str = "";
                    continue;
                }
                if (inbound[i] == '\0') 
                {
                    received_coords.push_back(stoll(str));
                    str = "";
                    break;
                } 
                str += inbound[i]; 
            }
            string plotter_coords = "";
            plotter_coords += (to_string((static_cast<double> (received_coords[0]))/100000)) + " ";
            plotter_coords += (to_string((static_cast<double> (received_coords[1]))/100000)) + "\n";
            // cout << plotterCoords;

            // handling the edge case of "N 1"
			if (val_n == 1)
			{
				plotter_coords += plotter_coords;
			}

            send_to_plotter.push_back(plotter_coords);
			       
        }

        // if none of the error happened, write to the pipe
		if (signal)
		{
			for (int i = 0; i < send_to_plotter.size(); i++)
			{   
                // Write to the outpipe 
				int byteswrite = write(out, send_to_plotter[i].c_str(), send_to_plotter[i].size());
                if (byteswrite == -1) 
                {
                    cerr << "Error: write operation failed!" << endl;
                }
			}
		}


    }

    // close socket & pipes
    close(socket_desc);
    close(in);
    close(out);
    unlink(inpipe);
    unlink(outpipe);
    return 0;
}
