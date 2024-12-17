//
// Created by manaci4466 on 12/15/24.
//

#ifndef GLRPARSER_H
#define GLRPARSER_H

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include <map>
#include <set>
#include <fstream>
#include "CFG.h"
#include "json.hpp"

using json = nlohmann::json;

struct Rule {
    int id;
    std::string head;
    std::vector<std::string> body;
};

struct ParseState {
    int state;
    std::vector<std::string> stack;
    int position;
};

struct TreeNode {
    std::string value;
    std::vector<std::unique_ptr<TreeNode>> children;

    explicit TreeNode(std::string val) : value(std::move(val)) {}

    // Recursively print the tree
    void printTree(int depth = 0) const {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << value << std::endl;
        for (const auto &child : children) {
            child->printTree(depth + 1);
        }
    }
};

using Chart = std::vector<std::vector<ParseState>>;

class GLRParser {
public:
    explicit GLRParser(const std::vector<Rule> &rules);

    explicit GLRParser(const std::string &filename);

    void parse(const std::vector<std::string> &tokens);

    void loadGrammar(const std::string &filename);

private:
    std::vector<Rule> grammar;

    void processState(const ParseState &state, const std::vector<std::string> &tokens, Chart &chart, int pos);

    void finalize(const std::vector<ParseState> &finalStates);

    std::unique_ptr<TreeNode> buildParseTree(const std::vector<std::string> &stack);
};


#endif //GLRPARSER_H
