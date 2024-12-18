//
// Created by manaci4466 on 12/15/24.
//

#include "GLRParser.h"

GLRParser::GLRParser(const std::vector<Rule> &rules) : grammar(rules) {}

GLRParser::GLRParser(const std::string& filename)
{
    loadGrammar(filename);
}

void GLRParser::parse(const std::vector<std::string> &tokens) {
    // Initialize chart
    Chart chart(tokens.size() + 1);
    chart[0].push_back({0, {}, 0}); // Initial state

    // Main GLR loop
    for (int pos = 0; pos <= tokens.size(); ++pos) {
        for (const auto &state : chart[pos]) {
            processState(state, tokens, chart, pos);
        }
    }

    // Handle final states
    finalize(chart.back());
}

void GLRParser::loadGrammar(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    // Parse JSON
    json j;
    file >> j;

    // Load production rules
    int id = 0; // Unique ID for each rule
    for (const auto& production : j["Productions"]) {
        Rule rule;
        rule.id = id++;
        rule.head = production["head"].get<std::string>();
        rule.body = production["body"].get<std::vector<std::string>>();
        grammar.push_back(rule);
    }

}

void GLRParser::processState(const ParseState &state, const std::vector<std::string> &tokens, Chart &chart, int pos) {
     const ParseState tempState = state; // Avoid modifying the original state during iteration

    // SHIFT operation
    for (const Rule &rule : grammar) {
        if (tempState.position < rule.body.size()) {
            const std::string &expectedSymbol = rule.body[tempState.position];
            if (pos < tokens.size() && expectedSymbol == tokens[pos]) {
                // Create a new state with the token shifted onto the stack
                ParseState nextState = tempState;
                nextState.stack.push_back(tokens[pos]); // Add token to the stack
                nextState.position++;                  // Move to the next symbol in the rule body
                chart[pos + 1].push_back(nextState);   // Add state to the next chart position
            }
        }
    }

    // REDUCE operation
    for (const Rule &rule : grammar) {
        if (tempState.position == rule.body.size()) {
            // Ensure the stack matches the rule's body length
            if (tempState.stack.size() >= rule.body.size() &&
                std::equal(
                    tempState.stack.end() - rule.body.size(),
                    tempState.stack.end(),
                    rule.body.begin())) {

                // Create a new state for the reduction
                ParseState nextState = tempState;
                nextState.stack.resize(tempState.stack.size() - rule.body.size()); // Remove the rule's body
                nextState.stack.push_back(rule.head);                             // Push the rule's head
                nextState.position = 0; // Reset position after reduction
                chart[pos].push_back(nextState); // Add the reduced state to the current position
            }
        }
    }

    // NULL productions (handle empty bodies)
    for (const Rule &rule : grammar) {
        if (rule.body.empty() &&
            (tempState.stack.empty() || tempState.stack.back() != rule.head)) { // Avoid redundant null productions
            ParseState nextState = tempState;
            nextState.stack.push_back(rule.head); // Push the rule's head for null production
            nextState.position = 0; // Reset position for null production
            chart[pos].push_back(nextState);
        }
    }
}

void GLRParser::finalize(const std::vector<ParseState> &finalStates) {
    const std::string startSymbol = "S"; // Assuming "S" is the start symbol of the grammar
    std::vector<std::unique_ptr<TreeNode>> validTrees;

    for (const auto &state : finalStates) {
        if (!state.stack.empty() && state.stack.back() == startSymbol) {
            // Build a parse tree from the stack
            auto tree = buildParseTree(state.stack);
            validTrees.push_back(std::move(tree)); // Transfer ownership to the validTrees vector
        }
    }

    // Handle results
    if (validTrees.empty()) {
        std::cout << "Parsing failed: No valid parse found." << std::endl;
        return;
    }

    std::cout << "Parsing succeeded with " << validTrees.size() << " valid parse(s):" << std::endl;

    for (size_t i = 0; i < validTrees.size(); ++i) {
        std::cout << "Parse Tree #" << (i + 1) << ":" << std::endl;
        validTrees[i]->printTree();
    }

    if (validTrees.size() > 1) {
        std::cout << "Ambiguity detected: Multiple valid parses exist." << std::endl;
    }
}

std::unique_ptr<TreeNode>  GLRParser::buildParseTree(const std::vector<std::string> &stack) {
    auto root = std::make_unique<TreeNode>(stack.back()); // Start with the last symbol
    TreeNode* current = root.get();

    // Walk backward through the stack to construct the tree
    for (int i = static_cast<int>(stack.size()) - 2; i >= 0; --i) {
        auto child = std::make_unique<TreeNode>(stack[i]);
        current->children.push_back(std::move(child)); // Transfer ownership
        current = current->children.back().get();      // Update the current node
    }

    return root;
}