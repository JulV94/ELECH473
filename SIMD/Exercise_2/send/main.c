#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define W 1024
#define H 1024
#define TEST_COUNT 100

void c_min_max(unsigned char *src, unsigned char *dst, int window_size)
{
    int i, j, k, l, min_buffer, max_buffer, border_size;
    border_size = (window_size-1)/2;

    for (i=border_size; i<(H-border_size); i++)
    {
        for (j=border_size; j<(W-border_size);j++)
        {
            min_buffer = 255;
            max_buffer = 0;
            for (k=i-border_size;k<=(i+border_size);k++)
            {
                for (l=j-border_size;l<=(j+border_size);l++)
                {
                    if (src[W*k+l] > max_buffer)
                    {
                        max_buffer = src[W*k+l];
                    }
                    if (src[W*k+l] < min_buffer)
                    {
                        min_buffer = src[W*k+l];
                    }
                }
            }
            dst[W*i+j] = max_buffer - min_buffer;
        }
    }
}

void simd_min_max_3(unsigned char *src, unsigned char *dst)
{
    long i = W*(H-2*1)/(16-2*1);

    asm(
        "mov %[src], %%rsi\n"
        "mov %[i], %%rcx\n"
        "mov %[dst], %%rdi\n"
        "l1:\n"
        "movdqu (%%rsi), %%xmm0\n"  // Line 1
        "movdqu 1024(%%rsi), %%xmm1\n"  // Line 2
        "movdqu 2048(%%rsi), %%xmm2\n"  // Line 3
        "pmaxub %%xmm1, %%xmm0\n"
        "pmaxub %%xmm2, %%xmm0\n"     // Compare lines vertically
        "movdqu %%xmm0, %%xmm6\n"
        "movdqu %%xmm0, %%xmm7\n"     // Copy the result
        "psrldq $1, %%xmm6\n"
        "psrldq $2, %%xmm7\n"         // Shift the register
        "pmaxub %%xmm7, %%xmm6\n"
        "pmaxub %%xmm0, %%xmm6\n"     // Compare lines vertically

        "movdqu (%%rsi), %%xmm0\n"  // Line 1
        "pminub %%xmm1, %%xmm0\n"
        "pminub %%xmm2, %%xmm0\n"     // Compare lines vertically
        "movdqu %%xmm0, %%xmm5\n"
        "movdqu %%xmm0, %%xmm7\n"     // Copy the result
        "psrldq $1, %%xmm5\n"
        "psrldq $2, %%xmm7\n"         // Shift the register
        "pminub %%xmm7, %%xmm5\n"
        "pminub %%xmm0, %%xmm5\n"     // Compare lines vertically

        "psubb %%xmm5, %%xmm6\n"     // Substract the min to the max
        "movdqu %%xmm6, (%%rdi)\n"    // Result in rdi register
        "add $14, %%rdi\n"
        "add $14, %%rsi\n"
        "sub $1, %%rcx\n"
        "jnz l1\n"
        : [dst]"=m" (dst)     // outputs
        : [src]"g" (src), [i]"g" (i)// inputs
        : "rsi", "rcx", "rdi", "xmm0", "xmm1", "xmm2", "xmm5", "xmm6", "xmm7"  // clobbers
        );
}

