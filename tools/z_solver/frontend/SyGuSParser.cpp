#include "SyGuSParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <stack>
#include <algorithm>

// Read the entire file into a string
std::string SyGuSParser::readFileToString(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Tokenize the input string, preserving parentheses and handling S-expressions
std::vector<std::string> SyGuSParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string currentToken;
    bool inString = false;
    int parenDepth = 0;
    
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        
        // Handle string literals
        if (c == '"') {
            inString = !inString;
            currentToken += c;
            continue;
        }
        
        if (inString) {
            currentToken += c;
            continue;
        }
        
        // Handle parentheses
        if (c == '(') {
            if (parenDepth == 0 && !currentToken.empty()) {
                tokens.push_back(currentToken);
                currentToken.clear();
            }
            currentToken += c;
            parenDepth++;
        } else if (c == ')') {
            currentToken += c;
            parenDepth--;
            if (parenDepth == 0) {
                tokens.push_back(currentToken);
                currentToken.clear();
            }
        } else if (std::isspace(c)) {
            if (parenDepth == 0 && !currentToken.empty()) {
                tokens.push_back(currentToken);
                currentToken.clear();
            } else if (parenDepth > 0) {
                currentToken += c;
            }
        } else {
            currentToken += c;
        }
    }
    
    if (!currentToken.empty()) {
        tokens.push_back(currentToken);
    }
    
    return tokens;
}

// Extract the logic from the SyGuS file
std::string SyGuSParser::extractLogic(const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        if (token.find("(set-logic") == 0) {
            std::regex logicRegex(R"(\(set-logic\s+([A-Z]+)\))");
            std::smatch match;
            if (std::regex_search(token, match, logicRegex) && match.size() > 1) {
                return match[1].str();
            }
        }
    }
    return "LIA"; // Default to LIA if not found
}

// Extract the invariant function name
std::string SyGuSParser::extractInvName(const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        if (token.find("(synth-inv") == 0) {
            std::regex invRegex(R"(\(synth-inv\s+([a-zA-Z0-9_-]+))");
            std::smatch match;
            if (std::regex_search(token, match, invRegex) && match.size() > 1) {
                return match[1].str();
            }
        }
    }
    throw std::runtime_error("Invariant function name not found");
}

// Extract the invariant function parameters
std::vector<SyGuSParser::VariableInfo> SyGuSParser::extractInvParams(const std::vector<std::string>& tokens) {
    std::vector<VariableInfo> params;
    
    for (const auto& token : tokens) {
        if (token.find("(synth-inv") == 0) {
            // Extract the parameter list
            size_t start = token.find('(', token.find('(') + 1);
            size_t end = token.rfind(')');
            
            if (start != std::string::npos && end != std::string::npos && start < end) {
                std::string paramList = token.substr(start, end - start + 1);
                
                // Parse the parameters
                std::regex paramRegex(R"(\(([a-zA-Z0-9_-]+)\s+([a-zA-Z]+)\))");
                std::sregex_iterator it(paramList.begin(), paramList.end(), paramRegex);
                std::sregex_iterator end;
                
                while (it != end) {
                    std::smatch match = *it;
                    if (match.size() > 2) {
                        VariableInfo var;
                        var.name = match[1].str();
                        var.type = match[2].str();
                        params.push_back(var);
                    }
                    ++it;
                }
            }
            break;
        }
    }
    
    return params;
}

// Extract a function definition (pre, trans, or post)
SyGuSParser::FunctionInfo SyGuSParser::extractFunction(const std::vector<std::string>& tokens, const std::string& funcType) {
    FunctionInfo func;
    std::string funcPrefix;
    
    // Handle different function name patterns
    if (funcType == "pre") {
        funcPrefix = "(define-fun pre";
    } else if (funcType == "trans") {
        funcPrefix = "(define-fun trans";
    } else if (funcType == "post") {
        funcPrefix = "(define-fun post";
    }
    
    for (const auto& token : tokens) {
        if (token.find(funcPrefix) == 0) {
            // Extract function name
            std::regex nameRegex(R"(\(define-fun\s+([a-zA-Z0-9_-]+))");
            std::smatch nameMatch;
            if (std::regex_search(token, nameMatch, nameRegex) && nameMatch.size() > 1) {
                func.name = nameMatch[1].str();
            }
            
            // Find the return type position (after the parameter list)
            size_t paramStart = token.find('(', token.find(func.name) + func.name.length());
            size_t paramEnd = token.find(')', paramStart);
            
            if (paramStart != std::string::npos && paramEnd != std::string::npos) {
                std::string paramList = token.substr(paramStart, paramEnd - paramStart + 1);
                
                // Parse the parameters
                std::regex paramRegex(R"(\(([a-zA-Z0-9_-]+)\s+([a-zA-Z]+)\))");
                std::sregex_iterator it(paramList.begin(), paramList.end(), paramRegex);
                std::sregex_iterator end;
                
                while (it != end) {
                    std::smatch match = *it;
                    if (match.size() > 2) {
                        VariableInfo var;
                        var.name = match[1].str();
                        var.type = match[2].str();
                        func.params.push_back(var);
                    }
                    ++it;
                }
            }
            
            // Find the return type and function body
            size_t returnTypePos = token.find("Bool", paramEnd);
            if (returnTypePos != std::string::npos) {
                // Find the actual body after the return type
                size_t bodyStart = token.find('(', returnTypePos);
                if (bodyStart != std::string::npos) {
                    // Extract everything from the body start to the end of the function definition
                    // (accounting for nested parentheses)
                    int depth = 1;
                    size_t bodyEnd = bodyStart + 1;
                    
                    while (depth > 0 && bodyEnd < token.length()) {
                        if (token[bodyEnd] == '(') {
                            depth++;
                        } else if (token[bodyEnd] == ')') {
                            depth--;
                        }
                        bodyEnd++;
                    }
                    
                    if (depth == 0) {
                        func.body = token.substr(bodyStart, bodyEnd - bodyStart);
                    }
                }
            }
            
            break;
        }
    }
    
    return func;
}

