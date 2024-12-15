//
// Created by manaci4466 on 12/15/24.
//

#ifndef GLRPARSER_H
#define GLRPARSER_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

struct Rule {
    int id;
    std::string lhs;
    std::vector<std::string> rhs;
};

struct ParseState {
    int state;
    std::vector<std::string> stack;
    int position;
};

using Chart = std::vector<std::vector<ParseState>>;

class GLRParser {
public:
    GLRParser(const std::vector<Rule> &rules);

    void parse(const std::vector<std::string> &tokens);

    void loadGrammar(const std::string &filename, std::vector<Rule> &rules);

private:
    std::vector<Rule> grammar;

    void processState(const ParseState &state, const std::vector<std::string> &tokens, Chart &chart, int pos);

    void finalize(const std::vector<ParseState> &finalStates);
};


#endif //GLRPARSER_H
