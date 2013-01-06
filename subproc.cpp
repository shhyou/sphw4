#include <cstdio>
#include <cassert>
#include <cerrno>
#include <ctime>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include "log.h"
#include "subproc.h"
#include "pipe.h"

Subprocess::Subprocess(const char *pathname) { __logc;
    Pipe prd, pwr;
    pid_t p;
    p = fork();
    if (p < -1) {
        //perror("fork");
        throw logger.errmsg("fork");
    }
    if (p != 0) {
        wr = new WriterPipe(pwr);
        rd = new ReaderPipe(prd);
        wr->setListener(this);
        rd->setListener(this);
        pid = p;
        listener = NULL;
        logger.print("New process: pid = %d", pid);
    } else {
        (new ReaderPipe(pwr))->dupTo(STDIN_FILENO);
        (new WriterPipe(prd))->dupTo(STDOUT_FILENO);
        pid = 1;
        execl(pathname, pathname, NULL);
        //perror("execl");
        kill(getppid(), SIGTERM);
        throw logger.errmsg("execl");
    }
    poke();
}

Subprocess::~Subprocess() { __logc;
    logger.print("killing %d", pid);
    kill(pid, SIGKILL);
    delete wr;
    delete rd;
}

void Subprocess::poke() { __logc;
    lastResponse = time(NULL);
}

bool Subprocess::responsed() { __logc;
    double dif = difftime(time(NULL), lastResponse);
    return dif < 15.0 + 1e-9;
}

void Subprocess::action(Listener* obj, int e) { __logc;
    ReaderPipe *obj_r;
    WriterPipe *obj_w;

    if ((obj_w = dynamic_cast<WriterPipe*>(obj))) {
        assert(wr == obj_w);
        e |= E_WRITEPIPE;
    } else if ((obj_r = dynamic_cast<ReaderPipe*>(obj))) {
        assert(rd == obj_r);
        e |= E_READPIPE;
    } else {
        logger.raise("Unknown object");
    }

    logger.print("action, obj = %p, e = %x", obj, e);
    if (listener)
        listener->action(this, e);
}

void Subprocess::setListener(Listener *receiver) { __logc;
    listener = receiver;
}

size_t Subprocess::readFrom(size_t byte, void* buffer) { __logc;
    ssize_t res = rd->read(byte, buffer);
    if (res < 0) {
        //perror("ReaderPipe::read");
        throw logger.errmsg("ReaderPipe::read");
    }
    if (res > 0) poke();

    logger.print("%ld bytes read", res);
    return (size_t)res;
}

size_t Subprocess::writeTo(size_t byte, void* buffer) { __logc;
    ssize_t res = wr->write(byte, buffer);
    if (res < 0) {
        if (errno == EINTR) {
            res = 0;
        } else {
            //perror("WriterPipe::write");
            throw logger.errmsg("WriterPipe::write");
        }
    }
    if (res > 0) poke();

    logger.print("%ld bytes written", res);
    return (size_t)res;
}

