#ifndef POPEN_STREAMBUF_H
#define POPEN_STREAMBUF_H
#include <stdio.h>
#include <iostream>

//#define BUFFER_SIZE     1024

class popen_streambuf : public std::streambuf {
	enum{BUFFER_SIZE=1024};
	FILE *fp;
	char_type *buffer;
public:
    popen_streambuf() : fp(NULL) {
    }
    ~popen_streambuf() {
        close();
    }
    popen_streambuf *open(const char *command, const char *mode) {
        fp = popen(command, mode);
        if (fp == NULL)
            return NULL;
        buffer = new char_type[BUFFER_SIZE];
        // It's good to check because exceptions can be disabled
        if (buffer == NULL) {
            close();
            return NULL;
        }
        setg(buffer, buffer, buffer);
        return this;
    }
    void close() {
        if (fp != NULL) {
            pclose(fp);
            fp = NULL;
        }
    }
    std::streamsize xsgetn(char_type *ptr, std::streamsize n) {
        std::streamsize got = showmanyc();
        if (n <= got) {
            memcpy(ptr, gptr(), n * sizeof(char_type));
            gbump(n);
            return n;
        }
        memcpy(ptr, gptr(), got * sizeof(char_type));
        gbump(got);

        if (traits_type::eof() == underflow()) {
            return got;
        }
        return (got + xsgetn(ptr + got, n - got));
    }
    int_type underflow() {
        if (gptr() == 0) {
            return traits_type::eof();
        }
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }
        size_t len = fread(eback(), sizeof(char_type), BUFFER_SIZE, fp);
        setg(eback(), eback(), eback() + (sizeof(char_type) * len));
        if (0 == len) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(*gptr());
    }
    std::streamsize showmanyc() {
        if (gptr() == 0) {
           return 0;
        }
        if (gptr() < egptr()) {
            return egptr() - gptr();
        }
        return 0; 
    }
};
#endif
