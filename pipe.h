#ifndef _PIPE_H_
#define _PIPE_H_

#include "watcher.h"

class Pipe {
    protected:
        int rd, wr;
    public:
        Pipe();
        ~Pipe();
        int useWriteEnd();
        int useReadEnd();

        Pipe(const Pipe&);
        const Pipe& operator=(const Pipe&);
};

class PipeEnd : public Listener {
    protected:
        int pipefd;
		Listener *listener;
    public:
		void action(Listener*, int);

        void dupTo(int);
		void setListener(Listener*);
        PipeEnd();
        ~PipeEnd();

        PipeEnd(const PipeEnd&);
        const PipeEnd& operator=(const PipeEnd&);
};

class WriterPipe : public PipeEnd {
    public:
        WriterPipe(Pipe&);
		ssize_t write(size_t, void*);

        WriterPipe(const WriterPipe&);
        const WriterPipe& operator=(const WriterPipe&);
};

class ReaderPipe : public PipeEnd {
    public:
        ReaderPipe(Pipe&);
		ssize_t read(size_t, void*);

        ReaderPipe(const ReaderPipe&);
        const ReaderPipe& operator=(const ReaderPipe&);
};

#endif

