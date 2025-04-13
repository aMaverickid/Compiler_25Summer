#ifndef SEMANTIC_SYMBOL_TABLE_HPP
#define SEMANTIC_SYMBOL_TABLE_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "type.hpp"

class Symbol;
using SymbolPtr = std::shared_ptr<Symbol>;
class Symbol {
 public:
  /// @brief The name of the symbol
  std::string name;
  /// @brief The unique name of the symbol
  std::string unique_name;
  /// @brief The type of the symbol
  TypePtr type;
#warning Symbol: need to add a member variable to store the symbol's scope
  Symbol(std::string name, TypePtr type) : name(name), type(type) {}
  static SymbolPtr create(std::string name, TypePtr type) {
    return std::make_shared<Symbol>(name, type);
  }
};

class SymbolTable {
 public:
  /// @brief Add a symbol to the table and return the unique name of the symbol
  /// @param name The name of the symbol
  /// @param type The type of the symbol
  /// @return The added symbol if added successfully, nullptr otherwise
  SymbolPtr add_symbol(std::string name, TypePtr type);

  /// @brief Find a symbol by name
  /// @param name The name of the symbol
  /// @param in_current_scope Whether to search only in the current scope
  /// @return The symbol if found, nullptr otherwise
  SymbolPtr find_symbol(std::string name, bool in_current_scope = false) const;

  /// @brief Enter a new scope
  void enter_scope();

  /// @brief Exit the current scope
  void exit_scope();

  /// @brief The current scope depth
  int scope_depth = -1;
  /// @brief functional implementation of the symbol table
  /// scopes.back() is the current scope
  std::vector<std::unordered_map<std::string, SymbolPtr>> scopes;

 private:
  
};

#endif  // SEMANTIC_SYMBOL_TABLE_HPP