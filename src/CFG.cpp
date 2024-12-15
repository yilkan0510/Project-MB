//
// Created by yilka on 10/4/2024.
//

#include "CFG.h"
#include <regex>
#include <queue>

CFG::CFG(string Filename) {
    ifstream input(Filename);
    if (!input) {
        cerr << "Unable to open file " << Filename << endl;
        exit(1);
    }
    json j;
    input >> j;

    // nonterminals
    nonTerminals = j["Variables"].get<set<string>>();

    // terminals
    for (const auto& terminal : j["Terminals"]) {
        terminals.insert(terminal.get<string>()[0]);
    }

    // productionRules
    for (const auto& production : j["Productions"]) {
        string head = production["head"];
        string body = "";

        for (int i = 0; i < production["body"].size(); ++i) {
            body += production["body"][i].get<string>();
            if (i != production["body"].size() - 1) {
                body += " ";
            }
        }

        // Add the production rule to the map
        productionRules[head].push_back(body);
    }

    // startSymbol
    startSymbol = j["Start"].get<string>();

}

void CFG::print() {
    // Print non-terminals
    cout << "V = {";
    for (auto it = nonTerminals.begin(); it != nonTerminals.end(); ++it) {
        cout << *it;
        if (next(it) != nonTerminals.end()) cout << ", ";
    }
    cout << "}" << endl;

    // Print terminals
    cout << "T = {";
    for (auto it = terminals.begin(); it != terminals.end(); ++it) {
        cout << *it;
        if (next(it) != terminals.end()) cout << ", ";
    }
    cout << "}" << endl;

    // Verzamel en sorteer productie regels
    cout << "P = {" << endl;
    vector<string> productionStrings;
    for (const auto& rule : productionRules) {
        for (const auto& prod : rule.second) {
            string productionStr = "  " + rule.first + " -> `" + prod + "`\n";
            productionStrings.push_back(productionStr);
        }
    }

    // Sorteer de productie strings op ASCII-volgorde
    sort(productionStrings.begin(), productionStrings.end());

    // Print de gesorteerde productie strings
    for (const auto& prodStr : productionStrings) {
        cout << prodStr;
    }
    cout << "}" << endl;

    // Print start symbol
    cout << "S = " << startSymbol << endl;
}





void CFG::eliminateEpsilonProductions() {
    set<string> nullable;

    // Stap 1: Bepaal nullable variabelen
    for (const auto& rule : productionRules) {
        for (const auto& body : rule.second) {
            if (body.empty()) {
                nullable.insert(rule.first);
            }
        }
    }

    bool changed;
    do {
        changed = false;
        for (const auto& rule : productionRules) {
            for (const auto& body : rule.second) {
                bool allNullable = true;
                for (char symbol : body) {
                    if (!nullable.count(string(1, symbol))) {
                        allNullable = false;
                        break;
                    }
                }
                if (allNullable && nullable.insert(rule.first).second) {
                    changed = true;
                }
            }
        }
    } while (changed);

    // Log de nullables
    cout << "  Nullables are {";
    for (auto it = nullable.begin(); it != nullable.end(); ++it) {
        cout << *it;
        if (next(it) != nullable.end()) cout << ", ";
    }
    cout << "}\n";

    // Stap 2: Creëer nieuwe producties door nullable variabelen te verwijderen
    map<string, vector<string>> newProductions;
    for (const auto& rule : productionRules) {
        for (const auto& body : rule.second) {
            vector<string> bodies = {body};

            // Verwijder nullable variabelen
            for (size_t i = 0; i < body.size(); ++i) {
                if (nullable.count(string(1, body[i]))) {
                    size_t currentSize = bodies.size();
                    for (size_t j = 0; j < currentSize; ++j) {
                        string newBody = bodies[j];
                        newBody.erase(remove(newBody.begin(), newBody.end(), body[i]), newBody.end());

                        // Voeg alleen unieke en niet-lege producties toe, zonder dubbele spaties
                        newBody.erase(unique(newBody.begin(), newBody.end(), [](char a, char b) { return a == ' ' && b == ' '; }), newBody.end());
                        if (!newBody.empty() && find(bodies.begin(), bodies.end(), newBody) == bodies.end()) {
                            bodies.push_back(newBody);
                        }
                    }
                }
            }

            // Verwijder extra spaties en voeg producties toe
            for (auto& newBody : bodies) {
                newBody = regex_replace(newBody, regex("^ +| +$|( ) +"), "$1");  // Verwijder begin/eindspaties en dubbele spaties
                if (!newBody.empty()) newProductions[rule.first].push_back(newBody);
            }
        }
    }

    // Stap 3: Verwijder lege producties en sorteer producties voor ASCII-volgorde
    for (auto& rule : newProductions) {
        auto& bodies = rule.second;
        bodies.erase(remove(bodies.begin(), bodies.end(), ""), bodies.end());
    }

    // Stap 4: Log productietellingen
    size_t originalProdCount = 0;
    for (const auto& rule : productionRules) {
        originalProdCount += rule.second.size();
    }
    size_t newProdCount = 0;
    for (const auto& rule : newProductions) {
        newProdCount += rule.second.size();
    }
    cout << "  Created " << newProdCount << " productions, original had " << originalProdCount << "\n\n";

    // Update de productie regels
    productionRules = newProductions;
}




