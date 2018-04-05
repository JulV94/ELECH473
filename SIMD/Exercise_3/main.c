#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#define W 1024
#define H 1024
#define THREAD_COUNT 2
#define TEST_COUNT 1000
#define THRESHOLD 80
#define WINDOW_SIZE 3
#define BORDER_SIZE (WINDOW_SIZE-1)/2

typedef struct thread_args_s{
    int thread_index;
    unsigned char *src;
    unsigned char *dst;
} thread_args_s;

void* thread_c_bw(void *args)
{
    thread_args_s *arguments = (thread_args_s*)args;
    int i, thread_work_size;
    thread_work_size = H*W/THREAD_COUNT;

    for (i=arguments->thread_index*thread_work_size; i<(arguments->thread_index+1)*thread_work_size; i++)
    {
        arguments->dst[i] = (arguments->src[i] > THRESHOLD) ? 255 : 0;
    }
    pthread_exit(NULL);
}

void* thread_simd_bw(void *args)
{
    thread_args_s *arguments = (thread_args_s*)args;
    int thread_work_size = W*H/THREAD_COUNT;
    int ptr_offset = arguments->thread_index*thread_work_size;
    long i = thread_work_size/16;
    unsigned char *src = arguments->src+ptr_offset;
    unsigned char *dst = arguments->dst+ptr_offset;
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

    pthread_exit(NULL);
}

void* thread_c_min_max(void *args)
{
    thread_args_s *arguments = (thread_args_s*)args;
    int i, j, k, l, min_buffer, max_buffer, thread_work_size;
    thread_work_size = (H-2*BORDER_SIZE)*W/THREAD_COUNT;

    for (i=BORDER_SIZE+arguments->thread_index*thread_work_size; i<(arguments->thread_index+1)*thread_work_size; i++)
    {
        for (j=BORDER_SIZE; j<(W-BORDER_SIZE);j++)
        {
            min_buffer = 255;
            max_buffer = 0;
            for (k=i-BORDER_SIZE;k<=(i+BORDER_SIZE);k++)
            {
                for (l=j-BORDER_SIZE;l<=(j+BORDER_SIZE);l++)
                {
                    if (arguments->src[W*k+l] > max_buffer)
                    {
                        max_buffer = arguments->src[W*k+l];
                    }
                    if (arguments->src[W*k+l] < min_buffer)
                    {
                        min_buffer = arguments->src[W*k+l];
                    }
                }
            }
            arguments->dst[W*i+j] = max_buffer - min_buffer;
        }
    }
    pthread_exit(NULL);
}

void* thread_simd_min_max(void *args)
{
    thread_args_s *arguments = (thread_args_s*)args;
    int thread_work_size = W*(H-2*BORDER_SIZE)/THREAD_COUNT;
    int ptr_offset = arguments->thread_index*thread_work_size;
    long i = thread_work_size/(16-2*BORDER_SIZE);
    unsigned char *src = arguments->src+ptr_offset;
    unsigned char *dst = arguments->dst+ptr_offset;

    asm(
        "mov %[src], %%rsi\n"
        "mov %[i], %%rcx\n"
        "mov %[dst], %%rdi\n"
        "l2:\n"
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
        "pmaxub %%xmm7, %%xmm5\n"
        "pmaxub %%xmm0, %%xmm5\n"     // Compare lines vertically

        "psubb %%xmm5, %%xmm6\n"     // Substract the min to the max
        "movdqu %%xmm6, (%%rdi)\n"    // Result in rdi register
        "add $14, %%rdi\n"
        "add $14, %%rsi\n"
        "sub $1, %%rcx\n"
        "jnz l2\n"
        : [dst]"=m" (dst)     // outputs
        : [src]"g" (src), [i]"g" (i)// inputs
        : "rsi", "rcx", "rdi", "xmm0", "xmm1", "xmm2", "xmm5", "xmm6", "xmm7"  // clobbers
        );

    pthread_exit(NULL);
}

void apply_filter(void* (*c_filter)(void*), void* (*simd_filter)(void*), char c_filename[25], char simd_filename[25])
{
    unsigned char *src, *dst_c, *dst_simd;
    float dt_c, dt_simd;
    time_t start_time;
    int i, j;
    pthread_t threads[THREAD_COUNT];
    thread_args_s threads_args[THREAD_COUNT];

    printf("Creating %d threads\n", THREAD_COUNT);

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

    for (int j=0;j<THREAD_COUNT;j++)
    {
        threads_args[j].thread_index = j;
        threads_args[j].src = src;
        threads_args[j].dst = dst_c;
    }

    start_time = clock();
    for (i=0; i<TEST_COUNT; i++)
    {
        for (j=0;j<THREAD_COUNT;j++)
        {
            pthread_create(&threads[j], NULL, (*c_filter), (void*)&threads_args[j]);
        }
        for (j=0;j<THREAD_COUNT;j++)
        {
            pthread_join(threads[j], NULL);
        }
    }
    dt_c = (clock()-start_time)/(float)(CLOCKS_PER_SEC*TEST_COUNT);
    printf("Execution time in C : %f s\n", dt_c);

    fwrite(dst_c, sizeof(unsigned char), W*H, fo_c);
    fclose(fo_c);

    for (int j=0;j<THREAD_COUNT;j++)
    {
        threads_args[j].dst = dst_simd;
    }

    start_time = clock();
    for (i=0; i<TEST_COUNT; i++)
    {
        for (j=0;j<THREAD_COUNT;j++)
        {
            pthread_create(&threads[j], NULL, (*simd_filter), (void*)&threads_args[j]);
        }
        for (j=0;j<THREAD_COUNT;j++)
        {
            pthread_join(threads[j], NULL);
        }
    }
    dt_simd = (clock()-start_time)/(float)(CLOCKS_PER_SEC*TEST_COUNT);
    printf("Execution time in SIMD : %f s\n", dt_simd);

    fwrite(dst_simd, sizeof(unsigned char), W*H, fo_simd);
    fclose(fo_simd);
    free(src);
    free(dst_c);
    free(dst_simd);

    printf("SIMD is %f faster than C\n", dt_c/dt_simd);
}

int main()
{
    apply_filter(thread_c_bw, thread_simd_bw, "test_bw_c.raw", "test_bw_simd.raw");
    apply_filter(thread_c_min_max, thread_simd_min_max, "test_min_max_c.raw", "test_min_max_simd.raw");
    return 0;
}
