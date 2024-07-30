#include "eshell.h"
#include "parser.h"
#include <iostream>
#include <string>

int main() {
    eshell shell;
    std::string line;
    std::cout << "/> ";

    while (getline(std::cin, line)) {
        fflush(stdout);  

        if (line == "quit") break;

        parsed_input input;
        if (parse_line(const_cast<char*>(line.c_str()), &input)) {
            //pretty_print(&input);

            shell.execute(&input);
            free_parsed_input(&input);
        } else {
            std::cerr << "Error parsing input" << std::endl;
        }

        std::cout << "/> ";
    }
    return 0;
}
