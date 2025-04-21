#include"cParser.h"
#include<fstream>
#include<sstream>
#include<vector>
#include<string>
#include<iostream>
#include<map>
#include<regex>

// Structure to represent a branch condition and its associated statements
struct BranchInfo {
	std::string condition;
	std::vector<std::string> statements;
	std::vector<std::string> elseStatements;
	bool hasElse;
	
	BranchInfo() : hasElse(false) {}
};

void cParser::c_to_chc(std::string file_in, std::string file_out) {
	// read the file
	std::ifstream in(file_in);
	std::string line;
	
	// Data structures to store program information
	std::map<std::string, std::string> variables; // variable name -> type (only int in this case)
	std::vector<std::string> preconditions;       // assume statements before the loop
	std::vector<std::string> loopCondition;       // the loop condition
	std::vector<std::string> loopBody;            // statements inside the loop
	std::vector<BranchInfo> branches;             // branches (if statements) inside the loop
	std::vector<std::string> postconditions;      // assert statements after the loop
	
	bool inLoop = false;
	int loopBraceCount = 0;
	bool foundLoop = false;
	
	bool inBranch = false;
	bool inElseBranch = false;
	int branchBraceCount = 0;
	BranchInfo currentBranch;
	
	// Regular expressions for detecting annotations
	std::regex assumeRegex("__VERIFIER_assume\\s*\\((.+)\\)");
	std::regex assertRegex("assert\\s*\\((.+)\\)");
	std::regex nondetRegex("__VERIFIER_nondet_int\\(\\)");
	
	// Read the entire file into a string for easier processing
	std::string fileContent;
	while (std::getline(in, line)) {
		fileContent += line + "\n";
	}
	in.close();
	
	// Process the file content
	std::istringstream fileStream(fileContent);
	
	// First pass: collect variable declarations
	while (std::getline(fileStream, line)) {
		// Skip comments and empty lines
		if (line.empty() || line.find("//") == 0) {
			continue;
		}
		
		// Look for integer variable declarations
		std::istringstream ss(line);
		std::string token;
		
		while (ss >> token) {
			if (token == "int") {
				// Read the variable name
				ss >> token;
				
				// Handle multiple declarations and remove semicolons/commas
				std::string varName = token;
				if (varName.find(";") != std::string::npos) {
					varName = varName.substr(0, varName.find(";"));
				}
				if (varName.find(",") != std::string::npos) {
					varName = varName.substr(0, varName.find(","));
				}
				
				// Store the variable if it's not a function
				if (varName.find("(") == std::string::npos) {
					variables[varName] = "int";
				}
				
				// Check for more variables in the same declaration
				while (ss >> token) {
					if (token.find(",") != std::string::npos) {
						// Another variable in the list
						ss >> token;
						varName = token;
						if (varName.find(";") != std::string::npos) {
							varName = varName.substr(0, varName.find(";"));
						}
						if (varName.find(",") != std::string::npos) {
							varName = varName.substr(0, varName.find(","));
						}
						variables[varName] = "int";
					}
					if (token.find(";") != std::string::npos) {
						break; // End of declaration
					}
				}
			}
		}
	}
	
	// Reset the file stream for the second pass
	fileStream.clear();
	fileStream.seekg(0);
	
	// Second pass: identify loop, preconditions, and postconditions
	while (std::getline(fileStream, line)) {
		// Skip comments and empty lines
		if (line.empty() || line.find("//") == 0) {
			continue;
		}
		
		// Check for assume statements (preconditions)
		std::smatch assumeMatch;
		if (std::regex_search(line, assumeMatch, assumeRegex)) {
			if (!inLoop && !foundLoop) {
				// This is a precondition (before the loop)
				preconditions.push_back(assumeMatch[1]);
			}
		}
		
		// Check for assert statements (postconditions)
		std::smatch assertMatch;
		if (std::regex_search(line, assertMatch, assertRegex)) {
			if (foundLoop && !inLoop) {
				// This is a postcondition (after the loop)
				postconditions.push_back(assertMatch[1]);
			}
		}
		
		// Look for the loop
		if (!foundLoop && (line.find("while") != std::string::npos || line.find("for") != std::string::npos)) {
			foundLoop = true;
			inLoop = true;
			
			// Extract the loop condition
			size_t openParen = line.find("(");
			size_t closeParen = line.find_last_of(")");
			
			if (openParen != std::string::npos && closeParen != std::string::npos) {
				std::string condition = line.substr(openParen + 1, closeParen - openParen - 1);
				loopCondition.push_back(condition);
			}
			
			// Check if the loop body starts on this line
			if (line.find("{") != std::string::npos) {
				loopBraceCount++;
			}
		}
		// Inside the loop
		else if (inLoop) {
			// Count braces to determine when we exit the loop
			for (char c : line) {
				if (c == '{') loopBraceCount++;
				if (c == '}') loopBraceCount--;
			}
			
			// If we're still in the loop body
			if (loopBraceCount > 0) {
				// Check for if statements inside the loop
				if (!inBranch && line.find("if") != std::string::npos) {
					inBranch = true;
					currentBranch = BranchInfo();
					
					// Extract the branch condition
					size_t openParen = line.find("(");
					size_t closeParen = line.find_last_of(")");
					
					if (openParen != std::string::npos && closeParen != std::string::npos) {
						std::string condition = line.substr(openParen + 1, closeParen - openParen - 1);
						currentBranch.condition = condition;
					}
					
					// Check if the branch body starts on this line
					if (line.find("{") != std::string::npos) {
						branchBraceCount++;
					}
				}
				// Check for else statements
				else if (inBranch && branchBraceCount == 0 && line.find("else") != std::string::npos) {
					inElseBranch = true;
					currentBranch.hasElse = true;
					
					// Check if the else body starts on this line
					if (line.find("{") != std::string::npos) {
						branchBraceCount++;
					}
				}
				// Inside a branch
				else if (inBranch) {
					// Count braces to determine when we exit the branch
					for (char c : line) {
						if (c == '{') branchBraceCount++;
						if (c == '}') branchBraceCount--;
					}
					
					// If we're still in the branch body, add the statement
					if (branchBraceCount > 0) {
						// Skip the opening brace line
						if (line.find("{") == std::string::npos || line.find("{") > 0) {
							// Replace nondet calls with symbolic variables
							std::string processedLine = std::regex_replace(line, nondetRegex, "nondet");
							
							if (inElseBranch) {
								currentBranch.elseStatements.push_back(processedLine);
							} else {
								currentBranch.statements.push_back(processedLine);
							}
						}
					}
					// We've exited the branch
					else if (branchBraceCount == 0 && !inElseBranch) {
						// If this is a single-line if without braces
						if (line.find("{") == std::string::npos && line.find("}") == std::string::npos && 
							line.find("if") == std::string::npos && line.find("else") == std::string::npos) {
							std::string processedLine = std::regex_replace(line, nondetRegex, "nondet");
							currentBranch.statements.push_back(processedLine);
						}
						
						// Check if this is the end of the entire if-else structure
						if (line.find("else") == std::string::npos) {
							branches.push_back(currentBranch);
							inBranch = false;
							inElseBranch = false;
						}
					}
					// We've exited the else branch
					else if (branchBraceCount == 0 && inElseBranch) {
						// If this is a single-line else without braces
						if (line.find("{") == std::string::npos && line.find("}") == std::string::npos && 
							line.find("if") == std::string::npos && line.find("else") == std::string::npos) {
							std::string processedLine = std::regex_replace(line, nondetRegex, "nondet");
							currentBranch.elseStatements.push_back(processedLine);
						}
						
						branches.push_back(currentBranch);
						inBranch = false;
						inElseBranch = false;
					}
				}
				// Regular loop statement (not in a branch)
				else if (!inBranch) {
					// Skip the opening brace line
					if (line.find("{") == std::string::npos || line.find("{") > 0) {
						// Replace nondet calls with symbolic variables
						std::string processedLine = std::regex_replace(line, nondetRegex, "nondet");
						loopBody.push_back(processedLine);
					}
				}
			}
			// We've exited the loop
			else if (loopBraceCount == 0) {
				inLoop = false;
			}
		}
	}
	
	// Now generate the CHC in SMT2 format
	std::ofstream out(file_out);
	
	// Write the header
	out << "(set-logic HORN)" << std::endl;
	
	// Declare the invariant function (only integer variables)
	out << "(declare-fun inv (";
	for (const auto& var : variables) {
		out << "Int ";
	}
	out << ") Bool)" << std::endl;
	
	// Generate the initial state assertion with preconditions
	out << "(assert (forall (";
	for (const auto& var : variables) {
		out << "(" << var.first << " Int) ";
	}
	out << ")" << std::endl;
	out << "   (=> (and ";
	
	// Add preconditions if any
	if (!preconditions.empty()) {
		for (const auto& pre : preconditions) {
			out << "(" << translateCondition(pre) << ") ";
		}
	} else {
		// Default initial conditions if no preconditions specified
		for (const auto& var : variables) {
			out << "(= " << var.first << " 0) ";
		}
	}
	
	out << ") (inv ";
	for (const auto& var : variables) {
		out << var.first << " ";
	}
	out << "))))" << std::endl;
	
	// Generate the transition relation based on the loop
	out << "(assert (forall (";
	// Original variables
	for (const auto& var : variables) {
		out << "(" << var.first << " Int) ";
	}
	
	// Next state variables
	for (const auto& var : variables) {
		std::string next_var = var.first + "_next";
		out << "(" << next_var << " Int) ";
	}
	out << ")" << std::endl;
	
	// Transition relation
	out << "   (=> (and (inv ";
	for (const auto& var : variables) {
		out << var.first << " ";
	}
	out << ")" << std::endl;
	
	// Add loop condition
	if (!loopCondition.empty()) {
		out << "       (" << translateCondition(loopCondition[0]) << ")" << std::endl;
	}
	
	// Process branches first to determine variable updates
	std::map<std::string, std::string> branchUpdates;
	
	// Add branch conditions and statements
	for (const auto& branch : branches) {
		out << "       (ite (" << translateCondition(branch.condition) << ")" << std::endl;
		out << "           (and ";
		
		// Process if-branch statements
		for (const auto& stmt : branch.statements) {
			// Check if it's an assignment
			if (stmt.find("=") != std::string::npos && stmt.find("==") == std::string::npos) {
				size_t equalPos = stmt.find("=");
				std::string lhs = stmt.substr(0, equalPos);
				std::string rhs = stmt.substr(equalPos + 1);
				
				// Remove semicolon and whitespace
				if (rhs.find(";") != std::string::npos) {
					rhs = rhs.substr(0, rhs.find(";"));
				}
				
				// Trim whitespace
				lhs.erase(0, lhs.find_first_not_of(" \t"));
				lhs.erase(lhs.find_last_not_of(" \t") + 1);
				rhs.erase(0, rhs.find_first_not_of(" \t"));
				rhs.erase(rhs.find_last_not_of(" \t") + 1);
				
				// Find the variable being assigned
				for (const auto& var : variables) {
					if (lhs == var.first) {
						std::string next_var = var.first + "_next";
						out << "(" << "= " << next_var << " " << translateExpression(rhs) << ") ";
						branchUpdates[var.first] = "branch";
					}
				}
			}
		}
		
		// For variables not explicitly updated in the if branch, they remain unchanged
		for (const auto& var : variables) {
			if (branchUpdates.find(var.first) == branchUpdates.end()) {
				bool found = false;
				for (const auto& stmt : branch.statements) {
					if (stmt.find(var.first + " =") != std::string::npos || 
						stmt.find(var.first + "=") != std::string::npos) {
						found = true;
						break;
					}
				}
				
				if (!found) {
					std::string next_var = var.first + "_next";
					out << "(" << "= " << next_var << " " << var.first << ") ";
				}
			}
		}
		
		out << ")" << std::endl;
		
		// Process else-branch statements
		if (branch.hasElse) {
			out << "           (and ";
			
			for (const auto& stmt : branch.elseStatements) {
				// Check if it's an assignment
				if (stmt.find("=") != std::string::npos && stmt.find("==") == std::string::npos) {
					size_t equalPos = stmt.find("=");
					std::string lhs = stmt.substr(0, equalPos);
					std::string rhs = stmt.substr(equalPos + 1);
					
					// Remove semicolon and whitespace
					if (rhs.find(";") != std::string::npos) {
						rhs = rhs.substr(0, rhs.find(";"));
					}
					
					// Trim whitespace
					lhs.erase(0, lhs.find_first_not_of(" \t"));
					lhs.erase(lhs.find_last_not_of(" \t") + 1);
					rhs.erase(0, rhs.find_first_not_of(" \t"));
					rhs.erase(rhs.find_last_not_of(" \t") + 1);
					
					// Find the variable being assigned
					for (const auto& var : variables) {
						if (lhs == var.first) {
							std::string next_var = var.first + "_next";
							out << "(" << "= " << next_var << " " << translateExpression(rhs) << ") ";
							branchUpdates[var.first] = "branch";
						}
					}
				}
			}
			
			// For variables not explicitly updated in the else branch, they remain unchanged
			for (const auto& var : variables) {
				if (branchUpdates.find(var.first) == branchUpdates.end()) {
					bool found = false;
					for (const auto& stmt : branch.elseStatements) {
						if (stmt.find(var.first + " =") != std::string::npos || 
							stmt.find(var.first + "=") != std::string::npos) {
							found = true;
							break;
						}
					}
					
					if (!found) {
						std::string next_var = var.first + "_next";
						out << "(" << "= " << next_var << " " << var.first << ") ";
					}
				}
			}
			
			out << "))" << std::endl;
		} else {
			// No else branch, variables remain unchanged
			out << "           (and ";
			
			for (const auto& var : variables) {
				if (branchUpdates.find(var.first) == branchUpdates.end()) {
					std::string next_var = var.first + "_next";
					out << "(" << "= " << next_var << " " << var.first << ") ";
				}
			}
			
			out << "))" << std::endl;
		}
	}
	
	// Add loop body statements as transition constraints
	for (const auto& stmt : loopBody) {
		// Skip comments and empty lines
		if (stmt.empty() || stmt.find("//") == 0) {
			continue;
		}
		
		// Check if it's an assignment
		if (stmt.find("=") != std::string::npos && stmt.find("==") == std::string::npos) {
			size_t equalPos = stmt.find("=");
			std::string lhs = stmt.substr(0, equalPos);
			std::string rhs = stmt.substr(equalPos + 1);
			
			// Remove semicolon and whitespace
			if (rhs.find(";") != std::string::npos) {
				rhs = rhs.substr(0, rhs.find(";"));
			}
			
			// Trim whitespace
			lhs.erase(0, lhs.find_first_not_of(" \t"));
			lhs.erase(lhs.find_last_not_of(" \t") + 1);
			rhs.erase(0, rhs.find_first_not_of(" \t"));
			rhs.erase(rhs.find_last_not_of(" \t") + 1);
			
			// Find the variable being assigned
			for (const auto& var : variables) {
				if (lhs == var.first && branchUpdates.find(var.first) == branchUpdates.end()) {
					std::string next_var = var.first + "_next";
					out << "       (= " << next_var << " " << translateExpression(rhs) << ")" << std::endl;
				}
			}
		}
	}
	
	// For variables not explicitly updated, they remain unchanged
	for (const auto& var : variables) {
		if (branchUpdates.find(var.first) == branchUpdates.end()) {
			bool found = false;
			for (const auto& stmt : loopBody) {
				if (stmt.find(var.first + " =") != std::string::npos || 
					stmt.find(var.first + "=") != std::string::npos) {
					found = true;
					break;
				}
			}
			
			if (!found) {
				std::string next_var = var.first + "_next";
				out << "       (= " << next_var << " " << var.first << ")" << std::endl;
			}
		}
	}
	
	out << ")" << std::endl;
	out << "       (inv ";
	for (const auto& var : variables) {
		std::string next_var = var.first + "_next";
		out << next_var << " ";
	}
	out << "))))" << std::endl;
	
	// Generate the property assertion with postconditions
	out << "(assert (forall (";
	for (const auto& var : variables) {
		out << "(" << var.first << " Int) ";
	}
	out << ")" << std::endl;
	out << "   (=> (and (inv ";
	for (const auto& var : variables) {
		out << var.first << " ";
	}
	out << ")" << std::endl;
	
	// Add negation of loop condition (loop exit)
	if (!loopCondition.empty()) {
		out << "       (not (" << translateCondition(loopCondition[0]) << "))" << std::endl;
	}
	
	out << "      )" << std::endl;
	
	// Add postconditions if any
	if (!postconditions.empty()) {
		out << "      (and ";
		for (const auto& post : postconditions) {
			out << "(" << translateCondition(post) << ") ";
		}
		out << ")" << std::endl;
	} else {
		out << "      true" << std::endl;
	}
	
	out << ")))" << std::endl;
	
	// Add check-sat command
	out << "(check-sat)" << std::endl;
	
	// Close the file
	out.close();
	
	std::cout << "Conversion completed: " << file_in << " -> " << file_out << std::endl;
}

