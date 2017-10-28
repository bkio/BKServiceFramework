// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WSegvCatch
#define Pragma_Once_WSegvCatch

#include "WEngine.h"
#include <stdexcept>

#if PLATFORM_WINDOWS
    #include <windows.h>

    static LONG CALLBACK win32_exception_handler(LPEXCEPTION_POINTERS e)
    {
        if (e->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            throw std::runtime_error("Segmentation fault");
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

    void InitWSegv()
    {
        SetUnhandledExceptionFilter(win32_exception_handler);
    }
#else
    #include <signal.h>
    #include <sys/syscall.h>
    #include <execinfo.h>
    #include <string>

    extern "C"
    {
        struct KernelSigaction
        {
            void (*k_sa_sigaction)(int, siginfo_t *, void *);
            unsigned long k_sa_flags;
            void (*k_sa_restorer) (void);
            sigset_t k_sa_mask;
        };
    }

    #define MAKE_THROW_FRAME(_exception)

    /* The return code for realtime-signals.  */
    asm
    (
        ".text\n"
        ".byte 0\n"
        ".align 16\n"
        "asm_restore_rt:\n"
        "	movq $__NR_rt_sigreturn, %rax\n"
        "	__NR_rt_sigreturn\n"
    );
    void restore_rt (void) asm ("asm_restore_rt")
      __attribute__ ((visibility ("hidden")));

    /* Unblock a signal.  Unless we do this, the signal may only be sent
       once.  */
    static void UnblockSignal(int signum __attribute__((__unused__)))
    {
#ifdef _POSIX_VERSION
        sigset_t sigs;
        sigemptyset(&sigs);
        sigaddset(&sigs, signum);
        sigprocmask(SIG_UNBLOCK, &sigs, NULL);
#endif
    }

    static void CatchSegv (int, siginfo_t *, void *_p __attribute__ ((__unused__)))
    {
        UnblockSignal(SIGSEGV);
        MAKE_THROW_FRAME(nullp);
        throw std::runtime_error("Segmentation fault");
    }

    void InitWSegv()
    {
        do
        {
            struct KernelSigaction act;
            act.k_sa_sigaction = CatchSegv;
            sigemptyset (&act.k_sa_mask);
            act.k_sa_flags = SA_SIGINFO|0x4000000;
            act.k_sa_restorer = restore_rt;
            syscall (SYS_rt_sigaction, SIGSEGV, &act, NULL, _NSIG / 8);
        }
        while (0);
    }
#endif

#endif //Pragma_Once_WSegvCatch