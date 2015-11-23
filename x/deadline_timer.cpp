
#include "deadline_timer.h"
#include "io_service.h"

namespace x
{

deadline_timer::deadline_timer(io_service& io_service)
    : io_service_(io_service)
{
}

deadline_timer::~deadline_timer()
{
    cancel();
}

unsigned int deadline_timer::cancel()
{
    return io_service_.cancel_timer(*this);
}

}