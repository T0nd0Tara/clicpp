#define CLICPP_APP
#include <iostream>
#include <cli.hpp>

int main(int argc, char** argv) {
  cli::Cli cli(std::cout, std::cin);

  cli.add_help();
  cli.add_exit();

  cli.add_cmd("print", "prints the next arguements", [](std::string rest){ std::cout << "PRINTING: " << rest << '\n'; });

  cli.add_cmd("get", "gets some stuff", std::vector<cli::NamedCmd>{
    {"name", "name desc", [](std::string rest){ std::cout << "get name"; }},
    {"param", "param desc", [](std::string rest){ std::cout << "get param"; }}
  });
  

  return cli.run(argc, argv);
}
