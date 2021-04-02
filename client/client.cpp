#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
// Add more libraries, macros, functions, and global variables if needed

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

int main(int argc, char const *argv[]) {
    const char *inpipe = "inpipe";
    const char *outpipe = "outpipe";

    int in = create_and_open_fifo(inpipe, O_RDONLY);
    cout << "inpipe opened..." << endl;
    int out = create_and_open_fifo(outpipe, O_WRONLY);
    cout << "outpipe opened..." << endl;

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

    close(in);
    close(out);
    unlink(inpipe);
    unlink(outpipe);
    return 0;
}
