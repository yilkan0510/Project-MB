/**************************************************
* EarleyParser.cpp - Rewritten Full Earley Parser
*
* Usage:
*   #include "EarleyParser.cpp" // or compile separately
*   EarleyParser parser(cfg);
*   bool ok = parser.parse("abba");
*
* Features:
*   - Step-by-step or one-shot parse
*   - Chart-based approach
*   - Single-character tokens
*   - Augmented grammar for acceptance
*   - Detailed stepExplanations for each stage
**************************************************/

#include <vector>
#include <set>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iostream>

// Include your existing CFG class header:
#include "CFG.h"

/**************************************************
* Data Structures
**************************************************/

// An Earley item: A -> α • β, startIndex
// `head` = A, `body` = αβ, `dotPos` is where the "•" is.
// `startIdx` is which chart column the item originated from.
struct EarleyItem {
 std::string head;  // e.g. "S"
 std::string body;  // e.g. "aB" (string of symbols)
 size_t dotPos;     // dot position in the body
 size_t startIdx;   // index of the chart where this item began

 bool operator<(const EarleyItem &o) const {
   if (head != o.head) return head < o.head;
   if (body != o.body) return body < o.body;
   if (dotPos != o.dotPos) return dotPos < o.dotPos;
   return startIdx < o.startIdx;
 }

 bool operator==(const EarleyItem &o) const {
   return head == o.head && body == o.body &&
          dotPos == o.dotPos && startIdx == o.startIdx;
 }
};

/**************************************************
* Earley Parser Class
**************************************************/
class EarleyParser {
public:
 // Construct with reference to a CFG
 explicit EarleyParser(const CFG &cfg);

 // Parse the entire string at once
 bool parse(const std::string &input);

 // Step-by-step interface
 void reset(const std::string &input);
 bool nextStep(); // advances one step
 bool isDone() const { return finished; }
 bool isAccepted() const { return accepted; }

 // Return the chart for external visualization
 // chart[i] = set of items after i tokens consumed
 const std::vector<std::set<EarleyItem>>& getChart() const { return chart; }

 // Logging/explanations for each step
 std::vector<std::string> stepExplanations;

private:
 const CFG &cfg;

 // Storing the grammar’s start symbol + an augmented start symbol
 std::string startSymbol;     // e.g. "S"
 std::string augmentedSymbol; // e.g. "S'"

 // The input string (plus we handle it char-by-char)
 std::string currentInput;

 // The chart: for an input of length n, we have chart[0..n]
 std::vector<std::set<EarleyItem>> chart;

 // The current position in the input
 size_t currentPos = 0;

 // Whether we've finished this parse and accepted or not
 bool finished = false;
 bool accepted = false;

 // Helpers for scanning, predicting, completing
 bool isNonTerminal(const std::string &symbol) const;
 bool isTerminal(char symbol) const;

 // Step subroutines
 void scan(char nextChar);
 void predictAndComplete(size_t pos);
};