void simd_min_max_5(unsigned char *src, unsigned char *dst)
{
    long i = W*(H-2*2)/(16-2*2);

    asm(
        "mov %[src], %%rsi\n"
        "mov %[i], %%rcx\n"
        "mov %[dst], %%rdi\n"
        "l2:\n"
        "movdqu (%%rsi), %%xmm0\n"  // Line 1
        "movdqu 1024(%%rsi), %%xmm1\n"  // Line 2
        "movdqu 2048(%%rsi), %%xmm2\n"  // Line 3
        "movdqu 3072(%%rsi), %%xmm3\n"  // Line 4
        "movdqu 4096(%%rsi), %%xmm4\n"  // Line 5
        "pmaxub %%xmm1, %%xmm0\n"
        "pmaxub %%xmm2, %%xmm0\n"
        "pmaxub %%xmm3, %%xmm0\n"
        "pmaxub %%xmm4, %%xmm0\n"     // Compare lines vertically
        "movdqu %%xmm0, %%xmm1\n"
        "movdqu %%xmm0, %%xmm2\n"
        "movdqu %%xmm0, %%xmm3\n"
        "movdqu %%xmm0, %%xmm4\n"     // Copy the result
        "psrldq $1, %%xmm1\n"
        "psrldq $2, %%xmm2\n"
        "psrldq $3, %%xmm3\n"
        "psrldq $4, %%xmm4\n"         // Shift the register
        "pmaxub %%xmm1, %%xmm0\n"
        "pmaxub %%xmm2, %%xmm0\n"
        "pmaxub %%xmm3, %%xmm0\n"
        "pmaxub %%xmm4, %%xmm0\n"     // Compare lines vertically

        "movdqu (%%rsi), %%xmm5\n"  // Line 1
        "movdqu 1024(%%rsi), %%xmm1\n"  // Line 2
        "movdqu 2048(%%rsi), %%xmm2\n"  // Line 3
        "movdqu 3072(%%rsi), %%xmm3\n"  // Line 4
        "movdqu 4096(%%rsi), %%xmm4\n"  // Line 5
        "pminub %%xmm1, %%xmm5\n"
        "pminub %%xmm2, %%xmm5\n"
        "pminub %%xmm3, %%xmm5\n"
        "pminub %%xmm4, %%xmm5\n"     // Compare lines vertically
        "movdqu %%xmm5, %%xmm1\n"
        "movdqu %%xmm5, %%xmm2\n"
        "movdqu %%xmm5, %%xmm3\n"
        "movdqu %%xmm5, %%xmm4\n"     // Copy the result
        "psrldq $1, %%xmm1\n"
        "psrldq $2, %%xmm2\n"
        "psrldq $3, %%xmm3\n"
        "psrldq $4, %%xmm4\n"         // Shift the register
        "pminub %%xmm1, %%xmm5\n"
        "pminub %%xmm2, %%xmm5\n"
        "pminub %%xmm3, %%xmm5\n"
        "pminub %%xmm4, %%xmm5\n"     // Compare lines vertically

        "psubb %%xmm5, %%xmm0\n"     // Substract the min to the max
        "movdqu %%xmm0, (%%rdi)\n"    // Result in rdi register
        "add $12, %%rdi\n"
        "add $12, %%rsi\n"
        "sub $1, %%rcx\n"
        "jnz l2\n"
        : [dst]"=m" (dst)     // outputs
        : [src]"g" (src), [i]"g" (i)// inputs
        : "rsi", "rcx", "rdi", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"  // clobbers
        );
}

void simd_min_max_7(unsigned char *src, unsigned char *dst)
{
    long i = W*(H-2*3)/(16-2*3);

    asm(
        "mov %[src], %%rsi\n"
        "mov %[i], %%rcx\n"
        "mov %[dst], %%rdi\n"
        "l3:\n"
        // MAX
        "movdqu (%%rsi), %%xmm0\n"  // Line 1
        "movdqu 1024(%%rsi), %%xmm1\n"  // Line 2
        "movdqu 2048(%%rsi), %%xmm2\n"  // Line 3
        "movdqu 3072(%%rsi), %%xmm3\n"  // Line 4
        "pmaxub %%xmm1, %%xmm0\n"
        "pmaxub %%xmm2, %%xmm0\n"
        "pmaxub %%xmm3, %%xmm0\n"     // Compare lines vertically
        "movdqu %%xmm0, %%xmm6\n"   //backup

        // Next three lines
        "movdqu 4096(%%rsi), %%xmm1\n"  // Line 5
        "movdqu 5120(%%rsi), %%xmm2\n"  // Line 6
        "movdqu 6144(%%rsi), %%xmm3\n"  // Line 7
        "pmaxub %%xmm1, %%xmm0\n"
        "pmaxub %%xmm2, %%xmm0\n"
        "pmaxub %%xmm3, %%xmm0\n"       // Compare lines vertically
        "movdqu %%xmm0, %%xmm1\n"
        "movdqu %%xmm0, %%xmm2\n"
        "movdqu %%xmm0, %%xmm3\n"     // Copy the result

        // Shift and compare again
        "psrldq $4, %%xmm1\n"
        "psrldq $5, %%xmm2\n"
        "psrldq $6, %%xmm3\n"         // Shift the register
        "pmaxub %%xmm1, %%xmm0\n"
        "pmaxub %%xmm2, %%xmm0\n"
        "pmaxub %%xmm3, %%xmm0\n"     // Compare lines vertically

        // Back to the four first lines
        "movdqu %%xmm6, %%xmm1\n"
        "movdqu %%xmm6, %%xmm2\n"
        "movdqu %%xmm6, %%xmm3\n"
        "psrldq $1, %%xmm1\n"
        "psrldq $2, %%xmm2\n"
        "psrldq $3, %%xmm3\n"         // Shift the register
        "pmaxub %%xmm1, %%xmm0\n"
        "pmaxub %%xmm2, %%xmm0\n"
        "pmaxub %%xmm3, %%xmm0\n"     // Compare lines vertically

        // MIN
        "movdqu (%%rsi), %%xmm5\n"  // Line 1
        "movdqu 1024(%%rsi), %%xmm1\n"  // Line 2
        "movdqu 2048(%%rsi), %%xmm2\n"  // Line 3
        "movdqu 3072(%%rsi), %%xmm3\n"  // Line 4
        "pminub %%xmm1, %%xmm5\n"
        "pminub %%xmm2, %%xmm5\n"
        "pminub %%xmm3, %%xmm5\n"     // Compare lines vertically
        "movdqu %%xmm5, %%xmm6\n"   //backup

        // Next three lines
        "movdqu 4096(%%rsi), %%xmm1\n"  // Line 5
        "movdqu 5120(%%rsi), %%xmm2\n"  // Line 6
        "movdqu 6144(%%rsi), %%xmm3\n"  // Line 7
        "pminub %%xmm1, %%xmm5\n"
        "pminub %%xmm2, %%xmm5\n"
        "pminub %%xmm3, %%xmm5\n"       // Compare lines vertically
        "movdqu %%xmm5, %%xmm1\n"
        "movdqu %%xmm5, %%xmm2\n"
        "movdqu %%xmm5, %%xmm3\n"     // Copy the result

        // Shift and compare again
        "psrldq $4, %%xmm1\n"
        "psrldq $5, %%xmm2\n"
        "psrldq $6, %%xmm3\n"         // Shift the register
        "pminub %%xmm1, %%xmm5\n"
        "pminub %%xmm2, %%xmm5\n"
        "pminub %%xmm3, %%xmm5\n"     // Compare lines vertically

        // Back to the four first lines
        "movdqu %%xmm6, %%xmm1\n"
        "movdqu %%xmm6, %%xmm2\n"
        "movdqu %%xmm6, %%xmm3\n"
        "psrldq $1, %%xmm1\n"
        "psrldq $2, %%xmm2\n"
        "psrldq $3, %%xmm3\n"         // Shift the register
        "pminub %%xmm1, %%xmm5\n"
        "pminub %%xmm2, %%xmm5\n"
        "pminub %%xmm3, %%xmm5\n"     // Compare lines vertically

        "psubb %%xmm5, %%xmm0\n"     // Substract the min to the max
        "movdqu %%xmm0, (%%rdi)\n"    // Result in rdi register
        "add $10, %%rdi\n"
        "add $10, %%rsi\n"
        "sub $1, %%rcx\n"
        "jnz l3\n"
        : [dst]"=m" (dst)     // outputs
        : [src]"g" (src), [i]"g" (i)// inputs
        : "rsi", "rcx", "rdi", "xmm0", "xmm1", "xmm2", "xmm3", "xmm5", "xmm6"  // clobbers
        );
}

