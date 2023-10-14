/* This code is a heavily modified version of the sample code from "Computer Networks: A Systems
 * Approach," 5th Edition by Larry L. Peterson and Bruce S. Davis. Code comes from a variety of places,
 * largely "Beej's guide to Network Programming", cplusplus.com and stack overflow */

// Developers	 : Drew L Mortenson, Christopher M Claire
// Class info	 : 2238-EECE-446-01-2992
// Semester		 : Fall 2023
// Functionality : This program connects to a P2P network which is hosted by the Jaguar ecc-linux server at
//				   		Chico State University. The program initializes a socket connection without request,
//				        but does not proceed with a JOIN request until the user requests it to be performed.
//						Similarly, the program is capable of publishing it's own files to the registry, and
//						searching the registry for other files of 'x' name. The program waits for user input
//						at each step and will not proceed with any actions (beyond socket connection) until
//                      the user requests it.


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
	vector<string> fileNames;

	if (argc < 4) {
		cout << "Usage: peer <address> <port #>, <peer ID>" << endl;;
		exit(-1);
	}
	//argv[1] = registry address/hostname || argv[2] = port || argv[3] = Peer ID
	const char *host = argv[1];
	const char *port = argv[2];
	int   		  id = atoi(argv[3]);

	/* Lookup IP and connect to server */
	if ( ( s = lookup_and_connect( host, port ) ) < 0 ) {
		cout << "Connection failed" << endl;
		exit( 1 );
	}

	while (1) {
		string choice;

		cout << "Please enter a command (press h for commands): ";
		cin >> choice;

		if (choice == "EXIT") {
			break;
		} else if (choice == "h") {
			cout << endl << "JOIN = Connect to P2P network" << endl << "PUBLISH = Push SharedFiles to registry" << endl;
			cout << "SEARCH = Find a peer who has 'x' file" << endl << "EXIT = Shutdown connection" << endl;
		} else if (choice == "JOIN") {
			int ID = htonl(id);
			uint8_t action = 0;
			char request[40];
			memcpy(request,&action,sizeof(action));
			memcpy(request+sizeof(action),&ID,sizeof(ID));
			
			int sent = send(s, request, 5, 0);
			if (errno != 0) {
				cout << "Send error: " << errno << endl;
				perror("JOIN: ");
			}
			// } else if (sent == 5) {
			// 	cout << "Join Successful" << endl;
			// } else {
			// 	cout << "Join Success, incorrect data sent" << endl;
			// }

		} else if (choice == "PUBLISH") {
			DIR* dir = opendir(sharedFileDir); // Open the directory containing the files to be published

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

			// Since we inserted the file count data prior to the file name data, all bytes and data should
			//		 be properly ordered.
			
			char* sentData = publishRequest.data();
			size_t sendSize = publishRequest.size();

			size_t sentSize = send(s, sentData, sendSize, 0);

			if (sentSize != sendSize) {
				// if this statement is 'true' then check wireshark to ensure that data sent.
				// if the data sent properly, replace "sentSize != sendSize" with "sentSize < 0"
				cerr << "Sent size != Intended send size, please assure data has sent properly" << endl;
				cerr << "Sent size: " << sentSize << endl;
				cerr << "Expected : " << sendSize << endl;
			} else {
				cout << "Sent: " << sentSize << " bytes of data" << endl;
			}

		} else if (choice == "SEARCH") {
			vector<char> searchRequest;
			string fileName;

			cout << "Enter a file name: ";
			cin  >> fileName;

			searchRequest.push_back(0x02); // Add action byte to "front" of request

			for (char c : fileName) { // for c = first char in fileName to the last char,
				searchRequest.push_back(c); // add the char to the back of the request
			}

			searchRequest.push_back(0x00);

			char* sentData = searchRequest.data();
			size_t sendSize = searchRequest.size();

			size_t sentSize = send(s, sentData, sendSize, 0);

			if (sentSize != sendSize) {
				cerr << "Sent size != intended send size, please assure data has sent properly" << endl;
				cerr << "Sent size: " << sentSize << endl;
				cerr << "Expected : " << sendSize << endl;
			} else {
				cout << "Sent: " << sentSize << " bytes of data" << endl;
			}
			recv(s, buf, sizeof(buf), 0);

			uint32_t peerID;
			uint32_t peerIP;
			uint16_t peerPort;
			char pIP[INET_ADDRSTRLEN];  // char array of size IPV4
			for (int i = 0; i < INET_ADDRSTRLEN; i++) {
				pIP[i] = 0;
			}
			memcpy(&peerID, &buf[0], 4);
			memcpy(&peerIP, &buf[4], 4);
			memcpy(&peerPort, &buf[8], 2);
			peerID = ntohl(peerID);
			peerPort = ntohs(peerPort);
			inet_ntop(AF_INET, &peerIP, pIP, INET_ADDRSTRLEN);

			if ((peerID == 0) & (peerPort == 0) & 
				(pIP[0] == 0) & (pIP[1] == 0) &
				(pIP[2] == 0) & (pIP[3] == 0) ) {
				cout << "File not indexed by registry" << endl;
			} else {
				cout << "file found at" << endl;
				cout << " Peer " << peerID << endl << " ";
				for(int i = 0; i < INET_ADDRSTRLEN; i++) {
					cout << pIP[i];
				}
				cout <<  ":" << peerPort << endl;
			}
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
