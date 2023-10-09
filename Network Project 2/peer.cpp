/* This code is an updated version of the sample code from "Computer Networks: A Systems
 * Approach," 5th Edition by Larry L. Peterson and Bruce S. Davis. Some code comes from
 * man pages, mostly getaddrinfo(3). */

// Developers	 : Drew L Mortenson, Christopher M Claire
// Class info	 : 2238-EECE-446-01-2992
// Semester		 : Fall 2023
// Functionality : This program will connect to the server defined in *host and *port, then send the request
//				        specified in 'strang'. It will then recieve all data sent by the aforementioned server
//				        and count the <p> tags as well as the total bytes received. Christopher and Drew have
//						added functionality to receive the data, as well as to count the <p> tags and bytes.
//						In addition, they have added the functionality to receive a command line argument
//						specifying the desired chunk size for packets.


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <arpa/inet.h>
#include <vector>
#include <bitset>
#include <bits/stdc++.h>
#include <fstream>
#include <dirent.h>

using namespace std;

/*
 * Lookup a host IP address and connect to it using service. Arguments match the first two
 * arguments to getaddrinfo(3).
 *
 * Returns a connected socket descriptor or -1 on error. Caller is responsible for closing
 * the returned socket.
 */
int lookup_and_connect( const char *host, const char *service );

int main(int argc, char *argv[]) {
	int s;
	const char* sharedFileDir = "SharedFiles";
	char buf[2048];
	char strang[] = "GET /~kkredo/file.html HTTP/1.0\r\n\r\n";
	vector<string> fileNames;

	if (argc < 4) {
		cout << "Usage: peer <address> <port #>, <peer ID>" << endl;;
		exit(-1);
	}
	//argv[1] = registry address/hostname || argv[2] = port || argv[3] = Peer ID
	const char *host = argv[1];
	const char *port = argv[2];
	int   		  id = atoi(argv[3]);

	while (1) {
		string choice;
		string store;

		/* Lookup IP and connect to server */
		if ( ( s = lookup_and_connect( host, port ) ) < 0 ) {
			cout << "Connection failed" << endl;
			exit( 1 );
		}

		cout << "Please enter a command (press h for commands): ";
		cin >> choice;

		if (choice == "EXIT") {
			break;
		} else if (choice == "h") {
			cout << endl << "JOIN = Connect to P2P network" << endl << "PUBLISH = Push SharedFiles to registry" << endl;
			cout << "SEARCH = Find a peer who has 'x' file" << endl << "EXIT = Shutdown connection" << endl;
		} else if (choice == "JOIN") {
			int ID = htonl(id);
			string joinRequest = "00000001" + bitset<32>(ID).to_string();
			cout << joinRequest << endl;
			const char* request[joinRequest.length()] = {joinRequest.c_str()};
			for (unsigned int i = 0; i < joinRequest.length(); i++) {
				cout << request[i];
			}
			cout << endl;

			if (send(s, request, 5, 0) == 5) {
				if (errno != 0) {
					cout << "Send error: " << errno << endl;
					perror("JOIN: ");
				} else {
					cout << "Join Successful" << endl;
				}
			}

		} else if (choice == "PUBLISH") {
			// Submit a publish request
			// ALL FILES ARE IN "SharedFiles" folder, must publish all available files in first shot
			
			// Send buffer in form of:
			// |ACTION | FILE COUNT, MUST EQUAL NULL COUNT   | NULL TERMINATED FILE NAME(S)
			// |-------|-------------------------------------|----------------------------
			// |   1   | 00000000 00000000 00000000 00000000 | xxxxxxxx (less than 100 bytes per filename,
			//                                                                 less than 1200 bytes total)

			DIR* dir = sharedFileDir; // Open the directory containing the files to be published

			if (dir) { // if successful, proceed with publish
				struct dirent* entry;
				while((entry = readdir(dir)) != NULL) { // readdir returns a pointer to essentially a file
														// descriptor, if NULL, then end of directory
					if (entry->d_type == DT_REG) { // if the file is of regular type, then:
						fileNames.push_back(entry->d_name); // Add the file name from to the back of the vector
					}
				}
				closedir(dir);
			} else{
				cerr << "Error opening directory for publish" << endl;
				exit(-1);
			}

			uint32_t fileCount = htonl(static_cast<uint32_t>(fileNames.size()));
			// Creates fileCount, ensures that bytes are in network order
			
			vector<char> publishRequest;
			publishRequest.push_back(0x01); // Throw a 0x01 descriptor at the front to say action = 1
			publishRequest.insert(
				publishRequest.end(),
				reinterpret_cast<const char*>(&fileCount),
				reinterpret_cast<const char*>(&fileCount) + sizeof(fileCount)
			); // Carefully inserts the file count data at the end of the publish request

			for (const string& fileName : fileNames) {
				publishRequest.insert(publishRequest.end(), fileName.c_str(), fileName.c_str() + fileName.size() +1);
			} // Again, inserts the file *NAME* data at the end of the publish request
			
			char* sentData = publishRequest.data();
			size_t sendSize = publishRequest.size();

			size_t sentSize = send(s, sentData, sendSize, 0);

			if (sentSize != sendSize) {
				cerr << "Sent size != Intended send size, please assure data has sent properly" << endl;
			} else {
				cout << "Sent: " << sentSize << " bytes of data" << endl;
			}

		} else if (choice == "SEARCH") {
			// search stuff
		} else {
			cout << "Input invalid, try again" << endl;
		}
	}

	close( s );
	return 0;
}

int lookup_and_connect( const char *host, const char *service ) {
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	/* Translate host name into peer's IP address */
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if ( ( s = getaddrinfo( host, service, &hints, &result ) ) != 0 ) {
		fprintf( stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror( s ) );
		return -1;
	}

	/* Iterate through the address list and try to connect */
	for ( rp = result; rp != NULL; rp = rp->ai_next ) {
		if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
			continue;
		}

		if ( connect( s, rp->ai_addr, rp->ai_addrlen ) != -1 ) {
			break;
		}

		close( s );
	}
	if ( rp == NULL ) {
		perror( "stream-talk-client: connect" );
		return -1;
	}
	freeaddrinfo( result );

	return s;
}
