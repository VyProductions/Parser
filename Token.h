#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <unordered_map>

/// @brief Pre-scoped identifiers

using std::string;

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

/// Hashing function for a Token instance
template <>
struct std::hash<Token>
{
    std::size_t operator()(const Token& t) const
    {
        return std::hash<string>()(t.ident);
    }
};

#endif /* TOKEN_H */

/* EOF */
