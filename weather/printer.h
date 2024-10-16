#pragma once

#include <iostream>

class WeatherPrinter {
public:
    virtual void print_to(std::ostream &stream) {}

    virtual ~WeatherPrinter() {}
};
