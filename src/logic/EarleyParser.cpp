#include "EarleyParser.h"
#include <iostream>
#include <queue>

/**************************************************
 * Implementation
 **************************************************/

EarleyParser::EarleyParser(const CFG &cfg) : cfg(cfg) {
  startSymbol = cfg.getStartSymbol();
  // Build an augmented symbol, e.g. "S'"
  augmentedSymbol = startSymbol + "'";
}

// Full parse (no stepping)
bool EarleyParser::parse(const std::string &input) {
  reset(input);
  while(!isDone()) {
    nextStep();
  }
  return isAccepted();
}

void EarleyParser::reset(const std::string &input) {
  currentInput = input;
  currentPos = 0;
  finished = false;
  accepted = false;
  stepExplanations.clear();

  // chart has length input.size() + 1
  size_t length = currentInput.size();
  chart.clear();
  chart.resize(length + 1);

  // Insert the augmented item: S' -> • S, at chart[0]
  // That is: head="S'", body=S, dotPos=0, startIdx=0
  EarleyItem initial{augmentedSymbol, startSymbol, 0, 0};
  chart[0].insert(initial);

  // Apply predict & complete to chart[0]
  predictAndComplete(0);

  std::ostringstream msg;
  msg << "EarleyParser reset: inserted augmented item "
      << augmentedSymbol << " -> •" << startSymbol
      << " at chart[0].";
  stepExplanations.push_back(msg.str());
}

bool EarleyParser::nextStep() {
  if (finished) return false;

  // 1. If we still have input left, SCAN from chart[currentPos] to chart[currentPos+1]
  if (currentPos < currentInput.size()) {
    char nextChar = currentInput[currentPos];

    // Step A: SCAN
    scan(nextChar);

    // Step B: Predict & Complete in chart[currentPos+1]
    predictAndComplete(currentPos + 1);

    // Move forward in the input
    currentPos++;

    std::ostringstream msg;
    msg << "Earley: advanced to pos=" << currentPos
        << " (nextChar='" << nextChar << "').";
    stepExplanations.push_back(msg.str());
  }
  else {
    // We have reached the end of the input
    finished = true;

    // Check if the augmented item was completed in chart[input.size()]
    // That means an item: S' -> S • with startIdx=0 is present
    for (auto &item : chart[currentInput.size()]) {
      if (item.head == augmentedSymbol &&
          item.dotPos == item.body.size() &&
          item.startIdx == 0) {
        accepted = true;
        break;
      }
    }

    std::ostringstream msg;
    msg << "Earley: end of input. "
        << (accepted ? "ACCEPTED" : "REJECTED");
    stepExplanations.push_back(msg.str());
  }

  return !finished;
}

bool EarleyParser::isNonTerminal(const std::string &symbol) const {
  // For single-character nonterminals, symbol.size()==1
  // and symbol is in cfg.getNonTerminals().
  return (cfg.getNonTerminals().count(symbol) > 0);
}

bool EarleyParser::isTerminal(char symbol) const {
  // For single-character terminals, check if symbol is in the set.
  return (cfg.getTerminals().count(symbol) > 0);
}

/**************************************************
 * SCAN
 * Move all items in chart[pos] with next symbol= char
 * to chart[pos+1], but dotPos++.
 **************************************************/
void EarleyParser::scan(char nextChar) {
  size_t pos = currentPos; // from chart[pos] to chart[pos+1]

  // We'll iterate over a snapshot of chart[pos]
  // because we might insert new items into chart[pos+1].
  std::vector<EarleyItem> items(chart[pos].begin(), chart[pos].end());

  bool scannedAnything = false;
  for (auto &item : items) {
    // If dotPos not at end, check the next symbol
    if (item.dotPos < item.body.size()) {
      char sym = item.body[item.dotPos];
      // If it's a terminal and matches nextChar, we can shift
      if (isTerminal(sym) && sym == nextChar) {
        // Move the dot one to the right
        EarleyItem newItem = item;
        newItem.dotPos++;
        // Insert it into chart[pos+1]
        chart[pos + 1].insert(newItem);
        scannedAnything = true;
      }
    }
  }

  std::ostringstream msg;
  msg << "Earley: SCAN at pos=" << pos
      << " with nextChar='" << nextChar << "'. ";
  if (scannedAnything) {
    msg << "Some items scanned -> chart[" << (pos+1) << "] updated.";
  } else {
    msg << "No items matched terminal '" << nextChar << "'.";
  }
  stepExplanations.push_back(msg.str());
}

/**************************************************
 * PREDICT & COMPLETE
 * For each item in chart[pos],
 *   - If next symbol is a nonterminal, PREDICT
 *   - If dot is at end, COMPLETE
 **************************************************/
void EarleyParser::predictAndComplete(size_t pos) {
  bool changed = true;
  while(changed) {
    changed = false;

    // We'll iterate over a snapshot of chart[pos] items
    std::vector<EarleyItem> items(chart[pos].begin(), chart[pos].end());

    for (auto &item : items) {
      // If dot not at end, check next symbol
      if (item.dotPos < item.body.size()) {
        std::string sym(1, item.body[item.dotPos]);
        // PREDICT if sym is a nonterminal
        if (isNonTerminal(sym)) {
          // For each rule X -> Y in the CFG,
          // if X == sym, add an item X -> •Y in chart[pos].
          // (startIdx = pos)
          auto &prodMap = cfg.getProductionRules();
          auto it = prodMap.find(sym);
          if (it != prodMap.end()) {
            for (auto &rhs : it->second) {
              // build an item sym -> •rhs
              EarleyItem newItem{sym, rhs, 0, pos};
              auto ins = chart[pos].insert(newItem);
              if (ins.second) {
                changed = true;
                std::ostringstream msg;
                msg << "Earley: PREDICT at chart[" << pos << "]: "
                    << newItem.head << " -> •" << newItem.body;
                stepExplanations.push_back(msg.str());
              }
            }
          }
        }
      }
      else {
        // dot is at the end -> COMPLETE
        // For each item in chart[item.startIdx],
        // if the next symbol matches item.head, move dot forward
        bool completedSomething = false;

        // We'll examine all items in chart[item.startIdx].
        std::vector<EarleyItem> startItems(chart[item.startIdx].begin(),
                                           chart[item.startIdx].end());

        for (auto &stItem : startItems) {
          if (stItem.dotPos < stItem.body.size()) {
            // check the next symbol
            char stSym = stItem.body[stItem.dotPos];
            // if that symbol is the completed item's head
            if (std::string(1, stSym) == item.head) {
              // push dot forward
              EarleyItem newItem = stItem;
              newItem.dotPos++;

              // Insert in chart[pos] (the position we’re “completing” at)
              auto ins = chart[pos].insert(newItem);
              if (ins.second) {
                changed = true;
                completedSomething = true;
              }
            }
          }
        }

        if (completedSomething) {
          std::ostringstream msg;
          msg << "Earley: COMPLETE at chart[" << pos << "]: "
              << item.head << " -> " << item.body << " •";
          stepExplanations.push_back(msg.str());
        }
      }
    }
  }
}