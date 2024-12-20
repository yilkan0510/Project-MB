#ifndef EARLYPARSER_H
#define EARLYPARSER_H

#include "CFG.h"
#include <string>
#include <vector>
#include <set>

struct EarleyItem {
  std::string head;
  std::string body;
  size_t dotPos;
  size_t startIdx;

  bool operator<(const EarleyItem &other) const {
    if (head != other.head) return head < other.head;
    if (body != other.body) return body < other.body;
    if (dotPos != other.dotPos) return dotPos < other.dotPos;
    return startIdx < other.startIdx;
  }

  bool operator==(const EarleyItem &other) const {
    return head == other.head && body == other.body &&
           dotPos == other.dotPos && startIdx == other.startIdx;
  }
};

class EarleyParser {
public:
  explicit EarleyParser(const CFG &grammar);

  // Normal parse: runs to completion
  bool parse(const std::string &input);

  // Step-by-step interface:
  void reset(const std::string &input);
  bool nextStep();       // Advance one step in parsing
  bool isDone() const;   // True if no more steps
  bool isAccepted() const;// True if the input is accepted
  size_t getCurrentPos() const { return currentPos; }
  const std::vector<std::set<EarleyItem>>& getChart() const { return chart; }

  std::vector<std::string> stepExplanations;

private:
  const CFG &cfg;
  std::string startSymbol;

  std::string currentInput;
  std::vector<std::set<EarleyItem>> chart;
  size_t currentPos;
  bool finished;
  bool accepted;

  bool isNonTerminal(const std::string &symbol) const;
  bool isTerminal(char symbol) const;

  void applyPredictComplete(size_t pos);
  void complete(const EarleyItem &item, size_t pos, bool &changed);
  // Each element explains what happened at currentPos

};

#endif
