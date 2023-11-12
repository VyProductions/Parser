#ifndef PARSER_H
#define PARSER_H

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Token.h"
#include "Grammar.h"

/// @brief Pre-scoped identifiers

using std::cout;
using std::ifstream;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

/// @typedef action_t : handle to represent actions taken given an input to a
///     LALR parser and the current stack state of the parser
struct action_t {
    vector<int> actions;  // Actions to execute when parsing an input token
};

/// @typedef goto_t : handle to represent which stack state can be pushed after
///     a reduction given the current stack state of the parser
struct goto_t {
    vector<unsigned> valid_states;  // Pushable stack states after a reduction
};

/// @typedef LALR_Parser : container to associate the operations of parsing a
///     grammar with the grammar itself
/// @note This parser is implemented as a LALR parser
class LALR_Parser {
public:
    LALR_Parser(const Grammar& g) : G(g) {}

    void read(ifstream& infile) {
        infile.ignore(80, '\n');  // State list comment
        string input;  // Input storage for action/goto table and state list
        int index;     // Index into terminal/nonterminal list of grammar
        unsigned line = 1;  // Line being read from action/goto/state lists

        // Read state list

        // EOF state (0)
        states.push_back({"\\eof", true, (unsigned)G.num_terms()});

        // Start symbol state (1)
        states.push_back(G.get_start());

        while (infile.peek() != '#') {
            getline(infile, input, '\n');

            // Attempt to find ident in terminals
            if ((index = G.has_terminal(input)) != -1) {
                states.push_back({input, true, (unsigned)index});
            // Attempt to find ident in nonterminals
            } else if ((index = G.has_nonterminal(input)) != -1) {
                states.push_back({input, false, (unsigned)index});
            } else {
                cout << "Unrecognized state token: '" << input << "' on line "
                     << line << " of the state list.\n";
                return;
            }

            // Grab next state
            ++line;
        }


        // Read action table
        infile.ignore(100, '\n');  // Action table comment
        line = 1;  // Reset line to 1 for action table reading

        size_t num_prods = G.num_prods();  // Number of productions in grammar

        while (infile.peek() != '#') {
            // Read leading terminal token
            infile >> input;

            // Check that it actually exists in the state list
            bool found = false;
            auto it = states.begin();

            while (it != states.end() && !found) {
                found = it->ident == input;
                if (!found) ++it;
            }

            if (!found) {
                cout << "State list does not contain symbol '" << input
                     << "'\n";
                return;
            }

            action_t entries;
            int action;
            size_t i = 0;

            // Read action entires for the given token
            while (infile && i < states.size()) {
                infile >> action;

                // Validate action
                if (action < 0 && (size_t)-action > num_prods + 1) {
                    cout << "Invalid reduction/halt in ACTION line " << line
                         << ", column " << i + 2 << ": '" << -action
                         << "' is out of range for " << num_prods
                         << " productions.\n";
                    return;
                } else if (action > 0 && (size_t)action >= states.size()) {
                    cout << "Invalid shift in ACTION line " << line
                         << ", column " << i + 2 << ": '" << action
                         << "' is out of range for " << states.size()
                         << " states.\n";
                    return;
                }

                entries.actions.push_back(action);

                ++i;
            }

            if (!infile || i != states.size()) {
                cout << "Expected " << states.size() << " integers for "
                     << "ACTION line " << line << '\n';
                return;
            }

            // Add action list entry to map
            actions[*it] = entries;

            // Move on to next action line
            ++line;
            infile.ignore();  // '\n';
        }

        // Read goto table
        infile.ignore(100, '\n');  // Goto table comment
        line = 1;

        while (infile.peek() != '#') {
            // Read leading terminal token
            infile >> input;

            // Check that the token is a nonterminal in the grammar
            if ((index = G.has_nonterminal(input)) == -1) {
                cout << "Grammar does not have nonterminal symbol '" << input
                     << "'\n";
                return;
            }

            goto_t entries;
            int state;
            size_t i = 0;

            // Read action entires for the given token
            while (infile && i < states.size()) {
                infile >> state;

                // Validate action
                if (state < 0 || (size_t)state > states.size() + 1) {
                    cout << "Invalid state in GOTO line " << line
                         << ", column " << i + 2 << ": '" << state
                         << "' is out of range for " << states.size()
                         << " states.\n";
                    return;
                }

                entries.valid_states.push_back(state);

                ++i;
            }

            if (!infile || i != states.size()) {
                cout << "Expected " << states.size() << " integers for "
                     << "GOTO line " << line << '\n';
                return;
            }

            // Add goto list entry to map
            goto_push[{input, false, (unsigned)index}] = entries;

            // Move on to next goto line
            ++line;
            infile.ignore();  // '\n';
        }
    }

    void debug() {
        // Print content of grammar for visual testing
        G.debug();

        // Print content of parser for visual testing
        cout << "State List:\n";
        auto it = states.begin();

        while (it != states.end()) {
            cout << "  " << (it->ident == "\\eof" ? "$" : it->ident)
                 << it++ - states.begin() << '\n';
        }

        cout << "Action Table:\n";

        for (auto& [token, action] : actions) {
            cout << "  " << token.ident << ' ';

            for (int act : action.actions) {
                cout << act << ' ';
            }
            cout << '\n';
        }

        cout << "Goto Table:\n";

        for (auto& [token, state] : goto_push) {
            cout << "  " << token.ident << ' ';

            for (int st : state.valid_states) {
                cout << st << ' ';
            }
            cout << '\n';
        }
    }

    string parse(const list<Token>& input) {
        const int HALT = (int)-(G.num_prods() + 1);  // Halt in action table
        bool running = true;                    // Still parsing
        auto front = input.begin();             // Front of input token list
        string rrd = "";                        // Reverse rightmost derivation

        parse_stack = {0};  // Set stack to contain only EOF state

        while (running && front != input.end()) {
            unsigned& top = parse_stack.back();  // Top of state stack
            const Token& first = *front;         // Front of input
            const int& action = actions[first].actions.at(top);

            if (action > 0) {  // Shift first token onto top of stack
                parse_stack.push_back(action);
                ++front;
            } else if (action == HALT) {  // Done parsing; terminate
                running = false;
            } else if (action < 0) {  // Reduce top of stack and push state
                production_t prod = G.get_production((unsigned)-action - 1);
                const Token& lhs_symbol  = prod.lhs;
                const size_t RHS_SYMBOLS = prod.rhs.size();

                // Pop symbols from stack for each symbol in rhs of production
                for (size_t i = 0; i < RHS_SYMBOLS; ++i) {
                    parse_stack.pop_back();
                }

                // Push new symbol from goto table onto stack using production
                parse_stack.push_back(
                    goto_push[lhs_symbol]
                    .valid_states
                    .at(
                        parse_stack.back()
                    )
                );

                // Store production into reverse rightmost derivation string
                rrd += to_string(-action);
            } else {
                cout << "Error. Parser hit an empty cell while parsing.\n";
                return rrd;
            }
        }

        if (running) {
            cout << "Error. Ran out of input during parse without halting.\n";
        }

        return rrd;
    }

private:
    // Parser details
    Grammar G;  // The grammar that can be parsed by the LALR parser

    // LALR details
    unordered_map<Token, action_t> actions;  // ACTION table for LALR parsing
    unordered_map<Token, goto_t> goto_push;  // GOTO table for LALR parsing
    vector<Token> states;   // Increasing state value identities from G::prods
    list<unsigned> parse_stack;  // State stack for LALR parsing algorithm
};

#endif /* PARSER_H */

/* EOF */
