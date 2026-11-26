#include "core/Application.hpp"

#include <iostream>
#include <exception>
#include <string_view>

int main(int argc, char** argv)
{
    const bool procedural = argc == 2 && std::string_view(argv[1]) == "--procedural";
    if (argc != 1 && !procedural)
    {
        std::cerr << "Usage: ASCIIEngine [--procedural]\n";
        return 2;
    }
    try
    {
        Application app(!procedural);

        app.run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';

        return 1;
    }

    return 0;
}
