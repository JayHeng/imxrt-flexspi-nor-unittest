/*
 * memtester version 4
 *
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2020 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 */

#define __version__ "4.5.0"

#include "memtester.h"
#include "tests.h"

#define EXIT_FAIL_NONSTARTER    0x01
#define EXIT_FAIL_ADDRESSLINES  0x02
#define EXIT_FAIL_OTHERTEST     0x04

struct test tests[] = {
    { "Random Value", test_random_value },
    { "Compare XOR", test_xor_comparison },
    { "Compare SUB", test_sub_comparison },
    { "Compare MUL", test_mul_comparison },
    { "Compare DIV",test_div_comparison },
    { "Compare OR", test_or_comparison },
    { "Compare AND", test_and_comparison },
    { "Sequential Increment", test_seqinc_comparison },
    { "Solid Bits", test_solidbits_comparison },
    { "Block Sequential", test_blockseq_comparison },
    { "Checkerboard", test_checkerboard_comparison },
    { "Bit Spread", test_bitspread_comparison },
    { "Bit Flip", test_bitflip_comparison },
    { "Walking Ones", test_walkbits1_comparison },
    { "Walking Zeroes", test_walkbits0_comparison },
#ifdef TEST_NARROW_WRITES    
    { "8-bit Writes", test_8bit_wide_random },
    { "16-bit Writes", test_16bit_wide_random },
#endif
    { NULL, NULL }
};

/* Sanity checks and portability helper macros. */
#ifdef _SC_VERSION
void check_posix_system(void) {
    if (sysconf(_SC_VERSION) < 198808L) {
        printf("A POSIX system is required.  Don't be surprised if "
            "this craps out.\n");
        printf("_SC_VERSION is %u\n", sysconf(_SC_VERSION));
    }
}
#else
#define check_posix_system()
#endif

#ifdef _SC_PAGE_SIZE
int memtester_pagesize(void) {
    int pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize == -1) {
        perror("get page size failed");
        exit(EXIT_FAIL_NONSTARTER);
    }
    printf("pagesize is %d\n", pagesize);
    return pagesize;
}
#else
int memtester_pagesize(void) {
    printf("sysconf(_SC_PAGE_SIZE) not supported; using pagesize of 8192\n");
    return 8192;
}
#endif

/* Some systems don't define MAP_LOCKED.  Define it to 0 here
   so it's just a no-op when ORed with other constants. */
#ifndef MAP_LOCKED
  #define MAP_LOCKED 0
#endif

/* Function declarations */
void memtester_usage(char *me);

/* Global vars - so tests have access to this information */
int use_phys = 0;
off_t physaddrbase = 0;
extern int s_memtester_fail_stop;

/* Function definitions */
void memtester_usage(char *me) {
    printf("\n"
            "Usage: %s [-p physaddrbase [-d device]] <mem>[B|K|M|G] [loops]\n",
            me);
    exit(EXIT_FAIL_NONSTARTER);
}

