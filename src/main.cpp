#include "simulator.hpp"

int main(int argc, const char **argv)
{
    Simulator simulator;

    int errorCode;
    errorCode = simulator.init();
    if (errorCode)
    {
        printf("init errored with code: %i\n", errorCode);
        return errorCode;
    }

    while (simulator.running())
    {
        errorCode = simulator.run();
        if (errorCode)
        {
            printf("Run errored with code: %i\n", errorCode);
            return errorCode;
        }
    }

    printf("Exited normally\n");
}