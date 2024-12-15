#pragma once

#include <functional>
#include <variant>
#include <map>
#include <string>
#include <vector>
#include <sstream>
namespace cli {
typedef std::function<void(std::string)> cmd_func;

struct Cmd;
struct NamedCmd;
typedef std::map<std::string, Cmd> cmd_map;

// ii - inner interface
typedef std::variant<cmd_func, cmd_map> ii_cmd_or_cmds;
typedef std::variant<cmd_func, std::vector<NamedCmd>> ui_cmd_or_cmds;

ii_cmd_or_cmds convert_ui_to_ii_cmd_or_cmds(ui_cmd_or_cmds) noexcept;
bool try_calling_cmd(cmd_map&, std::stringstream&, std::stringstream&);
void print_help(std::ostream&, const cmd_map&, size_t indent = 0);

// The reason we don't inhearite from the Cmd class
// is that we want the name to be the first one in the constructor for easier reading.
// And we don't want to break LSP with a different constructor stub
struct NamedCmd {
  std::string name;
  std::string description;
  ui_cmd_or_cmds cmd_or_cmds;
};

struct Cmd {
  Cmd(std::string description, ui_cmd_or_cmds cmd)
    : description(description), cmd_or_cmds(convert_ui_to_ii_cmd_or_cmds(cmd))
  {}
  std::string description;
  ii_cmd_or_cmds cmd_or_cmds;

  bool operator()(std::stringstream& current_cmd_path, std::stringstream& args) {
    if (std::holds_alternative<cmd_func>(cmd_or_cmds)) {
      std::string rest;
      std::getline(args, rest);
      std::get<cmd_func>(cmd_or_cmds)(rest);
      return true;
    }
      
    return try_calling_cmd(std::get<cmd_map>(cmd_or_cmds), current_cmd_path, args);
  }
};


class Cli {
public:
  Cli(std::ostream& out, std::istream& in)
    : m_out(out), m_in(in) {}

  void add_help() {
    return add_cmd("help", "Prints This Message", [this](std::string rest){ print_help(m_out, m_cmds); });
  }
  void add_exit() {
    return add_cmd("exit", "Exits the program", [this](std::string rest){m_should_run = false;});
  }
  void add_cmd(NamedCmd named_cmd);
  void add_cmd(std::string name, std::string description, std::variant<cmd_func, std::vector<NamedCmd>> cmd_or_cmds);
  int run(int argc, char** argv);
protected:
  std::ostream& m_out;
  std::istream& m_in;

  cmd_map m_cmds;
  bool m_should_run = true;
};

};


#ifdef CLICPP_APP
namespace cli {
void Cli::add_cmd(NamedCmd named_cmd) {
  return add_cmd(named_cmd.name, named_cmd.description, named_cmd.cmd_or_cmds);
}

void Cli::add_cmd(std::string name, std::string description, ui_cmd_or_cmds cmd_or_cmds) {
  m_cmds.emplace(name, Cmd(description, cmd_or_cmds));
}

int Cli::run(int argc, char** argv) {
  std::string full_cmd;
  while (m_should_run) {
    m_out << ">>";
    
    if (!std::getline(m_in, full_cmd)) break; 

    std::stringstream args(full_cmd);
    std::stringstream cmd_path;

    if (!try_calling_cmd(m_cmds, cmd_path, args)) {
      m_out << "Unknown command '" << cmd_path.str() << "'.\nFor usage enter 'help'.\n";

    }

    m_out << "\n";
  }
  return 0;
}

ii_cmd_or_cmds convert_ui_to_ii_cmd_or_cmds(ui_cmd_or_cmds cmd_or_cmds) noexcept {
  if (std::holds_alternative<cmd_func>(cmd_or_cmds))
    return std::get<cmd_func>(cmd_or_cmds);

  auto& ui_cmds = std::get<std::vector<NamedCmd>>(cmd_or_cmds);
  cmd_map ii_cmds;

  for (NamedCmd& cmd : ui_cmds) {
    ii_cmds.emplace(cmd.name, Cmd(cmd.description, cmd.cmd_or_cmds));
  }
  return ii_cmds;
}
bool try_calling_cmd(cmd_map& map, std::stringstream& current_cmd_path, std::stringstream& args) {
  std::string cmd_name;
  args >> cmd_name;

  current_cmd_path << cmd_name;

  auto cmd_iter = map.find(cmd_name);

  if (cmd_iter == map.end()) {
    return false;
  }
  return cmd_iter->second(current_cmd_path, args);
}

void print_help(std::ostream& out, const cmd_map& map, size_t indent) {
  for (const auto [cmd_name, cmd] : map) {
    out << std::string(indent * 2, ' ') << cmd_name << ": " << cmd.description << "\n";
    if (std::holds_alternative<cmd_map>(cmd.cmd_or_cmds))
      print_help(out, std::get<cmd_map>(cmd.cmd_or_cmds), indent + 1);
    out << "\n";
  }
}
}

#endif
