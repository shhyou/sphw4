#include <cstdio>
#include <cassert>
#include <cstdint>

#include <algorithm> // std::min

#include <unistd.h>

#include "log.h"
#include "task.h"
#include "subproc.h"
#include "watcher.h"

Task::Task(int _begin, int _end) { __logc;
    begin = _begin;
    end = _end;
    ftmp = NULL;
    listener = NULL;
    proc = NULL;
    logger.print("sort interval [%d,%d), total %lld bytes", begin, end, (end - begin)*4LL);
}

Task::~Task() { __logc;
    if (ftmp)
        fclose(ftmp);
}

void Task::reset() { __logc;
    if (ftmp) {
        fflush(ftmp);
        rewind(ftmp);
    }
}

int32_t Task::getInteger() { __logc;
    int32_t m;
    if (fread(&m, 1, sizeof(m), ftmp) != 4)
        logger.raise("fread != 4");
    begin++;
    return m;
}

bool Task::eof() {
    return begin == end;
}

void Task::attach(Subprocess *_proc) { __logc;
    proc = _proc;
    proc->setListener(this);
    bytesRead = 0;
    bytesWritten = 0;
    if (ftmp)
        fclose(ftmp);
    ftmp = tmpfile();
    //char buf[64];
    //sprintf(buf, "tmpf/task%d-%d.out", begin, end);
    //ftmp = fopen(buf, "w+b");
    logger.print("attach [%d,%d) to process %p", begin, end, proc);
}

void Task::detach() { __logc;
    logger.print("detach [%d,%d) from process %p", begin, end, proc);
    proc->setListener(NULL);
    proc = NULL;
}

bool Task::finished() { __logc;
    return (bytesWritten == (end - begin)*4LL + 4)
        && (bytesRead == (end - begin)*4LL);
}

void Task::setListener(Listener *receiver) { __logc;
    listener = receiver;
}

void Task::action(Listener *obj, int e) { __logc;
    assert(proc && proc == dynamic_cast<Subprocess*>(obj));
    logger.print("this = %p, proc = %p, object = %p, e = %x", this, proc, obj, e);
    if (e & E_ERROR) {
        write(STDERR_FILENO, "SIGPIPE\n", 8);
        if (listener)
            listener->action(this, E_ERROR);
        return;
    }
    if (e & E_HANGUP)
        return;
    if ((e&E_READPIPE) && (e&E_READ) && bytesRead < (end - begin)*4LL) {
        size_t brd = proc->readFrom(sizeof(buffer), buffer);
        fwrite(buffer, 1, brd, ftmp);
        bytesRead += brd;
    }
    if ((e&E_WRITEPIPE) && (e&E_WRITE) && bytesWritten < (end - begin)*4LL + 4) {
        size_t bwr;
        if (bytesWritten == 0) { /* might cause problem if not written 4 bytes */
            int n = end - begin;
            bwr = proc->writeTo(4, &n);
            assert(bwr == 4);
        } else {
            ssize_t bytesLeft = std::min(
                    (end - begin)*4LL + 4 - bytesWritten,
                    (long long)sizeof(buffer));
            off_t offset = begin*4LL + bytesWritten;
            ssize_t pr;
            if ((pr = pread(STDIN_FILENO, buffer, bytesLeft, offset)) != bytesLeft) {
                perror("pread");
                throw logger.errmsg("pread bytesWritten=%lu bytesLeft=%lu", bytesWritten, bytesLeft);
            }
            bwr = proc->writeTo((unsigned)bytesLeft, buffer);
        }
        bytesWritten += bwr;
    }
    if (finished()) {
        if (listener)
            listener->action(this, E_FINISHED);
    }
}