void CFG::eliminateUnitProductions() {
    std::set<std::pair<std::string, std::string>> unitPairs;
    std::set<std::pair<std::string, std::string>> directUnitPairs;
    for (const auto& nt : nonTerminals) {
        unitPairs.insert({nt, nt});
    }
    for (const auto& rule : productionRules) {
        for (const auto& prod : rule.second) {
            if (nonTerminals.count(prod)) {
                unitPairs.insert({rule.first, prod});
                directUnitPairs.insert({rule.first, prod});
            }
        }
    }

    bool changed;
    do {
        changed = false;
        for (const auto& [A, B] : unitPairs) {
            for (const auto& [C, D] : unitPairs) {
                if (B == C && unitPairs.insert({A, D}).second) {
                    changed = true;
                }
            }
        }
    } while (changed);

    std::set<std::pair<std::string, std::string>> uniqueProductions;
    int originalProdCount = 0;
    for (const auto& rule : productionRules) {
        originalProdCount += rule.second.size();
        for (const auto& body : rule.second) {
            if (!nonTerminals.count(body)) {
                uniqueProductions.insert({rule.first, body});
            }
        }
    }

    for (const auto& [A, B] : unitPairs) {
        for (const auto& body : productionRules[B]) {
            if (!nonTerminals.count(body)) {
                uniqueProductions.insert({A, body});
            }
        }
    }

    // Converteer unieke producties terug naar map-structuur
    productionRules.clear();
    for (const auto& [head, body] : uniqueProductions) {
        productionRules[head].push_back(body);
    }

    postUnitProdCount = uniqueProductions.size();

    std::cout << " >> Eliminating unit pairs\n";
    std::cout << "  Found " << directUnitPairs.size() << " unit productions\n";
    std::cout << "  Unit pairs: {";
    for (auto it = unitPairs.begin(); it != unitPairs.end(); ++it) {
        std::cout << "(" << it->first << ", " << it->second << ")";
        if (std::next(it) != unitPairs.end()) std::cout << ", ";
    }
    std::cout << "}\n";
    std::cout << "  Created " << uniqueProductions.size() << " new productions" << ", original had " << originalProdCount << "\n";

}



