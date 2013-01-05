#include <cstdio>
#include <cstring>
#include <csignal>
#include <cassert>

#include <string>
#include <algorithm> // swap, find, for_each, push_heap, pop_heap
#include <unordered_set>

#include <unistd.h>

#include "log.h"
#include "watcher.h"
#include "subproc.h"
#include "task.h"

using std::string;
using std::unordered_set;

const int PMAX = 16;
const int NMAX = 256;
const int ALRM_PERIOD = 3;

class Sched : public Listener {
    private:
        int P, N, K;
        string sorting;
        unordered_set<Task*> pending, running;
        Task *tasks[NMAX], *assoc[PMAX];
        Subprocess *procs[PMAX];
        bool busy[PMAX];
    public:
        void action(Listener*, int);
        Sched(int, char **);
        ~Sched();
        int run();
};

Sched::Sched(int argc, char **argv) { __logc;
    int i, j, k, n = 180;
    Task* t;
    if ((argc - 1)%2 != 0) {
        fprintf(stderr, "Usage: %s -p P -n N -e SORTING_PROG -k K\n", argv[0]);
        throw "wrong argc";
    }
    P = 1;
    N = 1;
    K = 1;
    sorting = "./sorting";
    for (i = 1; i < argc; i+=2) {
        if (!strcmp(argv[i], "-p")) {
            if (sscanf(argv[i+1], "%d", &P) != 1)
                throw "-p parameter error.";
        } else if (!strcmp(argv[i], "-n")) {
            if (sscanf(argv[i+1], "%d", &N) != 1)
                throw "-n parameter error.";
        } else if (!strcmp(argv[i], "-e")) {
            sorting = argv[i+1];
        } else if (!strcmp(argv[i], "-k")) {
            if (sscanf(argv[i+1], "%d", &K) != 1)
                throw "-k parameter error.";
        } else {
            throw "unknown parameter";
        }
    }
    if (read(STDIN_FILENO, &n, 4) != 4)
        logger.raise("read n");
 
    memset(procs, 0, sizeof(procs));
    memset(busy, 1, sizeof(busy));
    for (i = 0; i < P; i++) {
        procs[i] = new Subprocess(sorting.c_str());
        busy[i] = false;
    }
    for (i = 0, j = 0; i < N; i++, j = k) {
        if (i+1 == N) k = n;
        else k = j + n/N;

        t = new Task(j, k);
        t->setListener(this);
        tasks[i] = t;
        pending.insert(t);
    }
}

Sched::~Sched() { __logc;
    int i;
    std::for_each(tasks, tasks + N, [](Task* t) { delete t; });
    for (i = 0; i < P; i++)
        delete procs[i];
}

int Sched::run() { __logc;
    int i;
    for (i = 0; i < P; i++) {
        auto it = pending.begin();
        (*it)->attach(procs[i]);
        assoc[i] = *it;
        busy[i] = true;
        pending.erase(it);
        running.insert(assoc[i]);
    }
    while (!pending.empty() || !running.empty()) {
        logger.print("wait");
        Watcher::getInstance().wait();
        for (i = 0; i < P; i++) {
            if (!busy[i])
                continue;
            if (!procs[i]->responsed()) {
                logger.print("process %p has no response", procs[i]);
                assoc[i]->detach();
                delete procs[i];
                procs[i] = new Subprocess(sorting.c_str());
                assoc[i]->attach(procs[i]);
            }
        }
    }
    logger.print("sorting done");
    std::for_each(tasks, tasks + N, [](Task *t) { t->reset(); });

    int topidx, heap_c = N, heap[NMAX];
    int32_t num[NMAX];
    auto cmp = [&num](int idxa, int idxb) { return num[idxa] > num[idxb]; };
    for (i = 0; i < N; i++) {
        num[i] = tasks[i]->getInteger();
        heap[i] = i;
    }
    std::make_heap(heap, heap + heap_c, cmp);
    while (--K) {
        //printf("K = %d, heap_c = %d, min num = %d\n", K, heap_c, num[heap[0]]);
        topidx = heap[0];
        std::pop_heap(heap, heap + heap_c, cmp);
        if (tasks[topidx]->eof()) {
            heap_c--;
            continue;
        }
        num[topidx] = tasks[topidx]->getInteger();
        std::push_heap(heap, heap + heap_c, cmp);
    }
    printf("%d\n", num[heap[0]]);
    return 0;
}

void Sched::action(Listener* obj, int e) { __logc;
    int idx;
    unordered_set<Task*>::iterator it;
    Task *t = dynamic_cast<Task*>(obj);
    logger.print("obj = %p, e = %x", obj, e);
    if (t == NULL)
        logger.raise("dynamic_cast<Task*>");
    if (e & E_FINISHED) {
        if ((it = running.find(t)) == running.end())
            logger.raise("running not found");
        idx = std::find(assoc, assoc + P, t) - assoc;
        if (idx == P)
            logger.raise("assoc not found");
        running.erase(it);
        t->detach();
        if (pending.empty()) {
            assoc[idx] = NULL;
            busy[idx] = false;
        } else {
            it = pending.begin();
            (*it)->attach(procs[idx]);
            assoc[idx] = *it;
            pending.erase(it);
            running.insert(assoc[idx]);
        }
    } else if (e & E_ERROR) {
        idx = std::find(assoc, assoc + P, t) - assoc;
        if (idx == P)
            throw "Sched assoc not found";
        t->detach();
        delete procs[idx];
        procs[idx] = new Subprocess(sorting.c_str());
        t->attach(procs[idx]);
    } else {
        throw "Sched received unknown message";
    }
}

void install_signal(int signo, void (*func)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = func;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signo, &sa, NULL) < 0) {
        perror("sigaction");
        throw "sigaction";
    }
}

void ignore_signal(int signo) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signo, &sa, NULL) < 0) {
        perror("sigaction");
        throw "sigaction";
    }
}

void useless_sigpipe(int signo) {
    assert(signo == SIGPIPE);
    write(STDERR_FILENO, "SIGPIPE\n", 8);
}

void sigalrm(int signo) {
    static int times = 15/ALRM_PERIOD;
    assert(signo == SIGALRM);
    if (--times == 0) {
        write(STDERR_FILENO, "SIGALRM\n", 8);
        times = 15/ALRM_PERIOD;
    }
    alarm(ALRM_PERIOD);
}

int main(int argc, char **argv) { __log;
    int res = -1;
    assert(15%ALRM_PERIOD == 0);
    //freopen("ex1.in", "rb", stdin);
    try {
        setbuf(stdout, NULL);
        setbuf(stderr, NULL);
        install_signal(SIGPIPE, useless_sigpipe);
        install_signal(SIGALRM, sigalrm);
        ignore_signal(SIGCHLD);
        alarm(ALRM_PERIOD);
        res = (new Sched(argc, argv))->run();
    } catch (const char *message) {
        puts(message);
    } catch (const string& e) {
        logger.eprint("%s", e.c_str());
    }
    return res;
}

