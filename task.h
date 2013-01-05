#ifndef _TASK_H_
#define _TASK_H_

#include <cstdio>
#include <cstdint>

#include "subproc.h"

#define E_FINISHED 64

class Task : public Listener {
	private:
		int begin, end; // index
		FILE *ftmp;
		Subprocess *proc;
        Listener *listener;

		int bytesRead, bytesWritten;
		char buffer[65536];
	public:
		void action(Listener*, int);

		Task(int, int);
		~Task();
		void attach(Subprocess*);
		void detach();
        bool finished();
        void setListener(Listener*);
        void reset();
        int32_t getInteger();
        bool eof();

		Task(const Task&);
		const Task& operator=(const Task&);
};

#endif