/*******************************************************************************
* Input parameters:
* ---- phystestbase : memory base address
* ---- wantraw      : memory size
* ---- memsuffix    : memory unit, B,K,M,G for B, KB, MB, GB
* ---- loop         : memory test code loop times
* ---- pagesize     : memory pase size (Bytes)
*/
int memtester_main(ul phystestbase, ul wantraw, char *memsuffix, ul loops, ul pagesize) 
{
    ul loop, i;
    size_t wantmb, wantbytes, wantbytes_orig, bufsize,
         halflen, count;
    //char *addrsuffix, *loopsuffix;
    //ptrdiff_t pagesizemask;
    void volatile *buf, *aligned;
    ulv *bufa, *bufb;
    //int do_mlock = 1, done_mem = 0;
    int exit_code = 0;
    int /* memfd, opt,*/ memshift;
    size_t maxbytes = -1; /* addressable memory, in bytes */
    size_t maxmb = (maxbytes >> 20) + 1; /* addressable memory, in MB */
    /* Device to mmap memory from with -p, default is normal core */
    /*
    char *device_name = "/dev/mem";
    struct stat statbuf;
    int device_specified = 0;
    char *env_testmask = 0;
    */
    ul testmask = 0;

    physaddrbase = phystestbase;
    printf("Arg List: phystestbase=0x%x, wantraw=0x%x, pagesize=0x%x, loops=%d, fail_stop=%d.\n", (uint32_t)phystestbase, (uint32_t)wantraw, pagesize, loops, s_memtester_fail_stop);
    printf("memtester version " __version__ " (%d-bit)\n", UL_LEN);
    printf("Copyright (C) 2001-2020 Charles Cazabon.\n");
    printf("Licensed under the GNU General Public License version 2 (only).\n");
    printf("\n");

#if 0
    check_posix_system();
    pagesize = memtester_pagesize();
    pagesizemask = (ptrdiff_t) ~(pagesize - 1);
    printf("pagesizemask is 0x%tx\n", pagesizemask);
    
    /* If MEMTESTER_TEST_MASK is set, we use its value as a mask of which
       tests we run.
     */
    if (env_testmask = getenv("MEMTESTER_TEST_MASK")) {
        errno = 0;
        testmask = strtoul(env_testmask, 0, 0);
        if (errno) {
            printf("error parsing MEMTESTER_TEST_MASK %s: %s\n", 
                    env_testmask, strerror(errno));
            memtester_usage(argv[0]); /* doesn't return */
        }
        printf("using testmask 0x%x\n", testmask);
    }

    while ((opt = getopt(argc, argv, "p:d:")) != -1) {
        switch (opt) {
            case 'p':
                errno = 0;
                physaddrbase = (off_t) strtoull(optarg, &addrsuffix, 16);
                if (errno != 0) {
                    printf("failed to parse physaddrbase arg; should be hex "
                            "address (0x123...)\n");
                    memtester_usage(argv[0]); /* doesn't return */
                }
                if (*addrsuffix != '\0') {
                    /* got an invalid character in the address */
                    printf("failed to parse physaddrbase arg; should be hex "
                            "address (0x123...)\n");
                    memtester_usage(argv[0]); /* doesn't return */
                }
                if (physaddrbase & (pagesize - 1)) {
                    printf("bad physaddrbase arg; does not start on page "
                            "boundary\n");
                    memtester_usage(argv[0]); /* doesn't return */
                }
                /* okay, got address */
                use_phys = 1;
                break;
            case 'd':
                if (stat(optarg,&statbuf)) {
                    printf("can not use %s as device: %s\n", optarg, 
                            strerror(errno));
                    memtester_usage(argv[0]); /* doesn't return */
                } else {
                    if (!S_ISCHR(statbuf.st_mode)) {
                        printf("can not mmap non-char device %s\n", 
                                optarg);
                        memtester_usage(argv[0]); /* doesn't return */
                    } else {
                        device_name = optarg;
                        device_specified = 1;
                    }
                }
                break;              
            default: /* '?' */
                memtester_usage(argv[0]); /* doesn't return */
        }
    }

    if (device_specified && !use_phys) {
        printf("for mem device, physaddrbase (-p) must be specified\n");
        memtester_usage(argv[0]); /* doesn't return */
    }
    
    if (optind >= argc) {
        printf("need memory argument, in MB\n");
        memtester_usage(argv[0]); /* doesn't return */
    }

    errno = 0;
    wantraw = (size_t) strtoul(argv[optind], &memsuffix, 0);
    if (errno != 0) {
        printf("failed to parse memory argument");
        memtester_usage(argv[0]); /* doesn't return */
    }
#endif

    switch (*memsuffix) {
        case 'G':
        case 'g':
            memshift = 30; /* gigabytes */
            break;
        case 'M':
        case 'm':
            memshift = 20; /* megabytes */
            break;
        case 'K':
        case 'k':
            memshift = 10; /* kilobytes */
            break;
        case 'B':
        case 'b':
            memshift = 0; /* bytes*/
            break;
        case '\0':  /* no suffix */
            memshift = 20; /* megabytes */
            break;
        default:
            printf("ERR!!! memsuffix input not B|K|M|G  \r\n");
            goto __MMETESTER_EXIT__;
    }
    /*set use_phys*/
    use_phys = 1;

    wantbytes_orig = wantbytes = ((size_t) wantraw << memshift);
    wantmb = (wantbytes_orig >> 20);
    //optind++;
    if (wantmb > maxmb) {
        printf("This system can only address %u MB.\n", maxmb);
        goto __MMETESTER_EXIT__;
    }

#if 0
    if (wantbytes < pagesize) {
        printf("bytes %d < pagesize %d -- memory argument too large?\n",
                wantbytes, pagesize);
        exit(EXIT_FAIL_NONSTARTER);
    }

    if (optind >= argc) {
        loops = 0;
    } else {
        errno = 0;
        loops = strtoul(argv[optind], &loopsuffix, 0);
        if (errno != 0) {
            printf("failed to parse number of loops");
            memtester_usage(argv[0]); /* doesn't return */
        }
        if (*loopsuffix != '\0') {
            printf("loop suffix %c\n", *loopsuffix);
            memtester_usage(argv[0]); /* doesn't return */
        }
    }
#endif

    printf("want %dMB (%d bytes)\n", wantmb, wantbytes);
    buf = NULL;
#if 1
    bufsize = wantbytes; /* accept no less */
    buf = (unsigned int *)physaddrbase;
    aligned = buf;  
#else
    if (use_phys) {
        memfd = open(device_name, O_RDWR | O_SYNC);
        if (memfd == -1) {
            printf("failed to open %s for physical memory: %s\n",
                    device_name, strerror(errno));
            exit(EXIT_FAIL_NONSTARTER);
        }
        buf = (void volatile *) mmap(0, wantbytes, PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_LOCKED, memfd,
                                     physaddrbase);
        if (buf == MAP_FAILED) {
            printf("failed to mmap %s for physical memory: %s\n",
                    device_name, strerror(errno));
            exit(EXIT_FAIL_NONSTARTER);
        }

        if (mlock((void *) buf, wantbytes) < 0) {
            printf("failed to mlock mmap'ed space\n");
            do_mlock = 0;
        }

        bufsize = wantbytes; /* accept no less */
        aligned = buf;
        done_mem = 1;
    }

    while (!done_mem) {
        while (!buf && wantbytes) {
            buf = (void volatile *) malloc(wantbytes);
            if (!buf) wantbytes -= pagesize;
        }
        bufsize = wantbytes;
        printf("got  %uMB (%u bytes)", wantbytes >> 20,
            wantbytes);
        if (do_mlock) {
            printf(", trying mlock ...");
            if ((size_t) buf % pagesize) {
                /* printf("aligning to page -- was 0x%tx\n", buf); */
                aligned = (void volatile *) ((size_t) buf & pagesizemask) + pagesize;
                /* printf("  now 0x%tx -- lost %d bytes\n", aligned,
                 *      (size_t) aligned - (size_t) buf);
                 */
                bufsize -= ((size_t) aligned - (size_t) buf);
            } else {
                aligned = buf;
            }
            /* Try mlock */
            if (mlock((void *) aligned, bufsize) < 0) {
                switch(errno) {
                    case EAGAIN: /* BSDs */
                        printf("over system/pre-process limit, reducing...\n");
                        free((void *) buf);
                        buf = NULL;
                        wantbytes -= pagesize;
                        break;
                    case ENOMEM:
                        printf("too many pages, reducing...\n");
                        free((void *) buf);
                        buf = NULL;
                        wantbytes -= pagesize;
                        break;
                    case EPERM:
                        printf("insufficient permission.\n");
                        printf("Trying again, unlocked:\n");
                        do_mlock = 0;
                        free((void *) buf);
                        buf = NULL;
                        wantbytes = wantbytes_orig;
                        break;
                    default:
                        printf("failed for unknown reason.\n");
                        do_mlock = 0;
                        done_mem = 1;
                }
            } else {
                printf("locked.\n");
                done_mem = 1;
            }
        } else {
            done_mem = 1;
            printf("\n");
        }
    }

    if (!do_mlock) printf("Continuing with unlocked memory; testing "
                           "will be slower and less reliable.\n");

    /* Do alighnment here as well, as some cases won't trigger above if you
       define out the use of mlock() (cough HP/UX 10 cough). */
    if ((size_t) buf % pagesize) {
        /* printf("aligning to page -- was 0x%tx\n", buf); */
        aligned = (void volatile *) ((size_t) buf & pagesizemask) + pagesize;
        /* printf("  now 0x%tx -- lost %d bytes\n", aligned,
         *      (size_t) aligned - (size_t) buf);
         */
        bufsize -= ((size_t) aligned - (size_t) buf);
    } else {
        aligned = buf;
    }
#endif

    halflen = bufsize / 2;
    count = halflen / sizeof(ul);
    bufa = (ulv *) aligned;
    bufb = (ulv *) ((size_t) aligned + halflen);

    for(loop=1; ((!loops) || loop <= loops); loop++) {
        printf("Loop %d", loop);
        if (loops) {
            printf("/%d", loops);
        }
        printf(":\n");
        printf("  %s: ", "Stuck Address");
        if (!test_stuck_address(aligned, bufsize / sizeof(ul))) {
             printf("ok\n");
        } else {
            exit_code |= EXIT_FAIL_ADDRESSLINES;
            if (s_memtester_fail_stop)
              break;
        }
        for (i=0;;i++) {
            if (!tests[i].name) break;
            /* If using a custom testmask, only run this test if the
               bit corresponding to this test was set by the user.
             */
            if (testmask && (!((1 << i) & testmask))) {
                continue;
            }
            printf("  %s: ", tests[i].name);
            if (!tests[i].fp(bufa, bufb, count)) {
                printf("ok\n");
            } else {
                exit_code |= EXIT_FAIL_OTHERTEST;
                if (s_memtester_fail_stop)
                  break;
            }
            /* clear buffer */
            memset((void *) buf, 255, wantbytes);
        }
        printf("\n");
    }

#if 1
    if (exit_code)
      printf("Done and Failed!\r\n");
    else
      printf("Done and Passed!\r\n");
    printf("exit_code 0x%x \r\n", exit_code);
#else
    if (do_mlock) munlock((void *) aligned, bufsize);
    printf("Done.\n");
    exit(exit_code);
#endif

__MMETESTER_EXIT__:
    //printf("\a");
    return 0;
}
