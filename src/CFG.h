#ifndef PROGRAMEEROPDRACHT1_CFG_H
#define PROGRAMEEROPDRACHT1_CFG_H

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <set>
#include <iomanip>
#include <fstream>
#include "json.hpp"

using namespace std;
using namespace nlohmann;

class CFG {
private:
    set<string> nonTerminals;
    set<char> terminals;
    map<string, vector<string>> productionRules;
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
};

#endif //PROGRAMEEROPDRACHT1_CFG_H