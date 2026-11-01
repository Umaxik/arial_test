#include <map>
#include <string>

#include <sys/socket.h> 
#include <unistd.h>

using namespace std;

class ProgramParameters {
public:
    struct ParameterInfo {
        ParameterInfo(const char* name, const char* description, bool isRequired)
        :   name{name},
            description{description},
            isRequired{isRequired}
        {}

        const char* name;
        const char* description;
        bool isRequired;
    };

    ProgramParameters& AddParameter(const char* name, const char* descriptoin, bool isRequired);
    bool ParseParameters(int argc, char *argv[], string& error);

    string GetInfo() const;
    bool IsParameterDefined(const string& name) const;
    string GetParameter(const string&name) const;

private:
    map<string, ParameterInfo> knownParameters;
    map<string, string> parsedParameters;
};

struct SocketHolder {
    SocketHolder()
        : fd{socket(AF_INET, SOCK_DGRAM, 0)}
    {}

    ~SocketHolder() {
        if(fd != -1) {
            close(fd);
        }
    }

    const int fd;
};