void apply_filter(void (*c_filter)(unsigned char*, unsigned char*, int), void (*simd_filter)(unsigned char*, unsigned char*), char c_filename[25], char simd_filename[25], int window_size)
{
    printf("Size of the window is %d\n", window_size);
    unsigned char *src, *dst_c, *dst_simd;
    float dt_c, dt_simd;
    time_t start_time;
    int i;

    src = (unsigned char*) malloc(W*H*sizeof(unsigned char));
    dst_c = (unsigned char*) malloc(W*H*sizeof(unsigned char));
    dst_simd = (unsigned char*) malloc(W*H*sizeof(unsigned char));

    if (src == NULL || dst_c == NULL || dst_simd == NULL)
    {
        printf("Cannot allocate enough memory!\nExiting...\n");
        exit(1);
    }

    FILE* f1 = fopen("test.raw", "rb");
    FILE* fo_c = fopen(c_filename, "wb");
    FILE* fo_simd = fopen(simd_filename, "wb");

    if (f1 == NULL || fo_c == NULL || fo_simd == NULL)
    {
        printf("Cannot access specified file!\nExiting...\n");
        exit(1);
    }

    fread(src, sizeof(unsigned char), W*H, f1);
    fclose(f1);

    start_time = clock();
    for (i=0; i<TEST_COUNT; i++)
    {
        (*c_filter)(src, dst_c, window_size);
    }
    dt_c = (clock()-start_time)/(float)(CLOCKS_PER_SEC*TEST_COUNT);
    printf("Execution time in C : %f s\n", dt_c);

    fwrite(dst_c, sizeof(unsigned char), W*H, fo_c);
    fclose(fo_c);

    start_time = clock();
    for (i=0; i<TEST_COUNT; i++)
    {
        (*simd_filter)(src, dst_simd);
    }
    dt_simd = (clock()-start_time)/(float)(CLOCKS_PER_SEC*TEST_COUNT);
    printf("Execution time in SIMD : %f s\n", dt_simd);

    fwrite(dst_simd, sizeof(unsigned char), W*H, fo_simd);
    fclose(fo_simd);
    free(src);
    free(dst_c);
    free(dst_simd);

    printf("SIMD is %f faster than C\n\n", dt_c/dt_simd);
}

int main()
{
    apply_filter(c_min_max, simd_min_max_3, "test_min_max_3_c.raw", "test_min_max_3_simd.raw", 3);
    apply_filter(c_min_max, simd_min_max_5, "test_min_max_5_c.raw", "test_min_max_5_simd.raw", 5);
    apply_filter(c_min_max, simd_min_max_7, "test_min_max_7_c.raw", "test_min_max_7_simd.raw", 7);
    return 0;
}
