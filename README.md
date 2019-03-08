# PA4 Test cases
Put testcases.c and Makefile in pa4 directory.

Compile:

    make test
    
To run:
    
    ./runTests -t <test case> [-e]
    
Running with -e will run the test case using the reference solution.

To run all the tests, run

    ./runAll.sh
    
This will automatically diff all your output vs. the reference solution
output.

Note: You must change all the error messages so that it does not contain
"My" in order for the output to match the reference solution. For example,
in MyYieldThread, make sure that it says "YieldThread" in the print statement
instead.