#pragma once

// 平台检测
#if defined(_WIN32) || defined(_WIN64)
    #define RB_PLATFORM_WINDOWS 1
    #define RB_PLATFORM_POSIX 0
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define RB_PLATFORM_OSX 1
    #elif TARGET_OS_IPHONE
        #define RB_PLATFORM_IOS 1
    #endif
    #define RB_PLATFORM_POSIX 1
#elif defined(__linux__)
    #define RB_PLATFORM_LINUX 1
    #define RB_PLATFORM_POSIX 1
#elif defined(__EMSCRIPTEN__)
    #define RB_PLATFORM_EMSCRIPTEN 1
    #define RB_PLATFORM_POSIX 0
#endif

// 编译器检测
#if defined(_MSC_VER)
    #define RB_COMPILER_MSVC
#elif defined(__clang__)
    #define RB_COMPILER_CLANG
#elif defined(__GNUC__)
    #define RB_COMPILER_GCC
#endif

// 架构检测
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
    #define RB_ARCH_X64 1
#elif defined(__i386__) || defined(_M_IX86)
    #define RB_ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define RB_ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
    #define RB_ARCH_ARM 1
#endif

// 平台特定的宏定义
#if RB_PLATFORM_WINDOWS
    #define RB_WINDOWS 1
    #include <windows.h>
    typedef HWND RBWindowHandle;
    typedef HINSTANCE RBNativeDisplayType;
#elif RB_PLATFORM_OSX
    #define RB_OSX 1
    typedef void* RBWindowHandle;  // NSWindow*
    typedef void* RBNativeDisplayType;  // NSView*
#elif RB_PLATFORM_LINUX
    #define RB_LINUX 1
    typedef unsigned long RBWindowHandle;  // Window (X11)
    typedef void* RBNativeDisplayType;  // Display*
#endif

// 工具宏
#define RB_COUNT_OF(_ARR) (sizeof(_ARR)/sizeof(_ARR[0]))
#define RB_UNUSED(_VAL) ((void)(_val))

// 断言
#if defined(_DEBUG) || defined(DEBUG)
    #define RB_ASSERT(_COND) do { if(!(_COND)) { RB_DEBUG_BREAK(); } } while(0)
    #define RB_DEBUG_BREAK() do { \
        #if RB_COMPILER_MSVC \
            __debugbreak(); \
        #elif RB_COMPILER_CLANG || RB_COMPILER_GCC \
            __builtin_trap(); \
        #endif \
    } while(0)
#else
    #define RB_ASSERT(_COND) ((void)0)
    #define RB_DEBUG_BREAK() ((void)0)
#endif

// 导出宏
#if RB_PLATFORM_WINDOWS
    #define RB_API_EXPORT __declspec(dllexport)
#else
    #define RB_API_EXPORT __attribute__((visibility("default")))
#endif