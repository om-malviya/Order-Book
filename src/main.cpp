#include "cli.h"
#include "demo.h"
#include "console.h"
#include <iostream>
#include <string>

int main() {
    cli::init();
    cli::banner();
    cli::start_menu();

    std::cout << "  Choose> ";
    std::cout.flush();

    std::string answer;
    if (!std::getline(std::cin, answer)) {
        std::cout << "\n";
        return 0;
    }

    if (!answer.empty() && answer[0] == '1')
        demo::run();
    else
        console::run();

    return 0;
}
