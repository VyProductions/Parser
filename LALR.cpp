#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include "Parser.h"

/// @brief Pre-scoped identifiers

using std::cout;
using std::ifstream;
using std::list;
using std::reverse;
using std::string;

/// @brief Function declarations

list<Token> lexicate(const string& input, const Grammar& g);

/// @brief Main function
/// @param argc : number of command-line arguments on program execution
/// @param argv : vector of command-line arguments on program execution
/// @return integer to operating system

int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " [input string]\n";
        return 0;
    }

    ifstream grammar_file("grammar.txt");
    ifstream parser_file("parser.txt");
    Grammar g;

    // Populate grammar from file
    g.read(grammar_file);

    LALR_Parser parser(g);

    // Populate parser from file
    parser.read(parser_file);

    // Test LALR parser
    string input = argv[1];
    list<Token> tokens = lexicate(input, g);

    string rrd = parser.parse(tokens);
    string  rd = rrd;

    reverse(rd.begin(), rd.end());

    cout << "Reverse Rightmost Derivation: " << rrd << '\n'
         << "        Rightmost Derivation: " <<  rd << '\n';

    return 0;
}

/// @brief Function definitions

list<Token> lexicate(const string& input, const Grammar& g) {
    list<Token> tokens;
    string poss_ident = "";  // Possible identifier string to search for

    auto it = input.begin();

    while (it != input.end()) {
        if (isspace(*it) == 0) {  // Not a space character
            poss_ident += *it;
            list<Token> poss_tokens = g.term_prefix_matches(poss_ident);

            // No tokens to pick from that have this prefix in their ident
            if (poss_tokens.empty()) {
                cout << "Unknown symbol '" << poss_ident
                     << "' found during lexicating.\n";
                return {};
            // Found exactly one token that matches this ident
            } else if (
                poss_tokens.size() == 1 &&
                poss_tokens.front().ident == poss_ident
            ) {
                tokens.push_back(poss_tokens.front());
                poss_ident = "";
            // Else continue adding characters
            }
        } else {  // Is a space character
            if (!poss_ident.empty()) {
                list<Token> poss_tokens = g.term_prefix_matches(poss_ident);

                // One token terminated by a space matches this prefix
                if (
                    poss_tokens.size() == 1 &&
                    poss_tokens.front().ident == poss_ident
                ) {
                    tokens.push_back(poss_tokens.front());
                    poss_ident = "";
                // Not exactly one token to pick from
                } else {
                    cout << "Unknown symbol '" << poss_ident
                        << "' found during lexicating.\n";
                    return {};
                }
            }
        }

        ++it;
    }

    if (!poss_ident.empty()) {
        list<Token> poss_tokens = g.term_prefix_matches(poss_ident);

        // One token terminated by EOF matches this prefix
        if (
            poss_tokens.size() == 1 &&
            poss_tokens.front().ident == poss_ident
        ) {
            tokens.push_back(poss_tokens.front());
            poss_ident = "";
        // Not exactly one token to pick from
        } else {
            cout << "Unknown symbol '" << poss_ident
                << "' found during lexicating.\n";
            return {};
        }
    }

    tokens.push_back({"\\eof", true, (unsigned)g.num_terms()});

    return tokens;
}
