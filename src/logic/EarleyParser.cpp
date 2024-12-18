#include "EarleyParser.h"
#include <iostream>
#include <queue>

EarleyParser::EarleyParser(CFG &grammar) : cfg(grammar) {
    startSymbol = cfg.getStartSymbol();
}

// Earley parsing steps:
// 1. Initialize: Add [S' -> •S, 0] to chart[0], where S' is a new start symbol.
// 2. For each position, repeatedly PREDICT and COMPLETE until no new items are added.
// 3. SCAN to the next position for each terminal that matches.

bool EarleyParser::parse(const std::string &input) {
    // We'll use an augmented grammar: S' -> S
    // Add a new start rule: S' -> S
    std::string augmentedStart = startSymbol + "'";
    // We just simulate it by adding one initial item:
    chart.clear();
    chart.resize(input.size() + 1);
    chart[0].insert({augmentedStart, startSymbol, 0, 0});

    // We'll repeatedly apply predict and complete until no change, then scan terminals if possible.
    for (size_t pos = 0; pos <= input.size(); ++pos) {
        bool changed = true;
        while (changed) {
            changed = false;
            // We need to systematically apply PREDICT and COMPLETE to all items
            // Use a queue to handle newly added items
            std::queue<EarleyItem> worklist;
            for (auto &item : chart[pos]) {
                worklist.push(item);
            }

            // Keep track of previously known size to detect changes
            size_t oldSize = chart[pos].size();
            while (!worklist.empty()) {
                EarleyItem item = worklist.front();
                worklist.pop();

                // If dot is not at the end and next symbol is non-terminal -> PREDICT
                if (item.dotPos < item.body.size()) {
                    std::string symbol(1, item.body[item.dotPos]);
                    if (isNonTerminal(symbol)) {
                        predict(item, pos);
                    }
                } else {
                    // Dot at end: COMPLETE
                    complete(item, pos);
                }
            }

            if (chart[pos].size() > oldSize) {
                changed = true;
            }
        }

        // Now SCAN if pos < input.size()
        if (pos < input.size()) {
            // For all items in chart[pos], if next symbol is a terminal matching input[pos], SCAN
            std::vector<EarleyItem> itemsToScan(chart[pos].begin(), chart[pos].end());
            for (auto &item : itemsToScan) {
                if (item.dotPos < item.body.size()) {
                    char nextSym = item.body[item.dotPos];
                    if (isTerminal(nextSym) && nextSym == input[pos]) {
                        scan(item, pos, input);
                    }
                }
            }
        }
    }

    // Accept if [S' -> S•, 0] in chart[end]
    size_t endPos = input.size();
    for (auto &item : chart[endPos]) {
        if (item.head == augmentedStart && item.dotPos == item.body.size() && item.startIdx == 0) {
            return true;
        }
    }

    return false;
}

void EarleyParser::predict(const EarleyItem &item, size_t pos) {
    // Predict all rules that start with the non-terminal at the dot
    char nextSym = item.body[item.dotPos];
    std::string symbol(1, nextSym);

    // For each production of symbol, add [symbol -> •rhs, pos] to chart[pos]
    auto it = cfg.getProductionRules().find(symbol);
    if (it != cfg.getProductionRules().end()) {
        for (auto &rhs : it->second) {
            EarleyItem newItem{symbol, rhs, 0, pos};
            chart[pos].insert(newItem);
        }
    }
}

void EarleyParser::scan(const EarleyItem &item, size_t pos, const std::string &input) {
    // Move the dot over the terminal
    EarleyItem newItem = item;
    newItem.dotPos++;
    chart[pos+1].insert(newItem);
}

void EarleyParser::complete(const EarleyItem &item, size_t pos) {
    // For each item in chart[item.startIdx] that expects item.head next, advance dot
    std::string A = item.head;
    for (auto &cand : chart[item.startIdx]) {
        if (cand.dotPos < cand.body.size() && std::string(1,cand.body[cand.dotPos]) == A) {
            EarleyItem newItem = cand;
            newItem.dotPos++;
            chart[pos].insert(newItem);
        }
    }
}

bool EarleyParser::isNonTerminal(const std::string &symbol) const {
    return cfg.getNonTerminals().count(symbol) > 0;
}

bool EarleyParser::isTerminal(char symbol) const {
    return cfg.getTerminals().count(symbol) > 0;
}
