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
  bool parse(const std::string &input);

private:
  const CFG &cfg;
  std::string startSymbol;

  std::vector<std::set<EarleyItem>> chart;

  bool isNonTerminal(const std::string &symbol) const;
  bool isTerminal(char symbol) const;

  void applyPredictComplete(size_t pos);
  void complete(const EarleyItem &item, size_t pos, bool &changed);
};

#endif
