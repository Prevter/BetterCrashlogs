#if defined(__APPLE__) || defined(__MACH__)
#include "MacOS.hpp"

#include <array>
#include <thread>
#include <filesystem>
#include <mutex>
#include <functional>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <algorithm>

#include <mach-o/dyld_images.h>
#include <mach-o/dyld.h>
#import <Foundation/Foundation.h>

#include "../../crashhandler/crashhandler.hpp"

namespace breakdown::platform {

    std::function<void(CrashHandler const&)> s_crashHandler;

    constexpr size_t FRAME_SIZE = 64;
    static std::array<void*, FRAME_SIZE> s_backtrace;
    static size_t s_backtraceSize = 0;
    static std::mutex s_mutex;
    static std::condition_variable s_cv;
    static int s_signal = 0;
    static siginfo_t* s_signalInfo = nullptr;
    static ucontext_t* s_context = nullptr;

    static std::string_view getSignalCodeString() {
        switch(s_signal) {
            case SIGSEGV: return "SIGSEGV: Segmentation Fault";
            case SIGINT: return "SIGINT: Interactive attention signal, (usually ctrl+c)";
            case SIGFPE:
                switch(s_signalInfo->si_code) {
                    case FPE_INTDIV: return "SIGFPE: (integer divide by zero)";
                    case FPE_INTOVF: return "SIGFPE: (integer overflow)";
                    case FPE_FLTDIV: return "SIGFPE: (floating-point divide by zero)";
                    case FPE_FLTOVF: return "SIGFPE: (floating-point overflow)";
                    case FPE_FLTUND: return "SIGFPE: (floating-point underflow)";
                    case FPE_FLTRES: return "SIGFPE: (floating-point inexact result)";
                    case FPE_FLTINV: return "SIGFPE: (floating-point invalid operation)";
                    case FPE_FLTSUB: return "SIGFPE: (subscript out of range)";
                    default: return "SIGFPE: Arithmetic Exception";
                }
            case SIGILL:
                switch(s_signalInfo->si_code) {
                    case ILL_ILLOPC: return "SIGILL: (illegal opcode)";
                    case ILL_ILLOPN: return "SIGILL: (illegal operand)";
                    case ILL_ILLADR: return "SIGILL: (illegal addressing mode)";
                    case ILL_ILLTRP: return "SIGILL: (illegal trap)";
                    case ILL_PRVOPC: return "SIGILL: (privileged opcode)";
                    case ILL_PRVREG: return "SIGILL: (privileged register)";
                    case ILL_COPROC: return "SIGILL: (coprocessor error)";
                    case ILL_BADSTK: return "SIGILL: (internal stack error)";
                    default: return "SIGILL: Illegal Instruction";
                }
            case SIGTERM: return "SIGTERM: a termination request was sent to the program";
            case SIGABRT: return "SIGABRT: usually caused by an abort() or assert()";
            case SIGBUS: return "SIGBUS: Bus error (bad memory access)";
            default: return "Unknown signal code";
        }
    }

    extern "C" void handleCrash(int signal, siginfo_t* signalInfo, void* vcontext) {
        auto* context = static_cast<ucontext_t*>(vcontext);
        s_backtraceSize = backtrace(s_backtrace.data(), FRAME_SIZE);

        #ifdef GEODE_IS_INTEL_MAC
        s_backtrace[2] = reinterpret_cast<void*>(context->uc_mcontext->__ss.__rip);
        #else
        s_backtrace[2] = reinterpret_cast<void*>(context->uc_mcontext->__ss.__pc);
        #endif

        if (s_backtraceSize < FRAME_SIZE) {
            s_backtrace[s_backtraceSize] = nullptr;
        }

        {
            std::unique_lock<std::mutex> lock(s_mutex);
            s_signal = signal;
            s_signalInfo = signalInfo;
            s_context = context;
        }

        s_cv.notify_all();
        std::unique_lock<std::mutex> lock(s_mutex);
        s_cv.wait(lock, [] { return s_signal == 0; });
        std::_Exit(EXIT_FAILURE);
    }

    static void handleThread() {
        std::unique_lock<std::mutex> lock(s_mutex);
        s_cv.wait(lock, [] { return s_signal != 0; });

        #ifdef GEODE_IS_INTEL_MAC
        auto signalAddr = reinterpret_cast<void*>(s_context->uc_mcontext->__ss.__rip);
        #else
        auto signalAddr = reinterpret_cast<void*>(s_context->uc_mcontext->__ss.__pc);
        #endif

        auto addr = reinterpret_cast<uintptr_t>(signalAddr);
        ExceptionInfo exceptionInfo {
            addr, static_cast<size_t>(s_signal), std::string(getSignalCodeString())
        };

        s_crashHandler(CrashHandler(exceptionInfo));

        s_signal = 0;
        s_cv.notify_all();
    }

    void registerCrashHandler(std::function<void(CrashHandler const&)> handler) {
        s_crashHandler = std::move(handler);
        struct sigaction action;
        action.sa_sigaction = &handleCrash;
        action.sa_flags = SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        sigaction(SIGSEGV, &action, nullptr);
        sigaction(SIGFPE, &action, nullptr);
        sigaction(SIGILL, &action, nullptr);
        sigaction(SIGTERM, &action, nullptr);
        sigaction(SIGABRT, &action, nullptr);
        sigaction(SIGBUS, &action, nullptr);

        std::thread(&handleThread).detach();
    }

}

#endif
