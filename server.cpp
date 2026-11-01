#include "general.h"

#include <netinet/in.h>
#include <string.h>
#include <poll.h>

#include <iostream>
#include <csignal>
#include <fstream>

using namespace std;

bool exitRequested{false};
void SignalHandler(int signal) {
    exitRequested = true;
}

int main(int argc, char *argv[]) {
    ProgramParameters programParameters;
    programParameters
        .AddParameter("help", "show parameters", false)
        .AddParameter("port", "port to listen to", true)
        .AddParameter("log", "log file", true);

    string parametersError;
    bool goodParameters = programParameters.ParseParameters(argc, argv, parametersError );

    if(argc == 1 || programParameters.IsParameterDefined("help")) {
        cout << programParameters.GetInfo();
        return 0;
    }

    if(!goodParameters) {
        cerr << "Wrong parameters: " << parametersError << endl;
        return -1;
    }

    SocketHolder socket;
    if(socket.fd == -1) {
        cerr << "Could not create a socket: " << strerror(errno) << endl;
        return -1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY; 
    serverAddress.sin_port = htons(stoi(programParameters.GetParameter("port")));
    if(bind(socket.fd, (const sockaddr*)&serverAddress, sizeof(serverAddress))) { 
        cerr << "Could not bind a socket: " << strerror(errno) << endl;
        return -1;
    }

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    unsigned long long packetsCounter{0};
    char buffer[1024];
    sockaddr_in clientAddress;
    unsigned clientAddressLength = sizeof(clientAddress);
    pollfd pollFd{socket.fd, POLLIN};
    while(!exitRequested) {
        if(poll(&pollFd, 1, 500) <= 0) {
            continue;
        }

        int receiveStatus = recvfrom(
            socket.fd,
            buffer, sizeof(buffer),
            MSG_WAITALL,
            (sockaddr*)&clientAddress, &clientAddressLength); 

        if(receiveStatus != -1) {
            packetsCounter++;
        }
        else {
            cerr << "Receive error: " << strerror(errno) << endl;
        }

        const string answer{to_string(packetsCounter)};
        if(sendto(socket.fd, answer.c_str(), answer.length(), MSG_CONFIRM, (sockaddr *)&clientAddress, sizeof(clientAddress)) == -1) {
            cerr << "Could not send a packet: " << strerror(errno) << endl;
        }
    }

    ofstream logFile(programParameters.GetParameter("log"), ios::app);
    if(logFile.is_open()) {
        logFile << to_string(packetsCounter) << endl;
        cout << "Total number of packets received: " << packetsCounter << endl;
    }
    else {
        cerr << "Could not write to the log" << endl;
    }

    return 0;
}