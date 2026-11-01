#include "general.h"

#include <utility>
#include <sstream>
#include <string_view>

using namespace std;

ProgramParameters& ProgramParameters::AddParameter(const char* name, const char* description, bool isRequired) {
    knownParameters.emplace(make_pair(name, ParameterInfo{name, description, isRequired}));
    return *this;
}

bool ProgramParameters::ParseParameters(int argc, char *argv[], string& error) {
    parsedParameters.clear();

    int index;
    string currentParameter;
    for(index = 1; index < argc; index++) {
        string_view word{argv[index]};

        if(word.starts_with("--")) {
            currentParameter = word.substr(2);
            if(parsedParameters.contains(currentParameter)) {
                error = string{"duplicated parameter: "} + currentParameter;
                return false;
            }
            if(!knownParameters.contains(currentParameter)) {
                error = string{"unknown parameter: "} + currentParameter;
                return false;
            }

            parsedParameters[currentParameter];
        }
        else {
            if(!currentParameter.empty()) {
                parsedParameters[currentParameter] = word;
                currentParameter.clear();
            }
            else {
                error = string{"cannot understand parameter: "} + string{word};
                return false;
            }
        }
    }

    for(const auto& parameter : knownParameters) {
        if(parameter.second.isRequired && !parsedParameters.contains(parameter.first)) {
            error = string{"parameter should be defined: "} + parameter.first;
            return false;
        }
    }

    return true;
}

string ProgramParameters::GetInfo() const {
    stringstream result;

    result << "Possible parameters:" << endl;

    for(const auto& parameter : knownParameters) {
        result
            << "    --" << parameter.second.name << ": "
            << (parameter.second.isRequired ? "required, " : "")
            << parameter.second.description
            << endl;
    }

    return result.str();
}

bool ProgramParameters::IsParameterDefined(const string& name) const {
    return parsedParameters.contains(name);
}

string ProgramParameters::GetParameter(const string&name) const {
    return parsedParameters.contains(name) ? parsedParameters.at(name) : "";
}
