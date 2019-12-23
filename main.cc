/*
 * main.cc
 */
#include "reactor.hh"

int main(int ac, char** av)
{
    reactor r;
    //r.listen(sa);
    r.run();
    return 0;
}