void CFG::removeUselessSymbols() {
    int initialVariableCount = nonTerminals.size();
    int initialProdCount = postUnitProdCount;
    int initialTerminalCount = terminals.size();

    // Stap 1: Genereerbare symbolen vinden (inclusief terminals)
    set<string> generatingSymbols;
    for (char terminal : terminals) {
        generatingSymbols.insert(string(1, terminal));
    }

    bool changed;
    do {
        changed = false;
        for (const auto& rule : productionRules) {
            const string& head = rule.first;
            for (const auto& body : rule.second) {
                bool allGenerating = true;
                for (char symbol : body) {
                    string symStr(1, symbol);
                    if (nonTerminals.count(symStr) && !generatingSymbols.count(symStr)) {
                        allGenerating = false;
                        break;
                    }
                }
                if (allGenerating && generatingSymbols.insert(head).second) {
                    changed = true;
                }
            }
        }
    } while (changed);

    // Niet-genereerbare symbolen verwijderen
    for (auto it = productionRules.begin(); it != productionRules.end();) {
        if (!generatingSymbols.count(it->first)) {
            it = productionRules.erase(it);
        } else {
            it->second.erase(remove_if(it->second.begin(), it->second.end(),
                                       [&](const string& body) {
                                           for (char symbol : body) {
                                               if (nonTerminals.count(string(1, symbol)) && !generatingSymbols.count(string(1, symbol))) {
                                                   return true;
                                               }
                                           }
                                           return false;
                                       }), it->second.end());
            ++it;
        }
    }

    // Bereikbare symbolen vinden (startend bij startSymbool)
    set<string> reachableSymbols = {startSymbol};
    queue<string> toProcess;
    toProcess.push(startSymbol);
    while (!toProcess.empty()) {
        string current = toProcess.front();
        toProcess.pop();
        for (const auto& body : productionRules[current]) {
            for (char symbol : body) {
                string symStr(1, symbol);
                if (terminals.count(symbol)) {
                    reachableSymbols.insert(symStr); // Voeg terminal toe als hij wordt aangetroffen
                } else if (nonTerminals.count(symStr) && reachableSymbols.insert(symStr).second) {
                    toProcess.push(symStr);
                }
            }
        }
    }

    // Niet-bereikbare symbolen verwijderen
    for (auto it = productionRules.begin(); it != productionRules.end();) {
        if (!reachableSymbols.count(it->first)) {
            it = productionRules.erase(it);
        } else {
            it->second.erase(remove_if(it->second.begin(), it->second.end(),[&](const string& body) {
                for (char symbol : body) {
                    if (nonTerminals.count(string(1, symbol)) && !reachableSymbols.count(string(1, symbol))) {
                        return true;
                    }
                }
                return false;
            }), it->second.end());
            ++it;
        }
    }

    // Verzamelen van overgebleven producties in een vector (inclusief duplicaten)
    vector<pair<string, string>> remainingProductions;
    for (const auto& rule : productionRules) {
        for (const auto& body : rule.second) {
            remainingProductions.push_back({rule.first, body});
        }
    }

    // Update nonTerminals met de uiteindelijke bruikbare symbolen (exclusief terminals)
    set<string> usefulSymbols;
    set_intersection(generatingSymbols.begin(), generatingSymbols.end(),
                     reachableSymbols.begin(), reachableSymbols.end(),
                     inserter(usefulSymbols, usefulSymbols.begin()));
    nonTerminals.clear();
    for (const auto& symbol : usefulSymbols) {
        if (!terminals.count(symbol[0])) { // Voeg alleen niet-terminals toe aan V
            nonTerminals.insert(symbol);
        }
    }

    // Print resultaten
    cout << " >> Eliminating useless symbols\n";
    cout << "  Generating symbols: {";
    for (auto it = generatingSymbols.begin(); it != generatingSymbols.end(); ++it) {
        cout << *it;
        if (next(it) != generatingSymbols.end()) cout << ", ";
    }
    cout << "}\n";

    cout << "  Reachable symbols: {";
    for (auto it = reachableSymbols.begin(); it != reachableSymbols.end(); ++it) {
        cout << *it;
        if (next(it) != reachableSymbols.end()) cout << ", ";
    }
    cout << "}\n";

    cout << "  Useful symbols: {";
    for (auto it = usefulSymbols.begin(); it != usefulSymbols.end(); ++it) {
        cout << *it;
        if (next(it) != usefulSymbols.end()) cout << ", ";
    }
    cout << "}\n";

    // Verwijderde producties berekenen
    int removedVariables = initialVariableCount - nonTerminals.size();
    int removedTerminals = initialTerminalCount - terminals.size();
    int remainingProdCount = 0;
    for (const auto& rule : productionRules) {
        remainingProdCount += rule.second.size();
    }
    int removedProductions = initialProdCount - remainingProdCount;
    postUselessProdCount = remainingProdCount;
    cout << "  Removed " << removedVariables << " variables, "<< removedTerminals <<" terminals and " << removedProductions << " productions\n\n";
}

