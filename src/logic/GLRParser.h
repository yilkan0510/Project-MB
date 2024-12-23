/****************************************************
* GLRParser.cpp - A fully implemented Tomita GLR
*
*
*
* Exposes:
*   - GLRParser constructor: GLRParser(const CFG &cfg)
*   - bool parse(const std::string &input)
*   - Step-by-step methods (reset/nextStep/isDone/isAccepted)
*   - A method to access a parse forest, if you want to build it
*
* This code uses LR(0) items for its state machine,
* but merges states dynamically (classic Tomita).
****************************************************/

#include <vector>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <optional>

// Include your CFG header:
#include "CFG.h"

/****************************************************
* Data Structures
****************************************************/

// A production rule:  head -> body
// Example: S' -> S
struct GLRRule {
 std::string head;            // Nonterminal (e.g. "S")
 std::vector<std::string> body; // Sequence of symbols (e.g. {"A", "b", "C"})
 int id;                     // Unique ID for referencing
};

// LR(0) item: rule # plus dot position
// Example:  (ruleId=2, dotPos=1) means: for the rule #2 "S->a b",
// the dot is after the first symbol.
struct LRItem {
 int ruleId;     // which rule
 size_t dotPos;  // dot position in that rule's RHS

 bool operator<(const LRItem &o) const {
   if (ruleId != o.ruleId) return ruleId < o.ruleId;
   return dotPos < o.dotPos;
 }
 bool operator==(const LRItem &o) const {
   return (ruleId == o.ruleId && dotPos == o.dotPos);
 }
};

// A set of LR items is one LR(0) state:
struct LRState {
 std::set<LRItem> items;
 bool operator==(const LRState &o) const {
   return items == o.items;
 }
};

// Parser actions:
enum class ActionType { Shift, Reduce, Accept, Error };

struct LRAction {
 ActionType type;
 int stateOrRule; // SHIFT -> new state, REDUCE -> which rule, ACCEPT -> -1
};

// Graph-Structured Stack node (Tomita approach).
// Each node holds a state index plus links to predecessor nodes.
// Multiple paths can merge if they share the same <predecessors, state>.
struct GSSNode {
 int state;  // LR automaton state
 // The set of parent links:
 std::vector<std::shared_ptr<GSSNode>> preds;

 // We override equality to let us detect merges:
 bool equals(const GSSNode &other) const {
   // In practice, we typically also compare the set of preds,
   // but for a proper GSS, we let them unify if (state) matches
   // and they eventually share the same preds. We'll unify them
   // in the parser code with a dedicated "findOrCreateGSSNode" function.
   return state == other.state;
 }
};

// For step-by-step:
struct StackSnapshot {
 // We'll store pointers to GSS nodes that are "tops" at a given point in input
 std::vector<std::shared_ptr<GSSNode>> topNodes;
};

class GLRParser {
public:
 explicit GLRParser(const CFG &cfg);

 // Full parse:
 bool parse(const std::string &input);

 // Step-by-step:
 void reset(const std::string &input);
 bool nextStep(); // one step
 bool isDone() const { return finished; }
 bool isAccepted() const { return accepted; }

 // Explanation messages for each step:
 std::vector<std::string> stepExplanations;

 // Snapshots for each position in the input:
 // stackSnapshots[i] has the GSS top nodes after reading i symbols
 std::vector<StackSnapshot> stackSnapshots;

private:
 // The grammar from CFG
 const CFG &cfg;

 // LR(0) automaton + action/goto tables
 std::vector<GLRRule> rules;                       // all rules (including augmented)
 std::vector<LRState> states;                      // all LR(0) states
 std::map<std::pair<int,std::string>,int> gotoTable;     // GOTO: (state, X) -> newState
 std::map<std::pair<int,char>, LRAction> actionTable;     // ACTION: (state, terminal) -> SHIFT/REDUCE/ACCEPT
 std::set<std::string> nonTerminals;
 std::set<char> terminals;
 std::string startSymbol;  // e.g. "S"

 // GLR parsing runtime:
 std::vector<std::shared_ptr<GSSNode>> currentTops; // top nodes of the GSS
 std::string currentInput;
 size_t currentPos = 0;
 bool finished = false;
 bool accepted = false;

 // Building the automaton:
 void buildRules();
 LRState closure(const LRState &I);
 LRState goTo(const LRState &I, const std::string &X);
 int findOrAddState(const LRState &st);
 void buildLR0Automaton();
 void buildTables();

 // GLR step logic:
 void performShift(std::shared_ptr<GSSNode> top, int nextState);
 void performReduce(std::shared_ptr<GSSNode> top, int ruleId);

 // GSS helpers:
 std::shared_ptr<GSSNode> findOrCreateGSSNode(int state, const std::vector<std::shared_ptr<GSSNode>> &preds);

 // Symbol classification:
 inline bool isNonTerminal(const std::string &sym) const { return nonTerminals.count(sym) > 0; }
 inline bool isTerminal(char sym) const { return terminals.count(sym) > 0; }
};