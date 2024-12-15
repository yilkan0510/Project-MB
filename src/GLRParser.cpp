//
// Created by manaci4466 on 12/15/24.
//

#include "GLRParser.h"

GLRParser::GLRParser(const std::vector<Rule> &rules) : grammar(rules) {}

void GLRParser::parse(const std::vector<std::string> &tokens) {
    // Initialize chart
    Chart chart(tokens.size() + 1);
    chart[0].push_back({0, {}, 0});

    // Main GLR loop
    for (int pos = 0; pos <= tokens.size(); ++pos) {
        for (const auto &state : chart[pos]) {
            processState(state, tokens, chart, pos);
        }
    }

    // Handle final states
    finalize(chart.back());
}

void GLRParser::processState(const ParseState &state, const std::vector<std::string> &tokens, Chart &chart, int pos) {
    // Implement shift, reduce, and split logic
}

void GLRParser::finalize(const std::vector<ParseState> &finalStates) {
    // Resolve parse forest into trees or display ambiguity
}


