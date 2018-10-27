#include "EventPipe.h"
#include "Log.h"

#include <unistd.h>
#include <fcntl.h>

using namespace std;

EventPipe::EventPipe()
{
    event_mutex_ = make_shared<mutex>();

    if (pipe(pipes_) < 0) {
        Log(ERROR) << "Failed to create pipe, won't be able to wake threads," \
		" errno = " << errno << '\n';
    }

    if (fcntl(pipes_[0], F_SETFL, O_NONBLOCK) < 0) {
        Log(WARNING) << "Failed to set pipe non-blocking mode\n";
    }
}

EventPipe::~EventPipe()
{
	lock_guard<mutex> lock(*event_mutex_);

    if (pipes_[0] >= 0)
        close(pipes_[0]);

    if (pipes_[1] >= 0)
        close(pipes_[1]);
}

void EventPipe::set()
{
    lock_guard<mutex> lock(*event_mutex_);

    if (write(pipes_[1], "0", 1) < 0)
        Log(ERROR) << "Could not write to pipe, errno = " << errno << '\n';
}

void EventPipe::reset()
{
    lock_guard<mutex> lock(*event_mutex_);

    unsigned char buffer;

    while(read(pipes_[0], &buffer, 1) == 1)
        ;
}

int EventPipe::get_socket()
{
    lock_guard<mutex> lock(*event_mutex_);

    return pipes_[0];
}