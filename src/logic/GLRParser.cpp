#include "GLRParser.h"
#include <queue>
#include <stdexcept>
#include <algorithm>
#include <set>
// --------------------------------------------
// Implementation
// --------------------------------------------

GLRParser::GLRParser(const CFG &cfg) : cfg(cfg) {
  // Collect symbols:
  startSymbol = cfg.getStartSymbol();
  nonTerminals = cfg.getNonTerminals();
  terminals = cfg.getTerminals();
  // We'll treat '$' as the end marker:
  terminals.insert('$');

  buildRules();         // Build internal rules list (including augmented)
  buildLR0Automaton();  // Build the LR(0) states
  buildTables();        // Create SHIFT/REDUCE/ACCEPT actions
}

bool GLRParser::parse(const std::string &input) {
  reset(input);
  while(!isDone()) {
    nextStep();
  }
  return isAccepted();
}

// Step-by-step init
void GLRParser::reset(const std::string &input) {
  currentInput = input + "$";
  currentPos = 0;
  finished = false;
  accepted = false;
  stepExplanations.clear();

  // Clear GSS
  currentTops.clear();

  // Create an initial node for state 0
  auto root = std::make_shared<GSSNode>();
  root->state = 0;
  root->preds.clear();

  currentTops.push_back(root);

  // For debugging / visualization, store snapshots
  stackSnapshots.clear();
  stackSnapshots.resize(currentInput.size() + 1);
  stackSnapshots[0].topNodes = currentTops;
}

