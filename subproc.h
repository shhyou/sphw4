#ifndef _SUBPROC_H_
#define _SUBPROC_H_

#include <ctime>

#include <sys/types.h>

#include "pipe.h"
#include "watcher.h"

#define E_READPIPE 16
#define E_WRITEPIPE 32

class Subprocess : public Listener {
	private:
		pid_t pid;
		ReaderPipe *rd;
		WriterPipe *wr;
		time_t lastResponse;

		Listener *listener;
	public:
		void action(Listener*, int);
		void poke();
		bool responsed();

		Subprocess(const char*);
		~Subprocess();
		void setListener(Listener*);
		size_t readFrom(size_t, void*);
		size_t writeTo(size_t, void*);

		Subprocess(const Subprocess&);
};

#endif

