#include "EarleyParser.h"
#include <iostream>
#include <queue>

EarleyParser::EarleyParser(const CFG &grammar) : cfg(grammar) {
  startSymbol = cfg.getStartSymbol();
}

bool EarleyParser::parse(const std::string &input) {
  // Augment the grammar: S' -> S
  std::string augmented = startSymbol + "'";
  chart.clear();
  chart.resize(input.size() + 1);
  // Add [S'->·S,0]
  EarleyItem startItem{augmented, cfg.getStartSymbol(), 0, 0};
  chart[0].insert(startItem);

  // Predict/Complete at position 0
  applyPredictComplete(0);

  for (size_t pos = 0; pos <= input.size(); ++pos) {
    if (pos < input.size()) {
      // SCAN
      char c = input[pos];
      std::vector<EarleyItem> items(chart[pos].begin(), chart[pos].end());
      for (auto &item : items) {
        if (item.dotPos < item.body.size()) {
          char nextSym = item.body[item.dotPos];
          if (isTerminal(nextSym) && nextSym == c) {
            EarleyItem newItem = item;
            newItem.dotPos++;
            chart[pos+1].insert(newItem);
          }
        }
      }
      applyPredictComplete(pos+1);
    }
  }

  // Accept if [S'->S·,0] in chart[input.size()]
  for (auto &item : chart[input.size()]) {
    if (item.head == augmented && item.dotPos == item.body.size() && item.startIdx == 0) {
      return true;
    }
  }

  return false;
}

void EarleyParser::applyPredictComplete(size_t pos) {
  bool changed = true;
  while (changed) {
    changed = false;
    std::vector<EarleyItem> items(chart[pos].begin(), chart[pos].end());
    for (auto &item : items) {
      if (item.dotPos < item.body.size()) {
        // PREDICT
        std::string sym(1, item.body[item.dotPos]);
        if (isNonTerminal(sym)) {
          const auto &rules = cfg.getProductionRules();
          auto it = rules.find(sym);
          if (it != rules.end()) {
            for (auto &rhs : it->second) {
              EarleyItem newItem{sym, rhs, 0, pos};
              if (chart[pos].insert(newItem).second) {
                changed = true;
              }
            }
          }
        }
      } else {
        // COMPLETE
        complete(item, pos, changed);
      }
    }
  }
}

void EarleyParser::complete(const EarleyItem &item, size_t pos, bool &changed) {
  // For each item in chart[item.startIdx] that expects item.head next, advance dot
  for (auto &cand : chart[item.startIdx]) {
    if (cand.dotPos < cand.body.size()) {
      std::string nextSym(1, cand.body[cand.dotPos]);
      if (nextSym == item.head) {
        EarleyItem newItem = cand;
        newItem.dotPos++;
        if (chart[pos].insert(newItem).second) {
          changed = true;
        }
      }
    }
  }
}

bool EarleyParser::isNonTerminal(const std::string &symbol) const {
  return cfg.getNonTerminals().count(symbol) > 0;
}

bool EarleyParser::isTerminal(char symbol) const {
  return cfg.getTerminals().count(symbol) > 0;
}
