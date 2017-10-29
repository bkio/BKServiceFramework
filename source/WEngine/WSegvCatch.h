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
    #include <csignal>
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <execinfo.h>
    #include <string>

    #define MAKE_THROW_FRAME(_exception)

    /* The return code for realtime-signals.  */
    /* __NR_rt_sigreturn defined as 15 in unistd64.h */

    asm
    (
        ".text\n"
        ".byte 0\n"
        ".align 16\n"
        "__restore_rt:\n"
        "	movq $15, %rax\n"
        "	syscall\n"
    );
    void restore_rt () asm ("__restore_rt") __attribute__ ((visibility ("hidden")));

    /* Unblock a signal.  Unless we do this, the signal may only be sent
       once.  */
    static void UnblockSignal(int signum __attribute__((__unused__)))
    {
        sigset_t sigs{};
        sigemptyset(&sigs);
        sigaddset(&sigs, signum);
        sigprocmask(SIG_UNBLOCK, &sigs, nullptr);
    }

    static void CatchSegv (int, siginfo_t *, void *_p __attribute__ ((__unused__)))
    {
        UnblockSignal(SIGSEGV);
        MAKE_THROW_FRAME(nullp);
        throw std::runtime_error("Segmentation fault");
    }

    struct KernelSigaction
    {
        void (*k_sa_sigaction)(int, siginfo_t *, void *);
        unsigned long k_sa_flags;
        void (*k_sa_restorer) ();
        sigset_t k_sa_mask;
    };

    void InitWSegv()
    {
        KernelSigaction act{};
        act.k_sa_sigaction = CatchSegv;
        sigemptyset(&act.k_sa_mask);
        act.k_sa_flags = SA_SIGINFO | 0x4000000;
        act.k_sa_restorer = restore_rt;
        syscall(SYS_rt_sigaction, SIGSEGV, &act, nullptr, _NSIG / 8);
    }
#endif

#endif //Pragma_Once_WSegvCatch