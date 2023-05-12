#include <string.h>
#include <iostream>

int main() {
    char* text = "get /"; 
    char* m_url = strpbrk(text," \t");
    std::cout << text;
    std::cout << strcasecmp("h\01", "h");
}