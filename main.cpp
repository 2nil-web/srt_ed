
#include <algorithm>
#include <cctype>
#include <clocale>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

// clang-format off
#include "options.h"
#include "util.h"
// clang-format on

std::map<std::string, std::string> sym_table;
bool numbering = false;

bool is_varname(std::string s)
{

  for (unsigned char c : s)
  {
    if (isalnum(c) == 0 && c != '_')
      return false;
  }

  return true;
}

bool setvar(std::string expr)
{
  trim(expr, "\"");
  std::string var = {}, val = {};
  split_1st(var, val, expr, '=');

  if (var.empty() || val.empty())
    return false;

  trim(val, "\"");
  if (envvar_get(var).empty() || !envvar_set(var, val))
  {
    if (is_varname(var))
      sym_table[var] = val;
    else
      std::cerr << "Not a correct var name '" << var << "'" << std::endl;
  }

  return true;
}

bool unsetvar(std::string var)
{
  trim(var, "\"");
  if (var.empty())
    return false;
  sym_table.erase(var);
  return true;
}

std::string ret_expansion(std::string var, char c)
{
  std::string val, ret = {};

  if (!var.empty())
  {
    val = envvar_get(var);
    if (val.empty())
    {
      if (sym_table.count(var))
        ret += sym_table[var];
    }
    else
      ret += val;
  }

  if (ret.empty())
    std::cerr << "Var name '" << var << "' expands to empty value." << std::endl;
  ret += c;
  return ret;
}

size_t line_number = 1;
std::string expand(std::string expr)
{
  trim(expr, "\"");
//  std::cout << "Expanding: " << expr << std::endl;
  std::string res = {};

  for (size_t i = 0; i < expr.size(); i++)
  {
    if (expr[i] == '$')
    {
      if (!res.empty() && res.back() == '$')
        continue;

      std::string var = {}, val = {};

      // Look for var invocation between braces, in the form '${var}'
      if (expr[i + 1] == '{')
      {
        i++;
        for (;;)
        {
          i++;
          if (expr[i] == '}' || i >= expr.size())
          {
            if (expr[i] == '}')
              i++;
            res += ret_expansion(var, expr[i]);
            break;
          }

          var += expr[i];
        }
      }
      // Look for 'simple' var invocation in the form '$var'
      else
      {
        for (;;)
        {
          i++;
          if ((!isalnum(expr[i]) && expr[i] != '_') || isspace(expr[i]) || i >= expr.size())
          {
            res += ret_expansion(var, expr[i]);
            break;
          }

          var += expr[i];
        }
      }
    }
    else
    {
      res += expr[i];
    }
  }

  if (numbering)
    res = std::to_string(line_number++) + ':' + res;
  return res;
}

void listvar() {
  std::for_each(
    sym_table.begin(), sym_table.end(),
    [](std::pair<std::string, std::string> p) {
      std::cout << p.first << '=' << p.second << std::endl;
    }
  );
}

std::string read(std::string args)
{

  std::cout << args << std::endl;

  return args;
}

// The main function
int main(int argc, char **argv, char **)
{
  options myopt;
  myopt.set(argc, argv,
            {
                option_info('i', "interp", [&myopt](s_opt_params &) -> void { myopt.parse(std::cin); }, "Enter interpreter mode.", no_arg, option),
                option_info('n', "num", [](s_opt_params &) -> void { numbering = true; }, "Prepend resulting line(s) by a number.", no_arg, option),
                option_info('s', "set", [](s_opt_params &p) -> void { setvar(p.val); }, "Set a value to a variable in the form 'set var=val'.", required, interp),
                option_info('u', "unset", [](s_opt_params &p) -> void { unsetvar(p.val); }, "Unset a variable in the form 'unset var'.", required, interp),
                option_info('e', "println", [](s_opt_params &p) -> void { std::cout << expand(p.val) << std::endl; },
                    "Echo the provided parameter(s) and add a carriage return. Variable names must start with '$' or be formed as follow:'${var_name}'. And they are expanded to their value", optional, interp),
                option_info('w', "print", [](s_opt_params &p) -> void { std::cout << expand(p.val) << std::flush; }, "Same as 'println' without adding a carriage return.", optional, interp),
                option_info('l', "list", [](s_opt_params &) -> void { listvar(); }, "List all the variable that are within the symbol table.", no_arg),
                option_info('f', "read", [](s_opt_params &p) -> void { read(p.val); }, "Read file with path provided as first parameter and put its content into var provided as second parameter.", required),
                option_info('x', "exit", [](s_opt_params &) -> void { exit(0); }, "Exit from interpreted mode.", no_arg, interp),
                option_info('q', "quit", [](s_opt_params &) -> void { exit(0); }, "Alias for exit.", no_arg, interp),
            });

  myopt.parse();

  // If no remaining argument then interpretation mode
  if (myopt.args.empty())
  {
    myopt.parse(std::cin);
  }
  // else run all the provided file(s)
  else
    for (auto arg : myopt.args)
      myopt.parse(arg);

  return 0;
}

