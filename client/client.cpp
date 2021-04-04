#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <cstring>
#include <arpa/inet.h>
// Add more libraries, macros, functions, and global variables if needed
#define MAX_SIZE 1024
#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"

using namespace std;

int create_and_open_fifo(const char * pname, int mode) {
    // creating a fifo special file in the current working directory
    // with read-write permissions for communication with the plotter
    // both proecsses must open the fifo before they can perform
    // read and write operations on it
    if (mkfifo(pname, 0666) == -1) {
        cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
        exit(-1);
    }

    // opening the fifo for read-only or write-only access
    // a file descriptor that refers to the open file description is
    // returned
    int fd = open(pname, mode);

    if (fd == -1) {
        cout << "Error: failed on opening named pipe." << endl;
        exit(-1);
    }

    return fd;
}

int main(/*int argc, char const *argv[]*/) {
    const char *inpipe = "inpipe";
    const char *outpipe = "outpipe";

    int in = create_and_open_fifo(inpipe, O_RDONLY);
    cout << "inpipe opened..." << endl;
    int out = create_and_open_fifo(outpipe, O_WRONLY);
    cout << "outpipe opened..." << endl;

    char ack[] = "RECEIVED";
    // Your code starts here

    // Here is what you need to do:
    // 1. Establish a connection with the server
    // 2. Read coordinates of start and end points from inpipe (blocks until they are selected)
    //    If 'Q' is read instead of the coordinates then go to Step 7
    // 3. Write to the socket
    // 4. Read coordinates of waypoints one at a time (blocks until server writes them)
    // 5. Write these coordinates to outpipe
    // 6. Go to Step 2
    // 7. Close the socket and pipes

    // Your code ends here
    // Declare structure variables that store local and peer socket addresses
    // sockaddr_in is the address sturcture used for IPv4 
    // sockaddr is the protocol independent address structure
    struct sockaddr_in my_addr, peer_addr;

    // zero out the structor variable because it has an unused part
    memset(&my_addr, '\0', sizeof my_addr);

    // Declare socket descriptor
    int socket_desc;

    char outbound[MAX_SIZE] = {};
    char inbound[MAX_SIZE] = {};

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        cerr << "Listening socket creation failed!\n";
        return 1;
    }

    // Prepare sockaddr_in structure variable
    peer_addr.sin_family = AF_INET;                         
    peer_addr.sin_port = htons(SERVER_PORT);                
    peer_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);    
                                                                                           
    // connecting to the server socket
    if (connect(socket_desc, (struct sockaddr *) &peer_addr, sizeof peer_addr) == -1) {
        cerr << "Cannot connect to the host!\n";
        close(socket_desc);
        return 1;
    }
    cout << "Connection established with " << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << "\n";

    while (true) {
        int bytesread = read(in, outbound, MAX_SIZE);
        if (bytesread == -1)
            cerr << "Error: read operation failed!" << endl;
        if (strcmp("Q", outbound) == 0)
            break;

        send(socket_desc, outbound, strlen(outbound) + 1, 0);
      
        while (true) {
            int rec_size = recv(socket_desc, inbound, MAX_SIZE, 0);
            cout << "Received: " << inbound << endl;
            int byteswrite = write(out, inbound, strlen(inbound)+1);
            if (byteswrite == -1)
                cerr << "Error: write operation failed!" << endl;
            if (strcmp("E", inbound) == 0)
                break;
        }
    }

    // close socket
    close(socket_desc);
    close(in);
    close(out);
    unlink(inpipe);
    unlink(outpipe);
    return 0;
}
