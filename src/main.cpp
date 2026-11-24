#include "core/Application.hpp"

#include <iostream>
#include <exception>

int main()
{
    try
    {
        Application app;

        app.run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';

        return 1;
    }

    return 0;
}