void CFG::replaceTerminalsInBadBodies() {
    // Map of terminals to their corresponding non-terminals for direct replacements
    map<char, string> terminalToNonTerminal = {
            {'a', "A"},
            {'b', "B"}
    };

    map<char, string> terminalToVar;  // New variables for terminals without direct non-terminal replacements
    int newVariableCount = 0;

    // Process each production rule
    for (auto& rule : productionRules) {
        vector<string> updatedBodies;

        for (const string& body : rule.second) {
            // If the body is a single terminal, keep it as-is
            if (body.size() == 1 && terminals.count(body[0])) {
                updatedBodies.push_back(body);
                continue;
            }

            string newBody;
            bool bodyModified = false;
            set<char> uniqueTerminals;  // Track unique terminals in the body

            // First pass: collect unique terminals
            for (char symbol : body) {
                if (terminals.count(symbol)) {
                    uniqueTerminals.insert(symbol);
                }
            }

            // Determine if we need new variables
            bool useNewVariables = uniqueTerminals.size() > 1;

            // Process each symbol in the body
            for (char symbol : body) {
                if (terminals.count(symbol) && body.size() >= 2) {
                    if (useNewVariables) {
                        // Create a new variable for this terminal if needed
                        if (terminalToVar.find(symbol) == terminalToVar.end()) {
                            string newVar = "_" + string(1, symbol);
                            terminalToVar[symbol] = newVar;
                            nonTerminals.insert(newVar);
                            productionRules[newVar].push_back(string(1, symbol));
                            newVariableCount++;
                        }
                        newBody += terminalToVar[symbol];
                    } else if (terminalToNonTerminal.find(symbol) != terminalToNonTerminal.end()) {
                        // Replace with corresponding non-terminal if exists
                        newBody += terminalToNonTerminal[symbol];
                    }
                    bodyModified = true;
                } else {
                    newBody += symbol;
                }
            }

            // Add the modified or original body as needed
            updatedBodies.push_back(bodyModified ? newBody : body);
        }
        rule.second = updatedBodies;
    }

    // Print results
    cout << "    Added " << newVariableCount << " new variables: {";
    for (auto it = terminalToVar.begin(); it != terminalToVar.end(); ++it) {
        cout << it->second;
        if (next(it) != terminalToVar.end()) cout << ", ";
    }
    cout << "}" << endl;

    int totalProdCount = 0;
    for (const auto& rule : productionRules) {
        totalProdCount += rule.second.size();
    }
    cout << "    Created " << totalProdCount << " new productions, original had " << postUselessProdCount << "\n\n";
}





void CFG::breakLongBodies() {
    map<string, vector<string>> newProductions;
    map<string, int> varCount;  // Counter for each non-terminal to start from 2
    int brokeCount = 0;  // Counter to track how many bodies were broken down

    // Iterate over all production rules
    for (const auto& rule : productionRules) {
        const string& head = rule.first;
        varCount[head] = 1;  // Initialize the counter for this non-terminal to start from 1

        for (const auto& body : rule.second) {
            // Split the body into individual symbols
            vector<string> parts;
            istringstream iss(body);
            string part;
            while (iss >> part) {
                parts.push_back(part);
            }

            if (parts.size() > 2) {
                brokeCount++;
                string currentHead = head;

                // Process each segment of the body, creating a chain of new variables
                for (size_t i = 0; i < parts.size() - 2; ++i) {
                    string first = parts[i];
                    string newVar = head + "_" + to_string(++varCount[head]);  // Generate new variable, starting from 2
                    nonTerminals.insert(newVar);

                    // Add the current production as `currentHead -> first newVar`
                    newProductions[currentHead].push_back(first + " " + newVar);

                    // Move to the next part, updating the head to newVar
                    currentHead = newVar;
                }

                // The last segment (two symbols) is added as a final binary rule
                newProductions[currentHead].push_back(parts[parts.size() - 2] + " " + parts[parts.size() - 1]);

            } else {
                // If body has 2 or fewer symbols, add it directly without modification
                newProductions[head].push_back(body);
            }
        }
    }

    // Replace the old production rules with the new set
    productionRules = newProductions;

    cout << "\n >> Broke " << brokeCount << " bodies, added "
         << accumulate(varCount.begin(), varCount.end(), 0, [](int sum, const std::pair<const std::string, int>& p) {
             return sum + (p.second - 1);
         })
         << " new variables" << endl;
}






void CFG::toCNF() {
    cout << "Original CFG:\n\n";
    print();
    cout << "\n-------------------------------------\n\n";

    cout << " >> Eliminating epsilon productions\n";
    eliminateEpsilonProductions();
    print();

    cout << "\n";
    eliminateUnitProductions();
    cout << "\n";
    print();

    cout << "\n";
    removeUselessSymbols();
    print();

    cout << "\n >> Replacing terminals in bad bodies\n";
    // ZORG DAT ALLE PRODUCTION BODIES MET EEN LENGTE >= 2 ENKEL BESTAAN UIT VARIABELEN
    replaceTerminalsInBadBodies();
    print();

    //HERSCHRIJF ALLE PRODUCTION BODIES MET LENGTE >= 3 MET EXACT 2 VARIABELEN
    breakLongBodies();

    cout << ">>> Result CFG:\n\n";
    print();
}