// Step-by-step iteration
bool GLRParser::nextStep() {
  if (finished) return false;
  if (currentPos >= currentInput.size()) {
    // We are at or beyond the end -> accept if possible
    // If a node has an ACCEPT action on '$', that means success
    bool foundAccept = false;
    for (auto &top : currentTops) {
      int st = top->state;
      auto it = actionTable.find({st, '$'});
      if (it != actionTable.end() && it->second.type == ActionType::Accept) {
        foundAccept = true;
        break;
      }
    }
    if (foundAccept) {
      accepted = true;
      stepExplanations.push_back("GLR: Accepted at end of input.");
    } else {
      stepExplanations.push_back("GLR: No more input, and no accept state. Rejected.");
    }
    finished = true;
    return false;
  }

  // Next input symbol:
  char a = currentInput[currentPos];

  // We'll track newly formed top nodes after SHIFT/REDUCE
  // Because we can expand by reduce multiple times and also have multiple SHIFT paths
  // We'll do an iterative approach until no more expansions.
  bool changedSomething = true;

  // We hold a queue of "active" GSS nodes to process for reduce,
  // because multiple reduces can happen from each top.
  std::queue<std::shared_ptr<GSSNode>> queue;
  for (auto &t : currentTops) queue.push(t);

  // We'll hold the next set of top nodes after SHIFT:
  std::vector<std::shared_ptr<GSSNode>> newTops = currentTops;

  // We'll do multiple passes for all possible reduce expansions
  // before we do SHIFT from each top.
  std::set<std::shared_ptr<GSSNode>> visited; // to avoid infinite loops on merges

  while (!queue.empty()) {
    auto node = queue.front();
    queue.pop();
    if (visited.count(node)) continue;
    visited.insert(node);

    int st = node->state;
    // 1) Check for ACCEPT on this node if a == '$' and dot is at end
    auto acceptIt = actionTable.find({st,'$'});
    if (a == '$' && acceptIt!=actionTable.end() && acceptIt->second.type == ActionType::Accept) {
      accepted = true;
      finished = true;
      stepExplanations.push_back("GLR: Accepted at pos " + std::to_string(currentPos));
      stackSnapshots[currentPos].topNodes = currentTops;
      return false;
    }

    // 2) Check for REDUCE on (st, a) or (st, '$')
    bool foundReduce = false;
    for (char maybeTerm : {a, '$'}) {
      auto it = actionTable.find({st, maybeTerm});
      if (it != actionTable.end() && it->second.type == ActionType::Reduce) {
        // We have a reduce by some rule
        int ruleId = it->second.stateOrRule;
        performReduce(node, ruleId);

        // After reduce, new GSS nodes might appear as new "tops."
        // We push them on queue for further expansions.
        // Because we store them in newTops in performReduce,
        // we do queue expansions below.
        foundReduce = true;
        // We don't break; we keep checking both a and '$' for reduce
      }
    }
    // If we did any reduce, new top nodes might have formed
    // We'll push them all to the queue to see if they can reduce further
    // (multiple reduce expansions in a row).
    if (foundReduce) {
      for (auto &tnew : stackSnapshots[currentPos].topNodes) {
        if (!visited.count(tnew)) {
          queue.push(tnew);
        }
      }
    }
  }

  // Now that we have done all possible reduces, let's SHIFT on 'a'
  // from every top node if SHIFT is valid.
  std::vector<std::shared_ptr<GSSNode>> shiftResults;
  for (auto &top : stackSnapshots[currentPos].topNodes) {
    int st = top->state;
    auto actIt = actionTable.find({st, a});
    if (actIt != actionTable.end() && actIt->second.type == ActionType::Shift) {
      int nextSt = actIt->second.stateOrRule;
      performShift(top, nextSt);
    }
  }

  // After SHIFT, we must do a round of reduces again
  // (some grammars have immediate reduce after shift).
  // We'll store the newly SHIFTed top nodes into a fresh container:
  std::set<std::shared_ptr<GSSNode>> shiftTops;
  for (auto &t : currentTops) {
    if (t->state == -1) continue; // if we used placeholders
    shiftTops.insert(t);
  }

  // The SHIFT has advanced currentPos by 1 symbol, so we do that at the *end*:
  currentPos++;

  // Now we do another reduce wave at *currentPos*.
  // Because after SHIFT, we are effectively at new position in the input.
  if (currentPos < stackSnapshots.size()) {
    // For the newly SHIFTed position, we do the same reduce expansions
    // but let's do them in the same pattern:
    std::queue<std::shared_ptr<GSSNode>> wave;
    for (auto &t : shiftTops) wave.push(t);

    std::set<std::shared_ptr<GSSNode>> visited2;
    while (!wave.empty()) {
      auto node = wave.front();
      wave.pop();
      if (visited2.count(node)) continue;
      visited2.insert(node);

      int st = node->state;
      // Possibly accept if a == '$' (not likely if we just SHIFTed, but for completeness):
      auto acceptIt = actionTable.find({st,'$'});
      if (acceptIt!=actionTable.end() && acceptIt->second.type == ActionType::Accept) {
        accepted = true;
        finished = true;
        stepExplanations.push_back("GLR: Accepted after shift at pos " + std::to_string(currentPos));
        stackSnapshots[currentPos].topNodes = currentTops;
        return false;
      }

      // Check reduce for "next input symbol" (which is currentInput[currentPos], if we haven't advanced further).
      if (currentPos < currentInput.size()) {
        char nextSym = currentInput[currentPos];
        for (char maybeTerm : {nextSym, '$'}) {
          auto it = actionTable.find({st, maybeTerm});
          if (it != actionTable.end() && it->second.type == ActionType::Reduce) {
            int ruleId = it->second.stateOrRule;
            performReduce(node, ruleId);
            // queue newly formed tops
            for (auto &nt : stackSnapshots[currentPos].topNodes) {
              if (!visited2.count(nt)) {
                wave.push(nt);
              }
            }
          }
        }
      } else {
        // If we are at end, check reduce on '$'
        auto it = actionTable.find({st, '$'});
        if (it != actionTable.end() && it->second.type == ActionType::Reduce) {
          performReduce(node, it->second.stateOrRule);
          for (auto &nt : stackSnapshots[currentPos].topNodes) {
            if (!visited2.count(nt)) {
              wave.push(nt);
            }
          }
        }
      }
    }
  }

  // Save the final top nodes for this position:
  if (currentPos < stackSnapshots.size()) {
    stackSnapshots[currentPos].topNodes = currentTops;
  }

  // If after all SHIFT/REDUCE expansions, we have no top nodes, it's a reject
  if (stackSnapshots[currentPos].topNodes.empty()) {
    stepExplanations.push_back("GLR: No valid configurations at pos " + std::to_string(currentPos) + ". Rejected.");
    finished = true;
    accepted = false;
    return false;
  }

  // If not at end, we continue
  if (currentPos >= currentInput.size()) {
    // We might have ended exactly on the '$', check acceptance:
    bool foundAccept = false;
    for (auto &top : currentTops) {
      auto it = actionTable.find({top->state, '$'});
      if (it != actionTable.end() && it->second.type == ActionType::Accept) {
        foundAccept = true;
        break;
      }
    }
    if (foundAccept) {
      accepted = true;
      stepExplanations.push_back("GLR: Accepted exactly at input end pos=" + std::to_string(currentPos));
    } else {
      stepExplanations.push_back("GLR: Input ended, but no accept. Rejected.");
    }
    finished = true;
  }

  return !finished;
}

/****************************************************
 * Implementation Details
 ****************************************************/

