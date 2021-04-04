#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <list>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "wdigraph.h"
#include "dijkstra.h"

struct Point {
    long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
  long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
  return abs(dLat) + abs(dLon);
}

// finds the point that is closest to a given point, pt
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
  pair<int, Point> best = *points.begin();

  for (const auto& check : points) {
    if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
      best = check;
    }
  }
  return best.first;
}

// reads graph description from the input file and builts a graph instance
void readGraph(const string& filename, WDigraph& g, unordered_map<int, Point>& points) {
  ifstream fin(filename);
  string line;

  while (getline(fin, line)) {
    // split the string around the commas, there will be 4 substrings either way
    string p[4];
    int at = 0;
    for (auto c : line) {
      if (c == ',') {
        // starting a new string
        ++at;
      }
      else {
        // appending a character to the string we are building
        p[at] += c;
      }
    }

    if (at != 3) {
      // empty line
      break;
    }

    if (p[0] == "V") {
      // adding a new vertex
      int id = stoi(p[1]);
      assert(id == stoll(p[1])); // sanity check: asserts if some id is not 32-bit
      points[id].lat = static_cast<long long>(stod(p[2])*100000);
      points[id].lon = static_cast<long long>(stod(p[3])*100000);
      g.addVertex(id);
    }
    else {
      // adding a new directed edge
      int u = stoi(p[1]), v = stoi(p[2]);
      g.addEdge(u, v, manhattan(points[u], points[v]));
    }
  }
}


bool readA(int socket)
{

	fd_set set;
	struct timeval timeout = {10,0};
	int tmout;
	FD_ZERO(&set); /* clear the set */
  	FD_SET(socket, &set);

	if ((tmout = select(socket + 1, &set, NULL, NULL, &timeout)) == 0)
	{
		cout << "Time Out!" << endl;
		return false;
	}

	int buffer[512] = {0};
	int read_in = read(socket, buffer, 512);
	if (buffer[0] != 'A')
	{	
		cout << "Invalid Request" << endl;
		return false;
	}
	return true;
}


// Keep in mind that in Part I, your program must handle 1 request
// but in Part 2 you must serve the next request as soon as you are
// done handling the previous one
int main(int argc, char* argv[]) {
	WDigraph graph;
  	unordered_map<int, Point> points;

  // build the graph
	readGraph("edmonton-roads-2.0.1.txt", graph, points);

  // In Part 2, client and server communicate using a pair of sockets
  // modify the part below so that the route request is read from a socket
  // (instead of stdin) and the route information is written to a socket

  // read a request

	int sock_desc = socket(AF_INET, SOCK_STREAM, 0);
	long arg;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(8888);

	if (bind(sock_desc, (sockaddr*)&address, sizeof(address))<0)
	{
		cerr << "bind failure" << endl;
		close(sock_desc);
		return 0;
	}

	cout << "Listening to port " << 8888 << endl;
	if(listen(sock_desc, 8)<0)
	{
		cerr << "Listen failed" << endl;
		return 0;
	}
	cout << "Done Listening" << endl;

	while(true)
	{
		char buffer[512];
		for (int i = 0; i < 512; i++)
		{
			buffer[i] = '\0';
		}
		cout << "Accepting" << endl;
		int conn_socket;
		if ((conn_socket = accept(sock_desc, (sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
		{
			cerr << "Accept Failure" << endl;
			return 0;
		}
		cout  << "Done Accepting" << endl;

		while (true) 
		{
			
			cout << "Ready to take routing requests" << endl;
			int read_in = read(conn_socket, buffer, 512);
			char x;
			string val = "";
			vector<long long> coords;
			if (buffer[0] != 'R') 
			{
				cerr << "Invalid Request" << endl;
				continue;
			}
			for (int i = 2; i < 512; i++)
			{
				if (buffer[i] == ' ')
				{
					coords.push_back(stoll(val));
					val = "";
				}
				if (buffer[i] == '\0') break;
				val += buffer[i];
			}
			coords.push_back(stoll(val));
	
  			Point sPoint, ePoint;
  			sPoint.lat = coords[0];
			sPoint.lon = coords[1];
  			ePoint.lat = coords[2];
			ePoint.lon = coords[3];
	
  			// c is guaranteed to be 'R' in part 1, no need to error check until part 2
	
  			// get the points closest to the two points we read
  			int start = findClosest(sPoint, points), end = findClosest(ePoint, points);

			// run dijkstra's, this is the unoptimized version that does not stop
  			// when the end is reached but it is still fast enough
  			unordered_map<int, PIL> tree;
  			dijkstra(graph, start, tree);
	
  			// no path
  			if (tree.find(end) == tree.end()) {
      			cout << "N 0" << endl;
  			}
  			else {
    				// read off the path by stepping back through the search tree
    			list<int> path;
    			while (end != start) {
      			path.push_front(end);
      			end = tree[end].first;
    			}
   		 		path.push_front(start);
    			// output the path
    			//cout << "N " << path.size() << endl;
				buffer[0] = 'N';
				buffer[1] = ' ';
				buffer[2] = to_string(path.size())[0];
				buffer[3] = '\0';
				write(conn_socket, buffer, 512);
				if (path.size() == 0)
				{
					continue;			
				}

				bool doublebreak = false;
    			for (int v : path) {
					if (!(readA(conn_socket))) 
					{	
						doublebreak = true;
						break;
					}
  					// cout << "W " << points[v].lat << ' ' << points[v].lon << endl;
					string wpoint = "W " + to_string(points[v].lat) + 
									" " + to_string(points[v].lon);
					for (int i = 0; i < wpoint.size(); i++)
						buffer[i] = wpoint[i];
					buffer[wpoint.size()] = '\0';
					write(conn_socket, buffer, 512);
				}
				if (doublebreak) continue;
				if (!(readA(conn_socket))) continue;
				buffer[0] = 'E';
				buffer[1] = '\0';
				write(conn_socket, buffer, 512);
			}

		}
	}
  return 0;
}
