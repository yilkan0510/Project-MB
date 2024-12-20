#include "EarleyParser.h"
#include <iostream>
#include <queue>

EarleyParser::EarleyParser(const CFG &grammar) : cfg(grammar) {
  startSymbol = cfg.getStartSymbol();
}

bool EarleyParser::parse(const std::string &input) {
  reset(input);
  while (!isDone()) {
    nextStep();
  }
  return isAccepted();
}

void EarleyParser::reset(const std::string &input) {
  currentInput = input;
  size_t length = input.size();
  chart.clear();
  chart.resize(length + 1);

  // Augmented start: S' -> S
  std::string augmented = startSymbol + "'";
  chart[0].insert({augmented, cfg.getStartSymbol(), 0, 0});
  applyPredictComplete(0);

  currentPos = 0;
  finished = false;
  accepted = false;
}

bool EarleyParser::nextStep() {
  if (finished) return false;

  if (currentPos < currentInput.size()) {
    // SCAN step
    char c = currentInput[currentPos];
    std::vector<EarleyItem> items(chart[currentPos].begin(), chart[currentPos].end());
    for (auto &item : items) {
      if (item.dotPos < item.body.size()) {
        char nextSym = item.body[item.dotPos];
        if (isTerminal(nextSym) && nextSym == c) {
          EarleyItem newItem = item;
          newItem.dotPos++;
          chart[currentPos+1].insert(newItem);
        }
      }
    }
    applyPredictComplete(currentPos+1);
    currentPos++;
  } else {
    // No more characters to scan
    finished = true;
    // Check acceptance
    std::string augmented = startSymbol + "'";
    for (auto &item : chart[currentInput.size()]) {
      if (item.head == augmented && item.dotPos == item.body.size() && item.startIdx == 0) {
        accepted = true;
        break;
      }
    }
  }

  if (currentPos == currentInput.size() && !finished) {
    // Final predict/complete at the end
    applyPredictComplete(currentPos);
    finished = true;
    // Check acceptance again
    std::string augmented = startSymbol + "'";
    for (auto &item : chart[currentInput.size()]) {
      if (item.head == augmented && item.dotPos == item.body.size() && item.startIdx == 0) {
        accepted = true;
        break;
      }
    }
  }

  return !finished;
}

bool EarleyParser::isDone() const {
  return finished;
}

bool EarleyParser::isAccepted() const {
  return accepted;
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
