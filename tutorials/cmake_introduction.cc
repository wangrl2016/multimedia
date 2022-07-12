//
// Created by wangrl2016 on 2022/7/12.
//

#include <iostream>
#include <cmath>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " number" << std::endl;
        return 1;
    }

    // convert input to double
    const double input_value = std::stod(argv[1]);

    // calculate square root
    const double output_value = sqrt(input_value);
    std::cout << "The square root of " << input_value << " is " << output_value << std::endl;

    return 0;
}
