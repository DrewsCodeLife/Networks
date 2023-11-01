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

// To be returned by Search function
struct peerInfo {
	int port = 0;
	char peerIP[INET_ADDRSTRLEN];
	string filename;
};

int lookup_and_connect( const char *host, const char *service );
peerInfo search(int s, bool silent);
const char* int_to_cstr(int integer, char port[]);

int main(int argc, char *argv[]) {
	int s;
	int sock;
	const char* sharedFileDir = "SharedFiles";
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

		cout << "Enter a command: ";
		cin >> choice;

		if (choice == "EXIT") {
			break;
		} else if (choice == "JOIN") {
			int ID = htonl(id);
			uint8_t action = 0;
			char request[40];
			memcpy(request,&action,sizeof(action));
			memcpy(request+sizeof(action),&ID,sizeof(ID));
			
			errno = 0;
			send(s, request, 5, 0);
			if (errno != 0) {
				cout << "Send error: " << errno << endl;
				perror("JOIN: ");
			}

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

			errno = 0;
			size_t sentSize = send(s, sentData, sendSize, 0);
			if (errno != 0) {
				cout << "Publish error: " << errno << endl;
				perror("PUBLISH: ");
			}

			if (sentSize != sendSize) {
				cerr << "Sent size != Intended send size, please assure data has sent properly" << endl;
				cerr << "Sent size: " << sentSize << endl;
				cerr << "Expected : " << sendSize << endl;
			}

		} else if (choice == "SEARCH") {
			search(s, false);
		} else if(choice == "FETCH") {
			vector<char> fetchrequest;
			// Search(s, true) function handles request and returns peerInfo struct containing:
			//													  port, peerIP[], and filename
			peerInfo peer = search(s, true);
			char filename[sizeof(peer.filename)];
			strncpy(filename, peer.filename.c_str(), sizeof(filename));
			// peer.port contains port #, peer.peerIP[] contains peer ip,

			if(!peer.filename.empty() && peer.filename.back() != '\0') {
				peer.filename.append("\0");
			}

			char pport[5];
			int_to_cstr(peer.port, pport);
			if ( ( sock = lookup_and_connect( peer.peerIP, as_const(pport) ) ) < 0 ) {
				cout << "Connection to peer failed" << endl;
				exit( 1 );
			}

			ofstream file(peer.filename, std::ios::binary);

			if(!file.is_open()) {
				cerr << "Fetch failed to open file for save." << endl;
				exit(1);
			}

			// If lookup_and_connect successfully connected, then sock now contains an open socket with peer
			fetchrequest.push_back(0x03);
			for(char currentChar : peer.filename) {
				fetchrequest.push_back(currentChar);
			}

			char* sentData = fetchrequest.data();
			size_t sendSize = fetchrequest.size();

			cout << "Fetch request: ";
			for(char c : fetchrequest) {
				cout << c;
			}
			cout << endl;

			errno = 0;
			size_t sentSize = send(sock, sentData, sentSize, 0);
			if (errno != 0) {
				cout << "FETCH error: " << errno << endl;
				perror("FETCH: ");
			}
			if (sentSize != sendSize) {
				cerr << "Sent size != Intended send size, please assure data has sent properly" << endl;
				cerr << "Sent size: " << sentSize << endl;
				cerr << "Expected : " << sendSize << endl;
			}

			int bytesRead;
			int responseCode;
			char buf[1024];
			bytesRead = recv(sock, &responseCode, 1, 0);
			responseCode = ntohl(responseCode);
			if(responseCode == 0) {
				while (true) {
					// loop until bytesRead = 0, first loop bytesRead = 1
					errno = 0;
					bytesRead = recv(sock, buf, sizeof(buf), 0);
					if (errno != 0) {
						cerr << "Error receiving FETCH response" << endl;
						perror("FETCH response: ");
						break;
					}

					if (bytesRead == 0) {
						break;
					}

					file.write(buf, bytesRead);
				}
			} else {
				cout << "Received error response from peer" << endl;
			}

			file.close();
			close(sock);
		} else {
			cout << endl << "JOIN = Connect to P2P network" << endl << "PUBLISH = Push SharedFiles to registry" << endl;
			cout << "SEARCH = Find a peer who has 'x' file" << endl << "EXIT = Shutdown connection" << endl << endl;
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

int fetch(int s) {
	return 0;
}

peerInfo search(int s, bool silent) {
	char buf[2048];
	peerInfo result;
	vector<char> searchRequest;
	string fileName;

	cout << "Enter a file name: ";
	cin  >> fileName;
	result.filename = fileName;

	searchRequest.push_back(0x02); // Add action byte to "front" of request

	for (char c : fileName) { // for c = first char in fileName to the last char,
		searchRequest.push_back(c); // add the char to the back of the request
	}

	searchRequest.push_back(0x00);

	char* sentData = searchRequest.data();
	size_t sendSize = searchRequest.size();

	errno = 0;
	size_t sentSize = send(s, sentData, sendSize, 0);
	if (errno != 0) {
		cout << "Search send error: " << errno << endl;
		perror("SEARCH: ");
	}

	if (sentSize != sendSize) {
		cerr << "Sent size != intended send size, please assure data has sent properly" << endl;
		cerr << "Sent size: " << sentSize << endl;
		cerr << "Expected : " << sendSize << endl;
	}
	errno = 0;
	recv(s, buf, sizeof(buf), 0);
	if (errno != 0) {
		cout << "Search receive error: " << errno << endl;
		perror("SEARCH: ");
	}

	uint32_t peerID;
	uint32_t peerIP;
	uint16_t peerPort;
	char pIP[INET_ADDRSTRLEN];  // char array of size IPV4 address
	for (int i = 0; i < INET_ADDRSTRLEN; i++) {
		pIP[i] = 0;
	}
	memcpy(&peerID, &buf[0], 4);
	memcpy(&peerIP, &buf[4], 4);
	memcpy(&peerPort, &buf[8], 2);
	peerID = ntohl(peerID);
	peerPort = ntohs(peerPort);
	inet_ntop(AF_INET, &peerIP, pIP, INET_ADDRSTRLEN);

	if(silent == false) {
		if ((peerID == 0) & (peerPort == 0)) {
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
		result.port = peerPort;
		for (int i = 0; i < INET_ADDRSTRLEN; i++) {
			result.peerIP[i] = pIP[i];
		}
	}
	return result;
}

const char* int_to_cstr(int integer, char port[]) {
	sprintf(port, "%d", integer);
	return port;
}