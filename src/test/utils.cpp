#include "../utils.h"

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv)
{
    Stopwatch s;
    CharBuffer b;
    CharBuffer* text = sread_istream(cin, b);

    cout << text->chars << "hi there" << endl << s.lap() << " seconds" << endl;

    error("oh noes!");
}
