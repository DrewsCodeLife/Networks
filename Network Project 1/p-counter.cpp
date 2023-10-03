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
	int num,tot = 0;
	int chunksize = 0;
	char buf[2048];
	const char *host = "www.ecst.csuchico.edu";
	const char *port = "80";
	char strang[] = "GET /~kkredo/file.html HTTP/1.0\r\n\r\n";

	if(errno != 0) {
		errno=0;
	}

	if (argc < 2) {
		std::cout << stderr << " Please provide a valid chunk size" << std::endl;
	} else {
		chunksize = std::atoi(argv[1]);
	}

	/* Lookup IP and connect to server */
	if ( ( s = lookup_and_connect( host, port ) ) < 0 ) {
		exit( 1 );
	}

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
	printf("Number of bytes:    %d \n", tot);

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
