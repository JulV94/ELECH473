#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define W 1024
#define H 1024
#define TEST_COUNT 1000
#define THRESHOLD 80 // Between 0 and 255

void c_bw(unsigned char *src, unsigned char *dst)
{
    int i;

    for (i=0; i<H*W; i++)
    {
        dst[i] = (src[i] > THRESHOLD) ? 255 : 0;
    }
}

void simd_bw(unsigned char *src, unsigned char *dst)
{
    long i = W*H/16;
    char threshold[16];
    memset(threshold, THRESHOLD, 16*sizeof(char));

    asm(
        "mov %[src], %%rsi\n"
        "mov %[i], %%rcx\n"
        "mov %[dst], %%rdi\n"
        "mov %[threshold], %%rax\n"
        "movapd (%%rax), %%xmm7\n"
        "l1:\n"
        "movapd (%%rsi), %%xmm0\n"
        "pminub %%xmm7, %%xmm0\n"
        "pcmpeqb %%xmm7, %%xmm0\n"
        "movapd %%xmm0, (%%rdi)\n"
        "add $16, %%rdi\n"
        "add $16, %%rsi\n"
        "sub $1, %%rcx\n"
        "jnz l1\n"
        : [dst]"=m" (dst)     // outputs
        : [src]"g" (src), [i]"g" (i), [threshold]"g" (threshold) // inputs
        : "rsi", "rcx", "rdi", "rax", "xmm0", "xmm7"  // clobbers
        );
}

int main()
{
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
    FILE* fo_c = fopen("test_bw_c.raw", "wb");
    FILE* fo_simd = fopen("test_bw_simd.raw", "wb");

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
        c_bw(src, dst_c);
    }
    dt_c = (clock()-start_time)/(float)(CLOCKS_PER_SEC*TEST_COUNT);
    printf("Execution time in C : %f s\n", dt_c);

    fwrite(dst_c, sizeof(unsigned char), W*H, fo_c);
    fclose(fo_c);

    start_time = clock();
    for (i=0; i<TEST_COUNT; i++)
    {
        simd_bw(src, dst_simd);
    }
    dt_simd = (clock()-start_time)/(float)(CLOCKS_PER_SEC*TEST_COUNT);
    printf("Execution time in SIMD : %f s\n", dt_simd);

    fwrite(dst_simd, sizeof(unsigned char), W*H, fo_simd);
    fclose(fo_simd);
    free(src);
    free(dst_c);
    free(dst_simd);

    printf("SIMD is %f faster than C", dt_c/dt_simd);
    return 0;
}
