//
// Created by Grady Schofield on 8/25/24.
//

#ifndef GRADY_LIB_EXCEPTION_HPP
#define GRADY_LIB_EXCEPTION_HPP

#include<cxxabi.h>

#include<exception>
#include<source_location>
#include<sstream>
#include<string>

#ifdef __APPLE__

#include<execinfo.h>
#include<regex>

class Exception : public std::exception{
    std::string message;
public:
    Exception(std::string message)
        : message(message)
    {
    }

    Exception(std::string message, std::source_location sourceLocation) {
        std::ostringstream ostr;
        ostr << "Exception at " << sourceLocation.file_name() << "(" << sourceLocation.line() <<
            ") in function: " << sourceLocation.function_name() << "\n";
        ostr << "Exception message: " << message << "\n";
        ostr << "Stacktrace:\n";
        void * callstack[128];
        int frames = backtrace(callstack, 128);
        char ** syms = backtrace_symbols(callstack, frames);
        for (int i = 0; i < frames; ++i) {
            int status = 0;
            std::regex pattern(R"((\S+)\s+(\S+)\s+(\S+)\s+(\S+))");
            std::smatch match;
            std::string mangledName;
            std::string frameLine(syms[i]);
            char * demangledName{};
            if (std::regex_search(frameLine, match, pattern)) {
                if (match.size() > 4) {
                    mangledName = match[4];
                    demangledName = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &status);
                    if (status == 0) {
                        ostr << "\t" << i << " " << demangledName << std::endl;
                    } else {
                        ostr << "\t" << i << " " << mangledName << std::endl;
                    }
                    free(demangledName);
                }
            }
        }
        free(syms);
        Exception::message  = ostr.str();
    }

    char const * what() const noexcept {
        return message.c_str();
    }
};

#else

#endif

#define gradylibMakeException(str) Exception(str,std::source_location::current())

#endif //GRADY_LIB_EXCEPTION_HPP
