#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "Token.h"

/// @brief Pre-scoped identifiers

using std::cout;
using std::ifstream;
using std::list;
using std::string;
using std::vector;

/// @typedef Production : handle to represent each production in a grammar
struct production_t {
    Token lhs;        // Nonterminal token which will can be translated
    list<Token> rhs;  // Ordered set of tokens which result from translating lhs
};

/// @typedef Grammar : container to associate the data representing a grammar
///     with the valid set of operations on that data
class Grammar {
public:
    Grammar() = default;

    /// @brief Accessor Methods

    production_t get_production(unsigned which) const {
        if (which >= prods.size()) {
            throw std::out_of_range("Production index exceeds production list");
        }

        return prods.at(which);
    }

    int has_terminal(const string& ident) const {
        bool found = false;
        auto it = terminals.begin();

        while (it != terminals.end() && !found) {
            found = it->ident == ident;
            if (!found) ++it;
        }

        return found ? it->table_idx : -1;
    }

    int has_nonterminal(const string& ident) const {
        bool found = false;
        auto it = nonterminals.begin();

        while (it != nonterminals.end() && !found) {
            found = it->ident == ident;
            if (!found) ++it;
        }

        return found ? it->table_idx : -1;
    }

    size_t num_prods() const {
        return prods.size();
    }

    Token get_start() {
        return start;
    }

    size_t num_terms() const {
        return terminals.size();
    }

    size_t num_nterms() const {
        return nonterminals.size();
    }

    list<Token> term_prefix_matches(const string& ident) const {
        list<Token> result;

        for (auto token : terminals) {
            if (token.ident.find(ident) == 0) {
                result.push_back(token);
            }
        }

        return result;
    }

    /// @brief Mutator Methods

    void read(ifstream& infile) {
        string separator;  // Characterse between lhs and rule, or between rules
        Token input;       // Set up each token to be inserted

        // Read grammar separator characters
        infile.ignore(100, '\n');  // Grammar separator comment

        getline(infile, separator, '\n');

        // Get list of nonterminal tokens
        infile.ignore(100, '\n');  // Nonterminal tokens comment

        input.terminal = false;  // Pre-set nonterminal state for this section

        while (infile.peek() != '#') {
            getline(infile, input.ident, '\n');  // Each line is a n-term token

            // Check that this token does not already exist in nonterminal list
            if (has_nonterminal(input.ident) != -1) {
                cout << "Nonterminal token '" << input.ident
                     << "' already exists in grammar.\n";
                return;
            }

            input.table_idx = (unsigned)nonterminals.size();  // Spot in table
            nonterminals.push_back(input);       // Add to nonterminal list
        }

        // Read start symbol
        infile.ignore(100, '\n');  // Start symbol comment

        getline(infile, start.ident, '\n');  // Get physical start symbol string
        start.terminal = false;

        // Check that start symbol is an existing nonterminal token
        int index;  // Result of querying for existance of start nterm token

        if ((index = has_nonterminal(start.ident)) == -1) {
            cout << "Start symbol '" << start.ident << "' is not an existing "
                 << "nonterminal token.\n";
            return;
        }

        start.table_idx = (unsigned)index;

        // Get list of terminal tokens
        infile.ignore(100, '\n');  // Terminal tokens comment

        input.terminal = true;  // Pre-set terminal state for this section

        while (infile.peek() != '#') {
            getline(infile, input.ident, '\n');  // Each line is a term token

            // Check that this token does not already exist in terminal list
            if (has_terminal(input.ident) != -1) {
                cout << "Terminal token '" << input.ident
                     << "' already exists in grammar.\n";
                return;
            }

            // Check that this token does not already exist in nonterminal list
            if (has_nonterminal(input.ident) != -1) {
                cout << "Nonterminal token '" << input.ident
                     << "' already exists in grammar. "
                     << "Cannot make a terminal token with the same name.\n";
                return;
            }

            input.table_idx = (unsigned)terminals.size();  // Spot in table
            terminals.push_back(input);          // Add to terminal list
        }

        // Read list of grammar productions
        infile.ignore(100, '\n');  // Grammar productions comment

        unsigned i = 1;     // Which production line is being read
        unsigned j;         // Which rule for the production line is being read
        production_t prod;  // Production storage for insertion
        string text;        // Physical strings in production list

        prod.lhs.terminal = false;  // LHS must be a nonterminal symbol

        while (infile.peek() != '#') {
            infile >> prod.lhs.ident;  // Read lhs token of production

            // Verify that the input string is a listed nonterminal
            if ((index = has_nonterminal(prod.lhs.ident)) == -1) {
                cout << "Production line " << i << " does not start with a "
                     << "nonterminal token.\n";
                return;
            }

            prod.lhs.table_idx = (unsigned)index;

            infile >> text;

            if (text != separator) {
                cout << "Production line " << i << " does not follow the LHS "
                     << "token '" << prod.lhs.ident << "' with separator '"
                     << separator << "'\n";
                return;
            }

            j = 1;  // First rule in the current production line

            while (infile.peek() != '\n') {
                infile >> text;

                if (text == separator) {
                    if (prod.rhs.empty()) {
                        cout << "Production line " << i << ", rule " << j
                        << " is empty\n";
                        return;
                    }

                    prods.push_back(prod);  // Add production i-j to list
                    prod.rhs.clear();       // Clear rule for new input
                    ++j;                    // Move on to next rule in line
                } else {
                    // Check if the string is a terminal token
                    if ((index = has_terminal(text)) != -1) {
                        prod.rhs.push_back(Token{text, true, (unsigned)index});
                    // Check if the string is a nonterminal token
                    } else if ((index = has_nonterminal(text)) != -1) {
                        prod.rhs.push_back(Token{text, false, (unsigned)index});
                    // It's neither
                    } else {
                        cout << "Production line " << i << ", rule " << j
                                << " has an unrecognized token: '" << text
                                << "'\n";
                        return;
                    }
                }
            }

            // Check that last rule in production line has tokens
            if (prod.rhs.empty()) {
                cout << "Production line " << i << ", rule " << j
                      << " is empty\n";
                return;
            }

            // Add last rule in production line to productions list
            prods.push_back(prod);

            ++i;  // Move on to next production line
            infile.ignore();   // '\n' at end of production line
            prod.rhs.clear();  // Empty RHS for next line to start from
        }
    }

    void debug() {
        // Print content of grammar for visual testing
        cout << "Start Symbol:\n" << "  " << start.ident
             << " (idx: " << start.table_idx << ")\n";

        cout << "Nonterminals:\n";

        for (auto token : nonterminals) {
            cout << "  " << token.ident << " (idx: " << token.table_idx
                 << ")\n";
        }

        cout << "Terminals:\n";

        for (auto token : terminals) {
            cout << "  " << token.ident << " (idx: " << token.table_idx
                 << ")\n";
        }

        cout << "Productions:\n";

        unsigned k = 1;  // Which production is it?

        for (auto p : prods) {
            cout << "  " << k++ << ". " << p.lhs.ident << " -> ";

            for (auto token : p.rhs) {
                cout << token.ident;
            }

            cout << '\n';
        }
    }

private:
    // Grammar Details
    vector<Token> nonterminals;  // Nonterminal token instances in the grammar
    vector<Token> terminals;     // Terminal token instances in the grammar
    Token start;                 // Nonterminal starting token for the grammar
    vector<production_t> prods;  // Productions that derive valid token strings
};

#endif /* GRAMMAR_H */

/* EOF */