void GLRParser::buildRules() {
  // First, build an augmented rule: S' -> S
  GLRRule aug;
  aug.head = startSymbol + "'";
  aug.body.push_back(startSymbol);
  aug.id = 0;
  rules.push_back(aug);

  int ruleCount = 1;
  // Next, gather rules from the CFG
  for (auto &pr : cfg.getProductionRules()) {
    const std::string &A = pr.first;
    for (auto &rhs : pr.second) {
      GLRRule r;
      r.head = A;
      r.id = ruleCount++;
      // For each char in rhs, store as symbol
      for (char c : rhs) {
        r.body.push_back(std::string(1, c));
      }
      rules.push_back(r);
    }
  }
}

LRState GLRParser::closure(const LRState &I) {
  LRState result = I;
  bool changed = true;
  while (changed) {
    changed = false;
    // snapshot the items so we can iterate
    auto itemsVec = std::vector<LRItem>(result.items.begin(), result.items.end());
    for (auto &it : itemsVec) {
      int ruleId = it.ruleId;
      size_t dot = it.dotPos;
      if (dot < rules[ruleId].body.size()) {
        // next symbol
        std::string X = rules[ruleId].body[dot];
        if (isNonTerminal(X)) {
          // for each rule that has X as head, add (ruleX, dot=0)
          for (auto &r : rules) {
            if (r.head == X) {
              LRItem ni{r.id, 0};
              if (result.items.insert(ni).second) {
                changed = true;
              }
            }
          }
        }
      }
    }
  }
  return result;
}

LRState GLRParser::goTo(const LRState &I, const std::string &X) {
  LRState dst;
  for (auto &item : I.items) {
    const GLRRule &rule = rules[item.ruleId];
    if (item.dotPos < rule.body.size() && rule.body[item.dotPos] == X) {
      LRItem ni{ item.ruleId, item.dotPos + 1 };
      dst.items.insert(ni);
    }
  }
  if (!dst.items.empty()) {
    dst = closure(dst);
  }
  return dst;
}

int GLRParser::findOrAddState(const LRState &st) {
  // see if st is in states
  for (int i = 0; i < (int)states.size(); i++) {
    if (states[i] == st) {
      return i;
    }
  }
  states.push_back(st);
  return (int)states.size() - 1;
}

void GLRParser::buildLR0Automaton() {
  // Create initial state I0
  LRState I0;
  // Augmented rule is #0: S'->S
  // we add item (ruleId=0, dotPos=0)
  I0.items.insert({0,0});
  I0 = closure(I0);

  states.clear();
  int s0 = findOrAddState(I0);

  // BFS or queue approach
  std::queue<int> Q;
  Q.push(s0);

  // gather all possible symbols
  std::set<std::string> allSymbols;
  for (auto &r : rules) {
    for (auto &b : r.body) {
      allSymbols.insert(b);
    }
  }

  while (!Q.empty()) {
    int s = Q.front(); Q.pop();
    for (auto &X : allSymbols) {
      LRState nxt = goTo(states[s], X);
      if (!nxt.items.empty()) {
        int idx = findOrAddState(nxt);
        // if new, push it
        if (idx == (int)states.size()-1 && idx != s) {
          Q.push(idx);
        }
      }
    }
  }
}

void GLRParser::buildTables() {
  // build SHIFT, REDUCE, ACCEPT
  // For each state:
  for (int i = 0; i < (int)states.size(); i++) {
    const LRState &st = states[i];
    // For each item in st:
    for (auto &item : st.items) {
      const GLRRule &r = rules[item.ruleId];
      // If dotPos < length, SHIFT if next symbol is terminal
      if (item.dotPos < r.body.size()) {
        std::string X = r.body[item.dotPos];
        // SHIFT
        if (X.size() == 1 && isTerminal(X[0])) {
          LRState nxt = goTo(st, X);
          if (!nxt.items.empty()) {
            // find the state
            int ns = -1;
            for (int j = 0; j < (int)states.size(); j++) {
              if (states[j] == nxt) {
                ns = j;
                break;
              }
            }
            if (ns < 0) {
              // In a fully correct GLR, we can create a new state on the fly
              // But we found a missing state => add it
              ns = (int)states.size();
              states.push_back(nxt);
            }
            LRAction a;
            a.type = ActionType::Shift;
            a.stateOrRule = ns;
            actionTable[{ i, X[0] }] = a;
          }
        }
        // GOTO if X is nonterminal
        else if (isNonTerminal(X)) {
          LRState nxt = goTo(st, X);
          if (!nxt.items.empty()) {
            int ns = -1;
            for (int j = 0; j < (int)states.size(); j++) {
              if (states[j] == nxt) {
                ns = j;
                break;
              }
            }
            if (ns < 0) {
              ns = (int)states.size();
              states.push_back(nxt);
            }
            gotoTable[{ i, X }] = ns;
          }
        }
      } else {
        // dot is at the end => REDUCE
        if (r.id == 0) {
          // if it's the augmented rule and dot at end => ACCEPT
          LRAction a;
          a.type = ActionType::Accept;
          a.stateOrRule = -1;
          // accept for terminal = '$'
          actionTable[{ i, '$' }] = a;
        } else {
          // normal reduce
          LRAction a;
          a.type = ActionType::Reduce;
          a.stateOrRule = r.id;

          // The typical approach is to reduce on *all terminals*, plus '$'.
          // In a true LR(0) parser, we'd typically do LR(1) lookahead sets.
          // For GLR, we'll assign reduce to every possible terminal & '$'.
          for (char t : terminals) {
            // If SHIFT or ACCEPT is also set, we have a conflict => GLR can handle it by forking
            // but this code merges them in one table, which we do by overwriting or ignoring conflicts
            // For a complete solution, you’d store multiple actions in a conflict scenario.
            actionTable[{ i, t }] = a;
          }
        }
      }
    }
  }
}

