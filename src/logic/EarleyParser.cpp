#include "EarleyParser.h"
#include <iostream>
#include <queue>

EarleyParser::EarleyParser(const CFG &grammar) : cfg(grammar) {
  startSymbol = cfg.getStartSymbol();
}

bool EarleyParser::parse(const std::string &input) {
  reset(input);
  // Keep calling nextStep() until done
  while (!isDone()) {
    nextStep();
  }
  return isAccepted();
}

void EarleyParser::reset(const std::string &input) {
  currentInput = input;
  size_t length = input.size();

  // Clear the chart and resize for [0..length]
  chart.clear();
  chart.resize(length + 1);

  // Clear explanations for a fresh parse
  stepExplanations.clear();

  // Augment the grammar: S' -> S
  std::string augmented = startSymbol + "'";
  // Insert the initial item into chart[0]
  chart[0].insert({augmented, cfg.getStartSymbol(), 0, 0});

  // Apply PREDICT for chart[0]
  applyPredictComplete(0);

  currentPos = 0;
  finished = false;
  accepted = false;
}

bool EarleyParser::nextStep() {
  if (finished) return false;

  // 1) If we still have characters in the input, perform SCAN
  if (currentPos < currentInput.size()) {
    char c = currentInput[currentPos];

    // SCAN
    std::vector<EarleyItem> items(chart[currentPos].begin(), chart[currentPos].end());
    bool scannedSomething = false;
    for (auto &item : items) {
      // If dot isn't at the end, check if next symbol is a terminal
      if (item.dotPos < item.body.size()) {
        char nextSym = item.body[item.dotPos];
        if (isTerminal(nextSym) && nextSym == c) {
          EarleyItem newItem = item;
          newItem.dotPos++;
          chart[currentPos+1].insert(newItem);
          scannedSomething = true;
        }
      }
    }

    if (scannedSomething) {
      stepExplanations.push_back(
          "Chart[" + std::to_string(currentPos) +
          "]: Scanned character '" + std::string(1,c) + "'"
      );
    } else {
      stepExplanations.push_back(
          "Chart[" + std::to_string(currentPos) +
          "]: No terminal scanned (character '" + std::string(1,c) + "')"
      );
    }

    // 2) Apply PREDICT/COMPLETE for chart[currentPos+1]
    applyPredictComplete(currentPos+1);
    stepExplanations.push_back(
        "Chart[" + std::to_string(currentPos+1) +
        "]: Applied predict/complete after scanning."
    );

    // Move to next position in the input
    currentPos++;

  } else {
    // No more input to scan, we finalize
    finished = true;
    bool wasAccepted = false;
    std::string augmented = startSymbol + "'";
    for (auto &item : chart[currentInput.size()]) {
      if (item.head == augmented && item.dotPos == item.body.size() && item.startIdx == 0) {
        accepted = true;
        wasAccepted = true;
        break;
      }
    }
    stepExplanations.push_back(
        "Reached end of input. " +
        std::string(wasAccepted ? "ACCEPTED" : "REJECTED")
    );
  }

  // 3) If we've just scanned the last char, we may need a final PREDICT/COMPLETE
  if (currentPos == currentInput.size() && !finished) {
    applyPredictComplete(currentPos);
    finished = true;

    bool wasAccepted = false;
    std::string augmented = startSymbol + "'";
    for (auto &item : chart[currentInput.size()]) {
      if (item.head == augmented && item.dotPos == item.body.size() && item.startIdx == 0) {
        accepted = true;
        wasAccepted = true;
        break;
      }
    }
    stepExplanations.push_back(
        "Final completion at Chart[" + std::to_string(currentPos) + "]: " +
        (wasAccepted ? "ACCEPTED" : "REJECTED")
    );
  }

  return !finished;
}

bool EarleyParser::isDone() const {
  return finished;
}

bool EarleyParser::isAccepted() const {
  return accepted;
}

// This function does the PREDICT and COMPLETE for all items in chart[pos]
void EarleyParser::applyPredictComplete(size_t pos) {
  bool changed = true;
  while (changed) {
    changed = false;
    std::vector<EarleyItem> items(chart[pos].begin(), chart[pos].end());
    for (auto &item : items) {
      // If dotPos is not at the end, check next symbol
      if (item.dotPos < item.body.size()) {
        // PREDICT if next symbol is a non-terminal
        std::string sym(1, item.body[item.dotPos]);
        if (isNonTerminal(sym)) {
          const auto &rules = cfg.getProductionRules();
          auto it = rules.find(sym);
          if (it != rules.end()) {
            for (auto &rhs : it->second) {
              EarleyItem newItem{sym, rhs, 0, pos};
              if (chart[pos].insert(newItem).second) {
                changed = true;
                stepExplanations.push_back(
                    "Predict: " + sym + " -> " + rhs +
                    " at Chart[" + std::to_string(pos) + "]"
                );
              }
            }
          }
        }
      } else {
        // COMPLETE if dot is at end
        bool wasChanged = false;
        complete(item, pos, wasChanged);
        if (wasChanged) {
          changed = true;
          // Add an explanation if needed here
          stepExplanations.push_back(
              "Complete: " + item.head + " -> " + item.body +
              " (from Chart[" + std::to_string(item.startIdx) +
              "] to Chart[" + std::to_string(pos) + "])"
          );
        }
      }
    }
  }
}

// COMPLETE rule
void EarleyParser::complete(const EarleyItem &item, size_t pos, bool &changed) {
  // For each item in chart[item.startIdx], if the dot points to 'item.head', move the dot forward
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

// Utility checks
bool EarleyParser::isNonTerminal(const std::string &symbol) const {
  return cfg.getNonTerminals().count(symbol) > 0;
}

bool EarleyParser::isTerminal(char symbol) const {
  return cfg.getTerminals().count(symbol) > 0;
}
