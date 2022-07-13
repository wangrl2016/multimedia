//
// Created by admin on 2022/7/13.
//

#include <string>
#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    using namespace std::literals;

    // Creating a string from const char*.
    std::string str1 = "hello";

    // Creating a string using string literal.
    auto str2 = "world"s;

    // Concatenating strings.
    std::string str3 = str1 + " " + str2;

    // Print out the result.
    std::cout << str3 << '\n';

    std::string::size_type pos = str3.find(' ');
    str1 = str3.substr(pos + 1);
    str2 = str3.substr(0, pos);

    std::cout << str1 << ' ' << str2 << '\n';

    // Accessing an element using subscript operator[]
    std::cout << str1[0] << '\n';
    str1[0] = 'W';
    std::cout << str1 << '\n';

    return 0;
}