#include "Link.h"

using namespace std;


int Link::dest_id() const
{
    return _dest_id;
}

void Link::setDest_id(int dest_id)
{
    _dest_id = dest_id;
}

bool Link::enabled() const
{
    return _enabled;
}

void Link::setEnabled(bool enabled)
{
    _enabled = enabled;
}

double Link::bw() const
{
    return _bw;
}

void Link::setBw(double bw)
{
    _bw = bw;
}
int Link::source_id() const
{
    return _source_id;
}

void Link::setSource_id(int source_id)
{
    _source_id = source_id;
}


