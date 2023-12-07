// Developers    : Drew Mortenson & Christopher M. Claire
// Class info    : 2238-EECE-446-01-2992
// Semester      : Fall 2023
// Functionality : This program acts as the registry end of the previously developed peer programs.
//                      The registry will only be capable of listening and responding to JOIN,
//                      PUBLISH, and SEARCH requests. Ideally the registry will be capable of connecting
//                      an arbitrary number of peers, but no more than 5 is expected.
//                  The following assumptions have been made:
//                  (1) Peers will only ever attempt to PUBLISH after they have JOIN'd
//                  (2) A peer will only ever PUBLISH once
//                  (3) Publish requests will never contain more than 10 file names
//                  (4) File names will never exceed 100 characters (plus null termination)
//                  (5) When responding to a SEARCH, if multiple peers offer the file than any choice is good.

// This program contains code provided in the Lab 10 "Section 3" file. All code used is noted as such.
// Main function pulled inspiration from Lab 10 "Section 3" file but has been re-written to fit this project.
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <vector>

using namespace std;

#define MAX_PENDING 5

struct peerinfo {
    uint32_t ID;
    int fd = -1; // Socket descriptor
    char files[10][101];
    struct sockaddr_in address;
	char buffer[INET_ADDRSTRLEN];
};
static peerinfo connections[65537]; // Peer file descriptor = index in this array + 3

// START OF CODE FROM LAB 10 "Section 3" FILE

/*
 * Create, bind and passive open a socket on a local interface for the provided service.
 * Argument matches the second argument to getaddrinfo(3).
 *
 * Returns a passively opened socket or -1 on error. Caller is responsible for calling
 * accept and closing the socket.
 */
int bind_and_listen( const char *service );

/*
 * Return the maximum socket descriptor set in the argument.
 * This is a helper function that might be useful to you.
 */
int find_max_fd(const fd_set *fs);

// END OF CODE FROM LAB 10 "Section 3" FILE