// Helper function to translate C conditions to SMT2 format
std::string cParser::translateCondition(const std::string& condition) {
	std::string result = condition;
	
	// Replace C operators with SMT2 operators
	result = std::regex_replace(result, std::regex("=="), "=");
	result = std::regex_replace(result, std::regex("!="), "distinct");
	result = std::regex_replace(result, std::regex("&&"), "and");
	result = std::regex_replace(result, std::regex("\\|\\|"), "or");
	result = std::regex_replace(result, std::regex("!"), "not");
	result = std::regex_replace(result, std::regex("<="), "<=");
	result = std::regex_replace(result, std::regex(">="), ">=");
	result = std::regex_replace(result, std::regex("<"), "<");
	result = std::regex_replace(result, std::regex(">"), ">");
	
	// Format the condition properly for SMT2
	if (result.find("=") != std::string::npos && result.find("distinct") == std::string::npos) {
		size_t equalPos = result.find("=");
		std::string lhs = result.substr(0, equalPos);
		std::string rhs = result.substr(equalPos + 1);
		
		// Trim whitespace
		lhs.erase(0, lhs.find_first_not_of(" \t"));
		lhs.erase(lhs.find_last_not_of(" \t") + 1);
		rhs.erase(0, rhs.find_first_not_of(" \t"));
		rhs.erase(rhs.find_last_not_of(" \t") + 1);
		
		result = "= " + lhs + " " + rhs;
	}
	
	return result;
}

