#include "../utils.h"

#include <fstream>

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
        printf("arg %i: %s\n", i, argv[i]);

    if (argc != 2)
        error("need 1 (and only 1) arg (filename to read from)");

    Stopwatch s;
    ifstream input(argv[1]);
    CharBuffer text(input);

    cout << text << endl << s.lap() << " seconds" << endl;

    warn("oh noes!");
}