int main(int argc, char *argv[]) {
	if(argc < 2) {
		cout << "Expected 2 arguments, received: " << argc << endl;
		cout << "Usage: ./p4_registry <PORT_NUMBER> (1024 <--> 65353)" << endl;
		return -1;
	}
	const char *port = argv[1];
	int check = atoi(port);
    if(check < 1024 || check > 65353) {
        cout << "Usage   : ./p4_registry <PORT_NUMBER> (1024 <--> 65353)" << endl;
		cout << "Received: ./p4_registry " << port << endl;
        return 1;
    }

    fd_set all_socks;   // Instantiate set of socket file descriptors to track all active sockets
    FD_ZERO(&all_socks); // Define set as empty
    fd_set call_socks;  // Call set will temporarily hold sockets for select(). Tracks status of sockets.
    FD_ZERO(&call_socks);

    int listen_sock = bind_and_listen(port);
    FD_SET(listen_sock, &all_socks);

    int max_sock = listen_sock;

    while(1) {
		char buf[8192];
        call_socks = all_socks;
        int new_s = select(max_sock + 1, &call_socks, NULL, NULL, NULL);
        if (new_s < 0) {
            perror("Error in call to select() function");
            return -1;
        }

        // Now we will run through all of the 'ready' sockets, ignoring 0-2 (IN/OUT/ERROR)
        for (int s = 3; s <= max_sock; ++s) {
            if (!FD_ISSET(s, &call_socks)) {
                continue; // if s isn't ready, jump to next loop
            }

            if (s == listen_sock) { // New peer connecting
				int new_s = accept(s, NULL, NULL);
				FD_SET(new_s, &all_socks);
				max_sock = find_max_fd(&all_socks);

				connections[new_s + 3].fd = new_s;
				struct sockaddr_in addr;
				socklen_t len = sizeof(addr);
				int ret = getpeername(new_s, (struct sockaddr*)&addr, &len);
				if (ret != 0) {
					perror("Error receiving peer connection data");
					return -1;
				}
				connections[new_s + 3].address.sin_addr.s_addr = ntohl(addr.sin_addr.s_addr);
				connections[new_s + 3].address.sin_port = ntohs(addr.sin_port);

            } else { // pre-connected peer requesting something
				int received = 0;

				received = recv(s, buf, 1, 0);
				int action = static_cast<int>(buf[0]);

				if (received == 0) { // Peer disconnected
					FD_CLR(s, &all_socks);
					close(s);
				} else if (action == 0) { // Join
					uint32_t tempID = 0;
					int ret = recv(s, &tempID, 4, 0);
					if (ret == 0) {
						cout << "JOIN recv() FAILURE" << endl;
						return -1;
					}
					connections[s + 3].ID = ntohl(tempID);
					cout << "TEST] JOIN " << connections[s+3].ID << endl;
				} else if(action == 1) { // Publish
					uint32_t count = 0;
					int ret = 0;
					
					if ((ret = recv(s, &count, 4, 0)) < 0) {
						cout << "Publish file count recv error" << endl;
						return -1;
					}
					count = ntohl(count);

					if ((ret = recv(s, buf, 1200, 0)) < 0) {
						cout << "Publish file names recv error" << endl;
					}

					int j = 0;
					int n = 0;
					for(int i = 0; i<10;i++) {
						for(j=0; buf[n] != '\0'; j++) {
							connections[s + 3].files[i][j] = buf[n];
							n++;
						}
						connections[s + 3].files[i][j] = buf[n];
						n++;

						if(buf[n] == '\0' && buf[n-1] == '\0') { // Should only be true once if count < 10
							break;
						}
					}

					cout << "TEST] PUBLISH " << count << " ";
					for(n = 0; n < 10; n++) {
						cout << connections[s+3].files[n] << " ";
					}
					cout << endl;
				} else if (action == 2) {
					char buffy[100];
					// SEARCH command
					// Receive file name
					// loop through all connections[x + 3] and strcmp(connections[x+3].files[i], 'file name')
					// When strcmp returns 0, we've found our match, simply package and send() data to
					//		requesting peer.
					if ((received = recv(s, &buffy, 100, 0)) < 0) {
						perror("Search receive error");
						return -1;
					}

					bool correct = false;
					int n;
					for (n = 0; n < 65537; n++) {
						if (connections[n].fd != -1) {
							for (int i = 0; i < 10; i++) {
								if (strcmp(connections[n].files[i],buffy) == 0) {
									correct = true;
								}
								if (correct == true) {
									break;
								}
							}
							if (correct == true) {
								break;
							}
							cout << n << endl;
						}
					}
					if (correct == true) {
						vector<char> peer;
						peer.push_back(htonl(connections[n].ID));
						peer.push_back(htonl(connections[n].address.sin_addr.s_addr));
						peer.push_back(htons(connections[n].address.sin_port));
						cout << connections[n].ID << endl << connections[n].address.sin_addr.s_addr << ":" << endl;
						cout << connections[n].address.sin_port << endl;

						char* sendData = peer.data();
						int sendSize = sizeof(peer);
						if ((received = send(s, sendData, sendSize, 0)) < 0) {
							perror("Search response fail");
							return -1;
						}
						cout << "Sent " << received << " bytes" << endl;
					} else {
						// Send all zero's
					}
					}
				}
            }
        }

    return 0;
}

// START OF CODE FROM LAB 10 "Section 3" FILE

int bind_and_listen( const char *service ) {
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	/* Build address data structure */
	memset( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	/* Get local address info */
	if ( ( s = getaddrinfo( NULL, service, &hints, &result ) ) != 0 ) {
		fprintf( stderr, "stream-talk-server: getaddrinfo: %s\n", gai_strerror( s ) );
		return -1;
	}

	/* Iterate through the address list and try to perform passive open */
	for ( rp = result; rp != NULL; rp = rp->ai_next ) {
		if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
			continue;
		}

		if ( !bind( s, rp->ai_addr, rp->ai_addrlen ) ) {
			break;
		}

		close( s );
	}
	if ( rp == NULL ) {
		perror( "stream-talk-server: bind" );
		return -1;
	}
	if ( listen( s, MAX_PENDING ) == -1 ) {
		perror( "stream-talk-server: listen" );
		close( s );
		return -1;
	}
	freeaddrinfo( result );

	return s;
}

int find_max_fd(const fd_set *fs) {
	int ret = 0;
	for(int i = FD_SETSIZE-1; i>=0 && ret==0; --i){
		if( FD_ISSET(i, fs) ){
			ret = i;
		}
	}
	return ret;
}

// END OF CODE FROM LAB 10 "Section 3" FILE