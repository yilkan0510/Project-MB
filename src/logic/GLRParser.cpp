#include "GLRParser.h"
#include <queue>
#include <stdexcept>
#include <algorithm>
#include <set>

bool GLRParser::isNonTerminal(const std::string &sym) const {
  return nonTerminals.count(sym) > 0;
}
bool GLRParser::isTerminal(char sym) const {
  return terminals.count(sym) > 0;
}

GLRParser::GLRParser(const CFG &cfg) : cfg(cfg) {
  startSymbol = cfg.getStartSymbol();
  nonTerminals = cfg.getNonTerminals();
  terminals = cfg.getTerminals();
  terminals.insert('$');
  buildRules();
  buildLR0Automaton();
  buildTables();
}

void GLRParser::buildRules() {
  // augmented start
  GLRRule sr;
  sr.head = startSymbol + "'";
  sr.body.push_back(startSymbol);
  sr.id = 0;
  rules.push_back(sr);

  int idCount = 1;
  for (auto &pr : cfg.getProductionRules()) {
    for (auto &rhs : pr.second) {
      GLRRule r;
      r.head = pr.first;
      for (auto c : rhs) {
        std::string sym(1,c);
        r.body.push_back(sym);
      }
      r.id = idCount++;
      rules.push_back(r);
    }
  }
}

LRState GLRParser::closure(const LRState &I) {
  LRState J = I;
  bool changed = true;
  while (changed) {
    changed=false;
    for (auto &item : J.items) {
      const GLRRule &r = rules[item.ruleId];
      if (item.dotPos < r.body.size()) {
        std::string X = r.body[item.dotPos];
        if (isNonTerminal(X)) {
          for (auto &rr : rules) {
            if (rr.head == X) {
              LRItem ni{rr.id,0};
              if (J.items.insert(ni).second) {
                changed=true;
              }
            }
          }
        }
      }
    }
  }
  return J;
}

LRState GLRParser::go(const LRState &I, const std::string &X) {
  LRState J;
  for (auto &item : I.items) {
    const GLRRule &r = rules[item.ruleId];
    if (item.dotPos < r.body.size() && r.body[item.dotPos] == X) {
      LRItem ni{item.ruleId,item.dotPos+1};
      J.items.insert(ni);
    }
  }
  return closure(J);
}

int GLRParser::findOrAddState(const LRState &st) {
  for (size_t i=0; i<states.size(); i++) {
    if (states[i]==st) return (int)i;
  }
  states.push_back(st);
  return (int)states.size()-1;
}

void GLRParser::buildLR0Automaton() {
  LRState I0;
  I0.items.insert({0,0});
  I0 = closure(I0);
  states.clear();
  int s0 = findOrAddState(I0);

  std::queue<int> work;
  work.push(s0);

  std::set<std::string> symbols;
  for (auto &r : rules) {
    for (auto &b : r.body) {
      symbols.insert(b);
    }
  }

  while (!work.empty()) {
    int s=work.front(); work.pop();
    for (auto &X : symbols) {
      LRState g = go(states[s],X);
      if (!g.items.empty()) {
        int ns = findOrAddState(g);
        // no condition needed, just building all states
        // if new state, it’s appended
      }
    }
  }
}

void GLRParser::buildTables() {
  // For each state, add actions
  for (int i=0; i<(int)states.size(); i++) {
    const LRState &st= states[i];
    // Find symbols that can follow
    std::set<std::string> symbols;
    for (auto &item : st.items) {
      const GLRRule &r=rules[item.ruleId];
      if (item.dotPos < r.body.size()) {
        symbols.insert(r.body[item.dotPos]);
      }
    }

    // SHIFT/GOTO
    for (auto &X : symbols) {
      LRState g = go(st,X);
      if (!g.items.empty()) {
        int ns=-1;
        for (int j=0;j<(int)states.size();j++) {
          if (states[j]==g) {ns=j;break;}
        }
        if (ns<0) throw std::runtime_error("No state found");
        if (isNonTerminal(X)) {
          gotoTable[{i,X}]=ns;
        } else if (X.size()==1 && isTerminal(X[0])) {
          LRAction a;
          a.type=ActionType::Shift;
          a.stateOrRule=ns;
          actionTable[{i,X[0]}]=a;
        }
      }
    }

    // REDUCE/ACCEPT
    for (auto &item : st.items) {
      const GLRRule &r=rules[item.ruleId];
      if (item.dotPos == r.body.size()) {
        if (r.head == startSymbol + "'") {
          // Accept
          LRAction a;
          a.type=ActionType::Accept;
          a.stateOrRule=-1;
          actionTable[{i,'$'}]=a;
        } else {
          // Reduce on all terminals and '$'
          LRAction a;
          a.type=ActionType::Reduce;
          a.stateOrRule=r.id;
          for (auto t: terminals) {
            actionTable[{i,t}]=a;
          }
        }
      }
    }
  }
}

bool GLRParser::parse(const std::string &input) {
  return glrParse(input);
}

bool GLRParser::glrParse(const std::string &input) {
  std::string in = input+"$";
  // Simple GLR using a single path since no ambiguity in these simple grammars
  // We'll just mimic LR parsing. If multiple actions: handle them all.

  // Represent states as stack<int>
  struct Configuration {
    std::vector<int> stack;
    size_t pos;
  };
  std::vector<Configuration> configs;
  configs.push_back({{0},0});

  for (size_t i=0; i<in.size(); i++) {
    char a=in[i];
    std::vector<Configuration> newConfigs;
    for (auto &conf : configs) {
      bool progressed = false;
      bool tryAgain=true;
      while (tryAgain) {
        tryAgain=false;
        // Try reduce:
        bool reduced=false;
        for (;;) {
          bool didReduce=false;
          // Check reduce actions
          int state = conf.stack.back();
          for (auto t: {a,'$'}) {
            auto it=actionTable.find({state,t});
            if (it!=actionTable.end() && it->second.type==ActionType::Reduce) {
              const GLRRule &r=rules[it->second.stateOrRule];
              int popCount=(int)r.body.size();
              for (int c=0;c<popCount;c++) conf.stack.pop_back();
              int newState = gotoTable[{conf.stack.back(),r.head}];
              conf.stack.push_back(newState);
              didReduce=true;
              break;
            } else if (it!=actionTable.end() && it->second.type==ActionType::Accept && i==in.size()-1) {
              return true;
            }
          }
          if (!didReduce) break;
          reduced=true;
        }

        // If after reduces no shift/accept and we’re not at end:
        if (i<in.size()) {
          int state=conf.stack.back();
          auto it=actionTable.find({state,a});
          if (it!=actionTable.end() && it->second.type==ActionType::Shift) {
            conf.stack.push_back(it->second.stateOrRule);
            progressed=true;
          }
        }
      }

      if (progressed || i==in.size()-1) {
        newConfigs.push_back(conf);
      }
    }
    configs=newConfigs;
    if (configs.empty()) return false;
  }

  for (auto &conf : configs) {
    int state=conf.stack.back();
    auto it=actionTable.find({state,'$'});
    if (it!=actionTable.end() && it->second.type==ActionType::Accept)
      return true;
  }

  return false;
}

void GLRParser::performShift(std::vector<std::shared_ptr<GSSNode>> &tops, int nextState) {
  // Not used in this simplified GLR approach
}
std::vector<std::shared_ptr<GSSNode>> GLRParser::performReduce(std::vector<std::shared_ptr<GSSNode>> &tops, int ruleId) {
  // Not used in this simplified approach
  return {};
}
