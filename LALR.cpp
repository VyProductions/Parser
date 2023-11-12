#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/// @brief Pre-scoped identifiers

using std::cout;
using std::ifstream;
using std::list;
using std::reverse;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

/// @brief Type declarations
/// @typedef token_t : handle for classifying a token
struct token_t {
    int id;       // ID for token class
    string name;  // Name of token class
};

/// @typedef Token : record to represent information about a token
struct Token {
    string   ident;      // Physical string of token
    bool     terminal;   // Whether the token is terminal or nonterminal
    unsigned table_idx;  // Index of token into terminal/nonterminal token list

    bool operator==(const Token& rhs) const {
        return ident == rhs.ident && terminal == rhs.terminal &&
            table_idx == rhs.table_idx;
    }
};

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

/// Hashing function for a Token instance
template <>
struct std::hash<Token>
{
    std::size_t operator()(const Token& t) const
    {
        return std::hash<string>()(t.ident);
    }
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

/// @brief Global constants



/// @brief Global variables

// Map the name of a token class to its integer ID
static unordered_map<string, int> token_name_map;
// Map the ID of a token class to its string name
static unordered_map<int, string> token_id_map;

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
