#ifndef TIMER_H
#define TIMER_H

/// PROJECT
#include <csapex/utility/slim_signal.hpp>
#include <csapex/profiling/interval.h>
#include <csapex/profiling/interlude.h>

/// SYSTEM
#include <chrono>
#include <memory>
#include <vector>
#include <deque>
#include <map>

namespace csapex
{

class Timer
{
public:
    typedef std::shared_ptr<Timer> Ptr;

public:
    slim_signal::Signal<void(Interval::Ptr)> finished;

public:
    Timer(const std::string &name);
    ~Timer();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void restart();
    void finish();
    std::vector<std::pair<std::string, double> > entries() const;

    Interlude::Ptr step(const std::string& name);

    long startTimeMs() const;
    long stopTimeMs() const;

public:
    std::string timer_name_;

    Interval::Ptr root;
    std::deque<Interval::Ptr> active;

    bool enabled_;
};

}

#endif // TIMER_H