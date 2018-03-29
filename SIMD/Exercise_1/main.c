#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define W 1024
#define H 1024
#define TRESHOLD 80 // Between 0 and 255

void c_bw(unsigned char *src, unsigned char *dst)
{
    float dt;
    time_t start_time = clock();
    int i;

    for (i=0; i<H*W; i++)
    {
        dst[i] = (src[i] > TRESHOLD) ? 255 : 0;
    }
    dt = (clock()-start_time)/(float)(CLOCKS_PER_SEC);
    printf("Execution time in C : %f s\n", dt);
}

void simd_bw(unsigned char *src, unsigned char *dst)
{
    float dt;
    time_t start_time = clock();
    long i = 3;//W*H/16;
    char treshold[16];
    memset(treshold, TRESHOLD, 16*sizeof(char));
    printf("%d", treshold[3]);

    asm(
        "mov %[src], %%rsi\n"
        "mov %[i], %%rcx\n"
        "mov %[dst], %%rdi\n"
        "mov %[treshold], %%rax\n"
        "movapd (%%rax), %%xmm7\n"
        "l1:\n"
        "movapd (%%rsi), %%xmm0\n"
        "pcmpeqb %%xmm7, %%xmm0\n"
        "movapd %%xmm0, (%%rdi)\n"
        "add $16, %%rdi\n"
        "add $16, %%rsi\n"
        "sub $1, %%rcx\n"
        "jnz l1\n"
        : [dst]"=m" (dst)     // outputs
        : [src]"g" (src), [i]"g" (i), [treshold]"g" (treshold)  // inputs
        : "rsi", "rcx", "rdi", "rax", "xmm0", "xmm7"  // clobbers
        );

    dt = (clock()-start_time)/(float)(CLOCKS_PER_SEC);
    printf("Execution time in SIMD : %f s\n", dt);
}

int main()
{
    unsigned char *src, *dst_c, *dst_simd;

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

    c_bw(src, dst_c);

    fwrite(dst_c, sizeof(unsigned char), W*H, fo_c);
    fclose(fo_c);

    simd_bw(src, dst_simd);

    fwrite(dst_simd, sizeof(unsigned char), W*H, fo_simd);
    fclose(fo_simd);
    free(src);
    free(dst_c);
    free(dst_simd);
    return 0;
}
