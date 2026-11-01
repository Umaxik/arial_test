#include "general.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

void FillPacket(vector<byte>& packet) {
    const string message{"trololo!"};
    const auto messageSize{message.length()};
    int counter;
    const auto packetSize{packet.size()};
    for(counter = 0; counter < packetSize; counter++) {
        packet[counter] = static_cast<byte>(message[counter % messageSize]);
    }
}

int main(int argc, char *argv[]) {
    ProgramParameters programParameters;
    programParameters
        .AddParameter("help", "show parameters", false)
        .AddParameter("host", "address of the server", true)
        .AddParameter("port", "port of the server", true)
        .AddParameter("count", "number of packets to send", true)
        .AddParameter("size", "size of one packet, bytes (default: 64)", false)
        .AddParameter("interval", "interval between packets, ms (default: 0)", false);

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
    serverAddress.sin_port   = htons(stoi(programParameters.GetParameter("port")));
    serverAddress.sin_addr.s_addr = inet_addr(programParameters.GetParameter("host").c_str());

    const int packetSize{programParameters.IsParameterDefined("size") ? stoi(programParameters.GetParameter("size")) : 64};
    const int interval{programParameters.IsParameterDefined("interval") ? stoi(programParameters.GetParameter("interval")) : 0};
    if(!packetSize) {
        cerr << "Size of packet cannot be zero" << endl;
        return -1;
    }

    vector<byte> packet(packetSize);
    FillPacket(packet);
  
    char buffer[1024];
    unsigned long long serverCounter;
    unsigned serverAddressLength = sizeof(serverAddress);

    int packetsCounter{stoi(programParameters.GetParameter("count"))};
    if(!packetsCounter) {
        cerr << "Nothing to do: count is zero" << endl;
        return -1;
    }

    for(; packetsCounter; packetsCounter--) {
        if(sendto(socket.fd, packet.data(), packet.size(), MSG_CONFIRM, (sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
            cout << "Could not send a packet: " << strerror(errno) << endl;
            return -1;
        }

        int receiveStatus = recvfrom(
            socket.fd,
            buffer, sizeof(buffer) - 1,
            MSG_WAITALL,
            (sockaddr*)&serverAddress, &serverAddressLength);
        if(receiveStatus != -1) {
            buffer[receiveStatus] = 0;
            serverCounter = stoull(buffer);
        }
        else {
            cerr << "Receive error: " << strerror(errno) << endl;
            return -1;
        }

        this_thread::sleep_for(chrono::milliseconds(interval));
    }

    cout << "Counter at server: " << serverCounter << endl;    

    return 0;
}