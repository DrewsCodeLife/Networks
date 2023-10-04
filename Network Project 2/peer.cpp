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
#include <bits/stdc++.h>

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
	char buf[2048];
	bool quit = false;
	char strang[] = "GET /~kkredo/file.html HTTP/1.0\r\n\r\n";

	//argv[1] = registry address/hostname || argv[2] = port || argv[3] = Peer ID

	if(errno != 0) {
		errno=0;
	}

	if (argc < 4) {
		cout << "Usage: peer <address> <port #>, <peer ID>" << endl;;
		exit(-);
	}
	const char *host = argv[1];
	const char *port = argv[2];
	short int   id = stoi(argv[3]);

	while (quit == false) {
		string choice;
		string store;

		/* Lookup IP and connect to server */
		if ( ( s = lookup_and_connect( host, port ) ) < 0 ) {
			cout << "Connection failed" << endl;
			exit( 1 );
		}

		// if (JOIN) -> check length, htons(), assign joinRequest, send request

		// ADD SOME SORT OF CLARIFICATION ON INPUT ARGUMENTS (JOIN <peer ID> = .....)
		cout << "Please provide input:" << endl << "JOIN = connect to p2p network" << endl;
		cout << "Publish = push data to p2p network" << "SEARCH = search p2p network for some"
		cin >> choice;
		stringstream ss(choice);
		vector(string) v;

		while(getline(ss, store, " ")) {
			v.push_back(store); // first element will be command, subsequent elements will be arguments.
							   // for each command, we must check that the length of the vector matches
							   // the quantity expected.
		}

		if (v[1] == "JOIN") {
			if (v.size() != 2) {
				cout << "Improper quantity of arguments" << endl << "Expected: PEER <peer ID>" << endl;
			} else {
				// Proceed with JOIN request
				if(id < 0 | id > 15) {
					cout << "Peer ID must be 4 bit (0-15)" << endl;
				}
		
				id = htons(id);
				unsigned char joinRequest = (0 << 4)  | id; // uns char used to represent binary
				// "assign bit 1 = 0, then push 0 four bits to the left (00000)
				// OR that binary string with peer ID"

				if (send(s, &joinRequest, sizeof(joinRequest), 0) == -1) {
					perror("send");
					close(s);
					exit(1);
				}
			}
		} else if (v[1] == "PUBLISH") {
			// if (num arguments != expected) -> explain usage, else proceed with publish
		} else if (v[1] == "SEARCH") {
			// if (num arguments != expected) -> explain usage, else proceed with search
		} else if (v[1] == "EXIT") {
			// exit my guy, close connection properly (!!! don't just return 0; !!!)
		} else {
			cout << "Improper input, try again" << endl;
		}

		/*A PUBLISH request includes a 1 B field containing 1 (Action equals 1), a 4 B file count, and a list of
NULL-terminated file names. The PUBLISH request must contain Count file names in total with exactly
Count NULL characters. Count must be in network byte order. You may assume each filename is at most
100 B (including NULL). No unused bytes are allowed between file names. A PUBLISH request will be no
larger than 1200 B.
For example, if a peer PUBLISHed the two files ”a.txt” and ”B.pdf” then the raw PUBLISH request
would be:
0x01 0x00 0x00 0x00 0x02 0x61 0x2e 0x74 0x78 0x74 0x00 0x42 0x2e 0x70 0x64 0x66 0x00*/
	}
	






	/*
	send(s, strang, sizeof(strang), 0); /* CHECK FOR ERRORS HERE */
	if (errno != 0) {
		printf("Some error occured while sending data request\n");
		return -1;
	}

	int occurrences = 0;
	bool errsv = false;
	std::string data;
	while (num > 0) {
		num = recv(s, buf, chunksize, 0);
		tot = tot + num;
		data = buf;
		
		if (num < 0) {
			errsv = true;
		}
		
   		std::string::size_type pos = 0;
   		std::string target = "<p>";
   		while ((pos = data.find(target, pos )) != std::string::npos) { /* CHECK FOR ERRORS*/
   		       ++ occurrences;
   		       pos += target.length();
   		}
		if (errno != 0) {
			printf("Some error occured while parsing the data\n");
			return -1;
		}
	}

	if (errsv == true) {
		printf("Some error occured while recieving the data, please try again.\n");
		return -1;
	}

	printf("Number of <p> tags: %i \n", occurrences);
	printf("Number of bytes:    %d \n", tot); */

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
