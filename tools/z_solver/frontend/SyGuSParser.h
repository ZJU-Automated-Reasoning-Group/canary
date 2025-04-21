#ifndef SYGUS_PARSER_H
#define SYGUS_PARSER_H

#include <string>
#include <vector>
#include <map>

/**
 * @class SyGuSParser
 * @brief Parser for transforming SyGuS (Syntax-Guided Synthesis) format to CHC (Constrained Horn Clauses) format.
 * 
 * This parser handles the transformation of SyGuS invariant synthesis problems to CHC format.
 * It parses the pre-condition, transition relation, and post-condition functions from the SyGuS file
 * and generates the corresponding CHC constraints in SMT2 format.
 */
class SyGuSParser {
public:
    /**
     * @brief Transforms a SyGuS file to CHC format
     * 
     * @param file_in Path to the input SyGuS file
     * @param file_out Path to the output CHC file in SMT2 format
     */
    static void sygus_to_chc(const std::string& file_in, const std::string& file_out);

private:
    // Structure to store variable information
    struct VariableInfo {
        std::string name;
        std::string type;
    };

    // Structure to store function information
    struct FunctionInfo {
        std::string name;
        std::vector<VariableInfo> params;
        std::string body;
    };

    // Helper methods for parsing
    static std::string readFileToString(const std::string& filePath);
    static std::vector<std::string> tokenize(const std::string& input);
    static std::string extractLogic(const std::vector<std::string>& tokens);
    static std::string extractInvName(const std::vector<std::string>& tokens);
    static std::vector<VariableInfo> extractInvParams(const std::vector<std::string>& tokens);
    static FunctionInfo extractFunction(const std::vector<std::string>& tokens, const std::string& funcType);
    static std::string generateCHC(const std::string& logic, const std::string& invName, 
                                  const std::vector<VariableInfo>& invParams,
                                  const FunctionInfo& preFunc, 
                                  const FunctionInfo& transFunc, 
                                  const FunctionInfo& postFunc);
};

#endif // SYGUS_PARSER_H
