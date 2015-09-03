#ifndef LINK_H
#define LINK_H

#include <string>
#include <vector>

class Link{

private:

    int _source_id;
    int _dest_id;
    bool _enabled;
    double _bw;

public:

    explicit Link(int source_id, int dest_id, bool enabled = true, double bw = 0) :
                _source_id(source_id),
                _dest_id(dest_id),
                _enabled(enabled),
                _bw(bw)
            {
            }

    explicit Link(){}

    int source_id() const;
    void setSource_id(int source_id);
    int dest_id() const;
    void setDest_id(int dest_id);
    bool enabled() const;
    void setEnabled(bool enabled);
    double bw() const;
    void setBw(double bw);
};

#endif // LINK_H
