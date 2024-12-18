#ifndef EARLYPARSER_H
#define EARLYPARSER_H

#include "CFG.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <tuple>

// An Earley item: (head, body, dotPosition, startIndex)
// Example: [A -> α • β, i]
struct EarleyItem {
    std::string head;
    std::string body;
    size_t dotPos;    // position of the dot in the body
    size_t startIdx;  // where this item started in the input

    bool operator<(const EarleyItem &other) const {
        return std::tie(head, body, dotPos, startIdx) <
               std::tie(other.head, other.body, other.dotPos, other.startIdx);
    }

    bool operator==(const EarleyItem &other) const {
        return head == other.head && body == other.body &&
               dotPos == other.dotPos && startIdx == other.startIdx;
    }
};

class EarleyParser {
public:
    explicit EarleyParser(CFG &grammar);
    bool parse(const std::string &input);

private:
    CFG &cfg;
    std::string startSymbol;

    // Chart: vector of sets of items, one set per position in the input
    std::vector<std::set<EarleyItem>> chart;

    void predict(const EarleyItem &item, size_t pos);
    void scan(const EarleyItem &item, size_t pos, const std::string &input);
    void complete(const EarleyItem &item, size_t pos);

    [[nodiscard]] bool isNonTerminal(const std::string &symbol) const;
    [[nodiscard]] bool isTerminal(char symbol) const;
};

#endif
