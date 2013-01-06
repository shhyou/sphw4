#include <cstdio>
#include <cassert>

#include <unistd.h>
#include <fcntl.h>

#include "log.h"
#include "pipe.h"
#include "watcher.h"

Pipe::Pipe() { __logc;
    int fd[2];
    if (pipe2(fd, O_CLOEXEC) < 0) {
        //perror("pipe");
        throw logger.errmsg("pipe");
    }
    rd = fd[0];
    wr = fd[1];
    logger.print("new pipe: %d->%d", fd[1], fd[0]);
}

Pipe::~Pipe() { __logc;
    close(wr);
    close(rd);
}

int Pipe::useWriteEnd() { __logc;
    int fd = wr;
    close(rd);
    wr = -1;
    rd = -1;
    return fd;
}

int Pipe::useReadEnd() { __logc;
    int fd = rd;
    close(wr);
    wr = -1;
    rd = -1;
    return fd;
}

PipeEnd::PipeEnd() { __logc;
    pipefd = -1;
    listener = NULL;
}

PipeEnd::~PipeEnd() { __logc;
    Watcher::getInstance().unwatch(pipefd);
    close(pipefd);
    logger.print("unwatching %d", pipefd);
}

void PipeEnd::dupTo(int newfd) { __logc;
    int fd = dup2(pipefd, newfd);
    if (fd < 0) {
        //perror("dupTo");
        throw logger.errmsg("dupTo");
    }
}

void PipeEnd::setListener(Listener* receiver) { __logc;
    listener = receiver;
    Watcher::getInstance().watch(pipefd, this);
    logger.print("watching %d", pipefd);
}

void PipeEnd::action(Listener *obj, int e) { __logc;
    assert(this == dynamic_cast<PipeEnd*>(obj));
    logger.print("action, obj = %p, e = %x", obj, e);
    if (listener)
        listener->action(this, e);
}

WriterPipe::WriterPipe(Pipe& p) { __logc;
    pipefd = p.useWriteEnd();
}

ssize_t WriterPipe::write(size_t byte, void* buffer) { __logc;
    return ::write(pipefd, buffer, byte);
}

ReaderPipe::ReaderPipe(Pipe& p) { __logc;
    pipefd = p.useReadEnd();
}

ssize_t ReaderPipe::read(size_t byte, void* buffer) { __logc;
    return ::read(pipefd, buffer, byte);
}
