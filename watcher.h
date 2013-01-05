#ifndef _WATCHER_H_
#define _WATCHER_H_

#include <unordered_map>

#define E_READ 1
#define E_WRITE 2
#define E_HANGUP 4
#define E_ERROR 8

class Listener {
    public:
        virtual void action(Listener*, int) = 0;
        virtual ~Listener();
};

class Watcher {
	private:
		int epollfd;
        std::unordered_map<int, Listener*> watching;
	public:
		static Watcher& getInstance();

        void watch(int, Listener*);
        void unwatch(int);
        void wait();
		Watcher();
		~Watcher();

		Watcher(const Watcher&);
		const Watcher& operator=(const Watcher&);
};

#endif

