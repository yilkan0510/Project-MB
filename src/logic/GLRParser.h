#ifndef GLRPARSER_H
#define GLRPARSER_H

#include "CFG.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

struct GLRRule {
  std::string head;
  std::vector<std::string> body;
  int id;
};

struct LRItem {
  int ruleId;
  size_t dotPos;
  bool operator<(const LRItem &o) const {
    if (ruleId != o.ruleId) return ruleId < o.ruleId;
    return dotPos < o.dotPos;
  }
  bool operator==(const LRItem &o) const {
    return ruleId == o.ruleId && dotPos == o.dotPos;
  }
};

struct LRState {
  std::set<LRItem> items;
  bool operator<(const LRState &o) const {
    if (items.size() < o.items.size()) return true;
    if (items.size() > o.items.size()) return false;
    return std::lexicographical_compare(items.begin(), items.end(), o.items.begin(), o.items.end());
  }
  bool operator==(const LRState &o) const {
    return items == o.items;
  }
};

enum class ActionType { Shift, Reduce, Accept, Error };

struct LRAction {
  ActionType type;
  int stateOrRule;
};

struct GSSNode {
  int state;
  std::vector<std::shared_ptr<GSSNode>> preds;
};

class GLRParser {
public:
  GLRParser(const CFG &cfg);
  bool parse(const std::string &input);
  void reset(const std::string &input);
  bool nextStep();
  bool isDone() const;
  bool isAccepted() const;

  // Add these member variables:
  std::vector<int> parsingStack;

  std::vector<std::string> stepExplanations;
  std::vector<std::vector<int>> stackSnapshots;

private:
  const CFG &cfg;
  std::string startSymbol;
  std::set<std::string> nonTerminals;
  std::set<char> terminals;
  std::vector<GLRRule> rules;
  std::vector<LRState> states;
  std::map<std::pair<int,std::string>,int> gotoTable;
  std::map<std::pair<int,char>,LRAction> actionTable;


  bool isNonTerminal(const std::string &sym) const;
  bool isTerminal(char sym) const;

  void buildRules();
  LRState closure(const LRState &I);
  LRState go(const LRState &I, const std::string &X);
  int findOrAddState(const LRState &st);
  void buildLR0Automaton();
  void buildTables();
  bool glrParse(const std::string &input);

  void performShift(std::vector<std::shared_ptr<GSSNode>> &tops, int nextState);
  std::vector<std::shared_ptr<GSSNode>> performReduce(std::vector<std::shared_ptr<GSSNode>> &tops, int ruleId);

  // The LR stack of states
  bool finishedGLR = false;
  bool acceptedGLR = false;
  size_t currentPosGLR = 0;
  std::string currentInputGLR;

  // Explains each step (shift/reduce)


};

#endif
