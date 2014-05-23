#include "CliClient.qt.h"

int main(int argc, char **argv)
{
    CliClient cli(argc, argv);
    cli.init();
    return cli.exec();
}