// Generate CHC constraints from the parsed SyGuS components
std::string SyGuSParser::generateCHC(const std::string& logic, const std::string& invName, 
                                    const std::vector<VariableInfo>& invParams,
                                    const FunctionInfo& preFunc, 
                                    const FunctionInfo& transFunc, 
                                    const FunctionInfo& postFunc) {
    std::stringstream chc;
    
    // Set logic
    chc << "(set-logic " << logic << ")" << std::endl << std::endl;
    
    // Declare the invariant function
    chc << "(declare-fun " << invName << " (";
    for (const auto& param : invParams) {
        chc << param.type << " ";
    }
    chc << ") Bool)" << std::endl << std::endl;
    
    // Generate the initial state constraint (pre -> inv)
    chc << ";; Initial state constraint (pre -> inv)" << std::endl;
    chc << "(assert (forall (";
    for (const auto& param : invParams) {
        chc << "(" << param.name << " " << param.type << ") ";
    }
    chc << ")" << std::endl;
    chc << "    (=> " << preFunc.body << std::endl;
    chc << "        (" << invName << " ";
    for (const auto& param : invParams) {
        chc << param.name << " ";
    }
    chc << "))))" << std::endl << std::endl;
    
    // Generate the transition constraint (inv /\ trans -> inv')
    chc << ";; Transition constraint (inv /\\ trans -> inv')" << std::endl;
    chc << "(assert (forall (";
    
    // Original variables
    for (const auto& param : invParams) {
        chc << "(" << param.name << " " << param.type << ") ";
    }
    
    // Next state variables
    for (const auto& param : invParams) {
        chc << "(" << param.name << "! " << param.type << ") ";
    }
    
    chc << ")" << std::endl;
    chc << "    (=> (and (" << invName << " ";
    
    // Original state in invariant
    for (const auto& param : invParams) {
        chc << param.name << " ";
    }
    chc << ")" << std::endl;
    
    // Transition relation
    chc << "            " << transFunc.body << ")" << std::endl;
    
    // Next state in invariant
    chc << "        (" << invName << " ";
    for (const auto& param : invParams) {
        chc << param.name << "! ";
    }
    chc << "))))" << std::endl << std::endl;
    
    // Generate the property constraint (inv -> post)
    chc << ";; Property constraint (inv -> post)" << std::endl;
    chc << "(assert (forall (";
    for (const auto& param : invParams) {
        chc << "(" << param.name << " " << param.type << ") ";
    }
    chc << ")" << std::endl;
    chc << "    (=> (" << invName << " ";
    for (const auto& param : invParams) {
        chc << param.name << " ";
    }
    chc << ")" << std::endl;
    chc << "        " << postFunc.body << ")))" << std::endl << std::endl;
    
    // Add check-sat command
    chc << "(check-sat)" << std::endl;
    
    return chc.str();
}

// Main function to transform SyGuS to CHC
void SyGuSParser::sygus_to_chc(const std::string& file_in, const std::string& file_out) {
    try {
        // Read the input file
        std::string content = readFileToString(file_in);
        
        // Tokenize the content
        std::vector<std::string> tokens = tokenize(content);
        
        // Extract components
        std::string logic = extractLogic(tokens);
        std::string invName = extractInvName(tokens);
        std::vector<VariableInfo> invParams = extractInvParams(tokens);
        
        // Extract function definitions
        FunctionInfo preFunc = extractFunction(tokens, "pre");
        FunctionInfo transFunc = extractFunction(tokens, "trans");
        FunctionInfo postFunc = extractFunction(tokens, "post");
        
        // Generate CHC constraints
        std::string chcContent = generateCHC(logic, invName, invParams, preFunc, transFunc, postFunc);
        
        // Write to output file
        std::ofstream outFile(file_out);
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open output file: " + file_out);
        }
        
        outFile << chcContent;
        outFile.close();
        
        std::cout << "Conversion completed: " << file_in << " -> " << file_out << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
