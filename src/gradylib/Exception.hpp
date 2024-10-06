//
// Created by Grady Schofield on 8/25/24.
//

#pragma once

#include<cxxabi.h>

#include<exception>
#include<source_location>
#include<sstream>
#include<string>

#ifdef __APPLE__

#include<execinfo.h>

namespace gradylib {
    class Exception : public std::exception {
        std::string message;
    public:
        Exception(std::string message)
                : message(message) {
        }

        Exception(std::string message, std::source_location sourceLocation) {
            std::ostringstream ostr;
            ostr << "Exception at " << sourceLocation.file_name() << "(" << sourceLocation.line() <<
                 ") in function: " << sourceLocation.function_name() << "\n";
            ostr << "Exception message: " << message << "\n";
            ostr << "Stacktrace:\n";
            void *callstack[128];
            int frames = backtrace(callstack, 128);
            char **syms = backtrace_symbols(callstack, frames);
            for (int i = 0; i < frames; ++i) {
                int status = 0;
                std::string frameLine(syms[i]);
                char *demangledName{};
                auto getMangledName = [](std::string const & s) {
                    int nBlanks = 0;
                    int len = s.length();
                    int i = 0;
                    while (true) {
                        while (i < len && !isspace(s[i])) ++i;
                        while (i < len && isspace(s[i])) ++i;
                        ++nBlanks;
                        if (nBlanks == 3) break;
                    }
                    int j = i;
                    while (j < len && !isspace(s[j])) ++j;
                    return s.substr(i, j-i);
                };
                std::string mangledName = getMangledName(frameLine);
                if (!mangledName.empty()) {
                    demangledName = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &status);
                    if (status == 0) {
                        ostr << "\t" << i << " " << demangledName << "\n";
                    } else {
                        ostr << "\t" << i << " " << mangledName << "\n";
                    }
                    free(demangledName);
                } else {
                    ostr << "\t" << i << " " << syms[i] << "\n";
                }
            }
            free(syms);
            Exception::message = ostr.str();
        }

        char const *what() const noexcept {
            return message.c_str();
        }
    };
}

#define gradylibMakeException(str) gradylib::Exception(str,std::source_location::current())

#else

#include<stacktrace>

namespace gradylib {
    class Exception : public std::exception {
        std::string message;
    public:

        Exception(std::string error, std::basic_stacktrace<std::allocator<std::stacktrace_entry>> const st = std::basic_stacktrace<std::allocator<std::stacktrace_entry>>::current()) {
            std::ostringstream sstr;
            if(error.empty()) {
                sstr << "No error message for Exception\n" << st;
            } else {
                sstr << error << "\n" << st;
            }
            message = sstr.str();
        }

        Exception(std::basic_stacktrace<std::allocator<std::stacktrace_entry>> const st = std::basic_stacktrace<std::allocator<std::stacktrace_entry>>::current())
            : Exception("", st)
        {
        }

        char const * what() const noexcept override {
            return message.c_str();
        }
    };
}

#define gradylibMakeException(str) gradylib::Exception(str)

#endif