/****************************************************
 * GLR Step Internals
 ****************************************************/

void GLRParser::performShift(std::shared_ptr<GSSNode> top, int nextState) {
  // SHIFT: create or find a GSS node for nextState, with predecessor= top
  auto newNode = findOrCreateGSSNode(nextState, {top});
  // Add newNode to currentTops (the top set for the new position).
  // We'll do that in stackSnapshots[currentPos+1] typically,
  // but let's unify: we store them in currentTops as well.
  currentTops.push_back(newNode);

  std::ostringstream msg;
  msg << "GLR: SHIFT from state " << top->state << " to state " << nextState;
  stepExplanations.push_back(msg.str());
}

void GLRParser::performReduce(std::shared_ptr<GSSNode> top, int ruleId) {
  // REDUCE: pop as many symbols as the body length, then goto
  const GLRRule &r = rules[ruleId];
  int popCount = (int)r.body.size();

  // We'll do a BFS from top's predecessors up to popCount levels
  // but in a GSS, multiple paths might exist. We collect all states
  // that are exactly popCount edges above.

  // We create a queue of (node, remaining pops)
  struct Frame {
    std::shared_ptr<GSSNode> node;
    int remain;
  };
  std::queue<Frame> Q;
  Q.push({top, popCount});

  // We'll collect all predecessor states that are exactly popCount up
  std::vector<std::shared_ptr<GSSNode>> reduceSources;

  while (!Q.empty()) {
    auto f = Q.front();
    Q.pop();
    if (f.remain == 0) {
      // This node is a final candidate
      reduceSources.push_back(f.node);
    } else {
      // push all preds with remain-1
      for (auto &p : f.node->preds) {
        Q.push({p, f.remain - 1});
      }
    }
  }

  // Now, from each reduceSources node, we do a GOTO on r.head
  for (auto &src : reduceSources) {
    int st = src->state;
    auto itGoto = gotoTable.find({ st, r.head });
    if (itGoto == gotoTable.end()) {
      // In a fully general GLR, might create new state on the fly
      // or skip if no valid goto
      continue;
    }
    int nextSt = itGoto->second;
    auto newNode = findOrCreateGSSNode(nextSt, {src});
    currentTops.push_back(newNode);

    std::ostringstream msg;
    msg << "GLR: REDUCE by rule " << r.id << " (" << r.head << " -> ";
    for (auto &x : r.body) msg << x;
    msg << "), goto state " << nextSt;
    stepExplanations.push_back(msg.str());
  }

  // We unify merges in findOrCreateGSSNode.
}

std::shared_ptr<GSSNode> GLRParser::findOrCreateGSSNode(int state, const std::vector<std::shared_ptr<GSSNode>> &preds) {
  // For performance, you’d keep a cache of existing GSSNodes keyed by (state, set-of-preds).
  // For simplicity, we just create a new node and possibly unify if we find an identical existing top.
  // We'll unify if same state and same set of preds.
  // Real Tomita merges them if same <state> but let’s do a simpler approach.

  // 1) Check if there's already a top node with the same state
  for (auto &t : currentTops) {
    if (t->state == state) {
      // Merge preds
      for (auto &p : preds) {
        // Avoid duplicates
        bool alreadyThere = false;
        for (auto &ex : t->preds) {
          if (ex == p) { alreadyThere = true; break; }
        }
        if (!alreadyThere) {
          t->preds.push_back(p);
        }
      }
      // Return the existing node
      return t;
    }
  }

  // 2) Not found => create a new node
  auto node = std::make_shared<GSSNode>();
  node->state = state;
  node->preds = preds;
  return node;
}