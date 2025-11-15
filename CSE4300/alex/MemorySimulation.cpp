#include <iostream>
#include <string>
#include "MemoryManager.h"

MemoryManager mm;

bool running = true;

std::string listOptions() {
    return "1. Allocate a new page\n2. Delete a page at an address\n3. Write to an address\n4. Read from an address\n5. Print information about the page at an address\n6. Exit\n";
}

int hexStringToInt(std::string string) {
    // validate that string is valid hex
    if (string.compare(0, 2, "0x") == 0 || string.compare(0, 2, "0X") == 0) {
        string.erase(0, 2);
    }
    for (auto character : string){
        if (!isxdigit(character)) return -1;
    }

    int integer = std::stoi(string, nullptr, 16);

    return integer;
}

void allocateAPage() {
    int newPage = mm.allocateAnyPage();
    std::string zeroBase = "";
    if (newPage == 0) zeroBase = "0x";
    std::cout << "Your new page is located at virtual memory address: " << zeroBase << std::showbase << std::hex << newPage << std::endl;
    std::cout << "It can be written to from addresses: [" << zeroBase << newPage << ":" << newPage + 4095 << "]" << std::endl;
}

void deleteAPage() {
    std::string input;
    int address;

    std::cout << "Enter address of page you would like deleted (enter -1 to return): ";
    std::cin >> input; std::cout << std::endl;

    address = hexStringToInt(input);
    if (address < 0) {std::cout << "Please enter a valid address!" << std::endl; return;}

    mm.deletePageTableEntry(address);

    std::cout << "Entry successfully deleted!" << std::endl;
}

void writeToAnAddress() {}

void readFromAnAddress() {}

void printPageInfo() {}

void exitProgram() {
     running = false;

     std::cout << "Goodbye!" << std::endl;
}

void handleOptions(int choice) {
    switch (choice) {
        case 1:
            allocateAPage();
            break;
        case 2:
            deleteAPage();
            break;
        case 3:
            writeToAnAddress();
            break;
        case 4:
            readFromAnAddress();
            break;
        case 5:
            printPageInfo();
            break;
        case 6:
            exitProgram();
            break;
    }

    std::cout << std::endl;
}

int main() {
    std::cout << "\033[2J\033[1;1H" << std::flush;
    while (running) {
        std::cout << " --=--= Virtual Memory Simulation =--=--" << std::endl;
        std::cout << listOptions() << std::endl;

        std::string selection;
        int choice = -1;
        std::cout << "Please make a selection: ";

        std::cin >> selection;
        std::cout << std::endl;

        if (selection.length() == 1 && isdigit(selection[0])) {
            choice = selection[0] - '0';
            if (choice >= 1 && choice <= 6) {
                handleOptions(choice);
                continue;
            }
        }

        std::cout << "!!! Please pick from the choices presented! !!!" << std::endl << std::endl;
    }
    return 0;
}