// Helper function to translate C expressions to SMT2 format
std::string cParser::translateExpression(const std::string& expr) {
	std::string result = expr;
	
	// Check for arithmetic operations
	if (expr.find("+") != std::string::npos) {
		size_t opPos = expr.find("+");
		std::string lhs = expr.substr(0, opPos);
		std::string rhs = expr.substr(opPos + 1);
		
		// Trim whitespace
		lhs.erase(0, lhs.find_first_not_of(" \t"));
		lhs.erase(lhs.find_last_not_of(" \t") + 1);
		rhs.erase(0, rhs.find_first_not_of(" \t"));
		rhs.erase(rhs.find_last_not_of(" \t") + 1);
		
		result = "(+ " + lhs + " " + rhs + ")";
	}
	else if (expr.find("-") != std::string::npos) {
		size_t opPos = expr.find("-");
		std::string lhs = expr.substr(0, opPos);
		std::string rhs = expr.substr(opPos + 1);
		
		// Trim whitespace
		lhs.erase(0, lhs.find_first_not_of(" \t"));
		lhs.erase(lhs.find_last_not_of(" \t") + 1);
		rhs.erase(0, rhs.find_first_not_of(" \t"));
		rhs.erase(rhs.find_last_not_of(" \t") + 1);
		
		result = "(- " + lhs + " " + rhs + ")";
	}
	else if (expr.find("*") != std::string::npos) {
		size_t opPos = expr.find("*");
		std::string lhs = expr.substr(0, opPos);
		std::string rhs = expr.substr(opPos + 1);
		
		// Trim whitespace
		lhs.erase(0, lhs.find_first_not_of(" \t"));
		lhs.erase(lhs.find_last_not_of(" \t") + 1);
		rhs.erase(0, rhs.find_first_not_of(" \t"));
		rhs.erase(rhs.find_last_not_of(" \t") + 1);
		
		result = "(* " + lhs + " " + rhs + ")";
	}
	else if (expr.find("/") != std::string::npos) {
		size_t opPos = expr.find("/");
		std::string lhs = expr.substr(0, opPos);
		std::string rhs = expr.substr(opPos + 1);
		
		// Trim whitespace
		lhs.erase(0, lhs.find_first_not_of(" \t"));
		lhs.erase(lhs.find_last_not_of(" \t") + 1);
		rhs.erase(0, rhs.find_first_not_of(" \t"));
		rhs.erase(rhs.find_last_not_of(" \t") + 1);
		
		result = "(div " + lhs + " " + rhs + ")";
	}
	
	return result;
}
