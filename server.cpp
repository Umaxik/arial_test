#include "general.h"

#include <netinet/in.h>
#include <string.h>
#include <poll.h>
#include <syslog.h>

#include <iostream>
#include <csignal>
#include <fstream>
#include <chrono>
#include <sstream>

using namespace std;

bool exitRequested{false};
void SignalHandler(int signal) {
    exitRequested = true;
}

class MessagesManager {
public:
    class Sink {
    public:
        Sink(bool errors, bool daemon)
        :   errors(errors),
            daemon(daemon)
        {}

        ~Sink() {
            if(daemon) {
                syslog(errors ? LOG_ERR : LOG_INFO, "%s", buffer.str().c_str());
            } else {
                (errors ? cerr : cout) << buffer.str() << endl;
            }
        }

        template<typename ValueType>
        Sink& operator << (const ValueType& value) {
            buffer << value;

            return *this;
        }
    private:
        bool errors;
        bool daemon;

        stringstream buffer;
    };

    MessagesManager()
    :   daemon{false}
    {}

    void SetDaemonized(bool daemon) {
        this->daemon = daemon;
    }

    Sink Error() {
        return Sink(true, daemon);
    }
    Sink Notification() {
        return Sink(false, daemon);
    }

private:
    bool daemon;
};

int main(int argc, char *argv[]) {
    MessagesManager messagesManager;

    ProgramParameters programParameters;
    programParameters
        .AddParameter("help", "show parameters", false)
        .AddParameter("port", "port to listen to", true)
        .AddParameter("log", "log file", true)
        .AddParameter("daemon", "daemonize the server", false);

    string parametersError;
    bool goodParameters = programParameters.ParseParameters(argc, argv, parametersError );

    if(argc == 1 || programParameters.IsParameterDefined("help")) {
        cout << programParameters.GetInfo();
        return 0;
    }

    if(!goodParameters) {
        messagesManager.Error() << "Wrong parameters: " << parametersError;
        return -1;
    }

    if(programParameters.IsParameterDefined("daemon")) {
        pid_t pid = fork();
        if (pid == -1) {
            messagesManager.Error() << "Demonization has failed: " << strerror(errno);
            return -1;
        }
        if (pid != 0) {
            messagesManager.Notification() << "The server is now being demonized";
            return 0;
        }

        messagesManager.SetDaemonized(true);
            
        if(setsid() == -1) {
            messagesManager.Error() << "Failed to create a session: " << strerror(errno);
            return -1;
        }

        pid = fork();
        if (pid == -1) {
            messagesManager.Error() << "Inner fork failed: " << strerror(errno);
            return -1;
        }
        if (pid != 0) {
            return 0;
        }
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
        messagesManager.Error() << "Could not bind a socket: " << strerror(errno);
        return -1;
    }

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    ofstream logFile(programParameters.GetParameter("log"), ios::app);
    if(!logFile.is_open()) {
        cerr << "Could not open the log" << endl;
        return -1;
    }

    logFile << chrono::system_clock::now() << " Server has started, port: " << programParameters.GetParameter("port") << endl;

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
            messagesManager.Error() << "Receive error: " << strerror(errno);
        }

        const string answer{to_string(packetsCounter)};
        if(sendto(socket.fd, answer.c_str(), answer.length(), MSG_CONFIRM, (sockaddr *)&clientAddress, sizeof(clientAddress)) == -1) {
            messagesManager.Error() << "Could not send a packet: " << strerror(errno);
        }
    }

    logFile << chrono::system_clock::now() << " Server is stopping, packets received: " << packetsCounter << endl;

    return 0;
}