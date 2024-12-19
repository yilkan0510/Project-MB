#ifndef PROGRAMEEROPDRACHT1_CFG_H
#define PROGRAMEEROPDRACHT1_CFG_H

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <set>
#include <iomanip>
#include <fstream>
#include "../json.hpp"

using namespace std;
using namespace nlohmann;

class CFG {
private:
  string startSymbol;
    int postUnitProdCount;
    int postUselessProdCount;

    void eliminateEpsilonProductions();
    void eliminateUnitProductions();
    void removeUselessSymbols();
    void replaceTerminalsInBadBodies();
    void breakLongBodies();

    void generateParsePaths(const string &current, const string &target, vector<string> &paths, string path);

public:
    CFG(string Filename);

    void print();
    void toCNF(); // Voegt de CNF-conversiemethode toe

    bool isAmbiguous(const string &testString);

    void setStartSymbol(const string &symbol);

    const map<string, vector<string>>& getProductionRules() const;
    const set<string>& getNonTerminals() const;
    const set<char>& getTerminals() const;
    const string& getStartSymbol() const;



    set<string> nonTerminals;
    set<char> terminals;
    map<string, vector<string>> productionRules;
};

#endif //PROGRAMEEROPDRACHT1_CFG_H