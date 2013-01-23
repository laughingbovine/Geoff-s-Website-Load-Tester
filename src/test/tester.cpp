#include "../tester.h"

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char** argv)
{
    LoadTest::Tester t("www.columbia.edu", 80, 1, 1);

    CharBuffer request("GET / HTTP/1.0\r\nConnection: close\r\n\r\n");

    t.add_input(request);
    t.set_num_tests_per_ramp(1);
    t.set_num_ramps(5);
    t.set_ramp_wait(1);

    t.print_initial_summary();
    t.start_signal_handlers();
    ////////////////////////////////////////////////////////////////////////////////
    // from this point on, SIGINT will cause the program to exit gracefully       //
    // that is, stop spawning threads, stop all tests in progress,                //
    // join them and collect/print statistics                                     //
    ////////////////////////////////////////////////////////////////////////////////
    t.init_tests();
    t.run_tests();
    t.finish_tests();
    t.print_final_summary();
}
