# README

### Compile in debug mode
`gcc -g -m64 -march=corei7-avx main.c -o debug`

### Compile in release mode
`gcc -s -m64 -march=corei7-avx main.c -o release`

### Explanations of the results
We can see by running the code that the SIMD code is a little bit less than 36 times faster. We would expect it to be 16 times faster as we process 16 pixels at a time but we have nearly the double, maybe because of some optimisations in the SIMD instructions compared to standard registers and instructions.

The code in release is smaller because all the debug part are stripped out of the binary. Normally the execution should also be faster but no difference were noticed.

In order to get a precise measurement of the time, the function is run multiple times (`TEST_COUNT` value) to get an average time and cancel the most we can the perturbations on the system due to other processes running at the same time.
