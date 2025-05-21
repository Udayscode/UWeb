#ifndef TEMPLATE_HPP
#define TEMPLATE_HPP

#include <string>
#include <map>
#include <vector>
#include <regex>
#include <fstream>
#include <sstream>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

class Template {
private:
    string templateContent;
    const string templateDir = "./templates/";

    string replaceVariables(string content, const json& data) {
        regex pattern("\\{\\{\\s*([a-zA-Z0-9_\\>]+)\\s*\\}\\}");
        string result = content;
        smatch matches;

        string::const_iterator searchStart(content.cbegin());
        while (regex_search(searchStart, content.cend(), matches, pattern)) {
            string varName = matches[1].str();
            string replacement = "";

            if (varName.find('.') != string::npos) {
                size_t dotPos = varName.find('.');
                string objName = varName.substr(0, dotPos);
                string propName = varName.substr(dotPos + 1);

                if (data.contains(objName) && data[objName].is_object() && data.contains(propName)) {
                    replacement = data[objName][propName].get<string>();
                }
            } else if (data.contains(varName)) {
                if (data[varName].is_string()) {
                    replacement = data[varName].get<string>();
                } else if (data[varName].is_number()) {
                    replacement = data[varName].get<int>();
                } else if (data[varName].is_boolean()) {
                    replacement = data[varName].get<bool>() ? "true" : "false";
                }
            }

            result = regex_replace(result, regex("\\{\\{\\s*" + varName + "\\s*\\}\\}"),replacement);
            searchStart = matches.suffix().first;
        }

        return result;
    }

    string processConditionals(string content, const json& data) {
        regex ifPattern("\\{\\%\\s*if\\s+([a-zA-Z0-9_\\.]+)\\s*\\%\\}([\\s\\S]*?)\\{\\%\\s*endif\\s*\\%\\}");
        string result = content;
        smatch matches;

        string::const_iterator searchStart(content.cbegin());
        while (regex_search(searchStart, content.cend(), matches, ifPattern)) {
            string condition = matches[1].str();
            string ifContent = matches[2].str();
            bool conditionMet = false;

            if (data.contains(condition)) {
                if (data[condition].is_boolean()) {
                    conditionMet = data[condition].get<bool>();
                } else if (data[condition].is_string()) {
                    conditionMet = !data[condition].get<string>().empty();
                } else if (data[condition].is_number()) {
                    conditionMet = data[condition].get<int>() != 0;
                } else if (data[condition].is_array() || data[condition].is_object()) {
                    conditionMet = !data[condition].empty();
                }
            }

            string replacement = conditionMet ? ifContent : "";
            result = regex_replace(result, regex("\\{\\%\\s*if\\s+" + condition + "\\s*\\%\\}[\\s\\S]*?\\{\\%\\s*endif\\s*\\%\\}"), replacement);
            
            searchStart = matches.suffix().first;
        }

        return result;
    }

    string processLoops(string content, const json& data) {
        regex forPattern("\\{\\%\\s*for\\s+([a-zA-Z0-9_]+)\\s+in\\s+([a-zA-Z0-9_]+)\\s*\\%\\}([\\s\\S]*?)\\{\\%\\s*endfor\\s*\\%\\}");
        string result = content;
        smatch matches;

        string::const_iterator searchStart(content.cbegin());
        while (regex_search(searchStart, content.cend(), matches, forPattern)) {
            string itemName = matches[1].str();
            string arrayName = matches[2].str();
            string loopContent = matches[3].str();
            string replacement = "";

            if (data.contains(arrayName) && data[arrayName].is_array()) {
                for (const auto& item : data[arrayName]) {
                    string itemContent  = loopContent;
                    json loopData = data;
                    loopData[itemName] = item;

                    itemContent = replaceVariables(itemContent, loopData);
                    replacement += itemContent;
                }
            }

            result = regex_replace(result, regex("\\{\\%\\s*for\\s+" + itemName + "\\s+in\\s+" + arrayName + "\\s*\\%\\}[\\s\\S]*?\\{\\%\\s*endfor\\s*\\%\\}"), replacement);

            searchStart = matches.suffix().first;
        }

        return result;
    }
    string processIncludes(string content) {
        regex includePattern("\\{\\%\\s*include\\s+\"([a-zA-Z0-9_\\./]+)\"\\s*\\%\\}");
        string result = content;
        smatch matches;

        string::const_iterator searchStart(content.cbegin());
        while (regex_search(searchStart, content.cend(), matches, includePattern)) {
            string filename = matches[1].str();
            string includeContent = "";

            string includePath = templateDir + filename;
            ifstream includeFile(includePath);
            if (includeFile.is_open()) {
                stringstream buffer;
                buffer << includeFile.rdbuf();
                includeContent = buffer.str();
                includeFile.close(); 
            } else {
                cerr << "Warning: Include file not found: " << includePath << endl;
            }

            result = regex_replace(result, regex("\\{\\%\\s*include\\s+\"" + filename + "\"\\s*\\%\\}"), includeContent);
            searchStart = matches.suffix().first;
        }

        return result;
    }

public:
    Template(const string& filename) {
        loadFromFile(filename);
    }

    Template(const string& content, bool isString) {
        if (isString) {
            templateContent = content;
        } else {
            loadFromFile(content);
        }
    }

    void loadFromFile(const string& filename) {
        string path = templateDir + filename;
        ifstream file(path);

        if (!file.is_open()) {
            cerr << "Error: Could not open template file: " << path << endl;
            return;
        }

        stringstream buffer;
        buffer << file.rdbuf();
        templateContent = buffer.str();
        file.close();
    }

    string render(const json& data) {
        string result = templateContent;

        result = processIncludes(result);
        result = processConditionals(result, data);
        result = processLoops(result, data);
        result = replaceVariables(result, data);
        
        return result;
    }
};

#endif