#include <sys/select.h>
#include <sys/socket.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

using namespace std;

int main() {
    while (true) {
        int selectr;
        string clear;
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        FD_SET(0, &readfds);

        selectr = select(2, &readfds, NULL, NULL, &tv);
        if (selectr == 0) {
            cout << endl << "No Input" << endl << endl;
            cout << "Select return: " << selectr << endl;
            cout << tv.tv_sec << "." << tv.tv_usec << endl;
            cout << "Stdin in readfds? " << FD_ISSET(0, &readfds) << endl;
            break;
        }

        cout << endl << "Input" << endl << endl;
        cout << "Select return: " << selectr << endl;
        cout << tv.tv_sec << "." << tv.tv_usec << endl;
        cout << "Stdin in readfds? " << FD_ISSET(0, &readfds) << endl;

        cin >> clear;
    }
    return 0;
}