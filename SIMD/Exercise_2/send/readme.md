# README

### Compile in debug mode
`gcc -g -m64 -march=corei7-avx main.c -o debug`

### Compile in release mode
`gcc -s -m64 -march=corei7-avx main.c -o release`

### Convert the images from raw to bmp
`./convertIt.sh`

### Explanations of the results
We see that the SIMD version of the code is really faster than the C version. For the 3x3 example, the SIMD is ~271 times faster. This is because the searching of the maximum and minimum are done in one time on the SIMD code on multiple pixels where in C the pixels surrounding the working pixel are treated one after the other.

The C code could somehow be improved by doing a list of value found in the neighbours and only removing and adding 1 column as most of the pixels of the neighbours doesn't change, it will result in a gain of speed.

In the case of the 5x5 example, the SIMD code is even faster compared to C (~274 times faster) and for the 7x7 the factor is ~291. This is the result of what is explained above.

Using the code in release mode makes the code faster but the difference is not pronounced. C part of the code benefits more of the extraction of all the debug code.
