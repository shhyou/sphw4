#include <errno.h>
#include <netdb.h>

#include <cstdarg>
#include <cstring>

#include <string>
#include <vector>

#define _LOG_C
#include "log.h"

using std::string;
using std::vector;

string stktrace() {
	string res;
	for (vector<string>::iterator it=callstk.begin(); it!=callstk.end(); it++) {
		res += *it;
		if (it+1 != callstk.end()) res += "!";
	}
	return res;
}

string strerrmsg(const char *fmt, va_list args, const char *err_str) {
	static char buf[4096];
	vsprintf(buf, fmt, args);
	return stktrace() + ": " + buf + " (" + err_str + ")";
}

logprint_t::logprint_t(const char *_type, FILE *_output) : type(_type), output(_output) {}
void logprint_t::operator()(const char *fmt, ...) const {
	static char buf[256];
	va_list args;
	time_t tm = time(NULL);
    int len, i;
    char ch;
return;//FOR SUBMITTING
	va_start(args, fmt);
	strftime(buf, sizeof(buf), "%Y%m%d %a %H:%M:%S", localtime(&tm));

	fprintf(output, "[%s] ", type);
	fprintf(output, "[\x1b[1;33m%s\x1b[m] ", buf);
	fprintf(output, "\x1b[1;30m%s\x1b[m", stktrace().c_str());

    buf[sizeof(buf) - 1] = 0;
	len = vsnprintf(buf, sizeof(buf) - 1, fmt, args);

    for (i = 0; i < len; i += 93) {
        fprintf(output, "\n\x1b[1;30m|    |\x1b[m ");
        if (i+93 < len) {
            ch = buf[i+93];
            buf[i+93] = 0;
            fprintf(output, "%s", buf + i);
            buf[i+93] = ch;
        } else {
            fprintf(output, "%s", buf + i);
        }
    }

	fprintf(output, "\n");
    va_end(args);
}

log_t::log_t(const char* t, const char* s)
	: print("\x1b[1;32mINFO\x1b[m", stdout), eprint("\x1b[1;31mERR \x1b[m", stderr) {
	callstk.push_back(t);
	callstk.back() += "::";
	callstk.back() += s;
}

log_t::log_t(const char* s)
	: print("\x1b[1;32mINFO\x1b[m", stdout), eprint("\x1b[1;31mERR \x1b[m", stderr) {
	callstk.push_back(s);
}

log_t::~log_t() { callstk.pop_back(); }

string log_t::trace() const { return stktrace(); }

string log_t::raise(const char *fmt, ...) const {
	va_list args;
	va_start(args, fmt);
	string res(strerrmsg(fmt, args, "raise"));
	va_end(args);
	throw res;
	/* should not be executed */
	return res;
}

string log_t::errmsg(const char *fmt, ...) const {
	va_list args;
	va_start(args, fmt);
	string res(strerrmsg(fmt, args, strerror(errno)));
	va_end(args);
	return res;
}

string log_t::herrmsg(const char *fmt, ...) const {
	va_list args;
	va_start(args, fmt);
	string res(strerrmsg(fmt, args, hstrerror(h_errno)));
	va_end(args);
	return res;
}
