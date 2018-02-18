#ifndef SUBPROCESS_H
#define SUBPROCESS_H

/// COMPONENT
#include <csapex/utility/subprocess_channel.h>
#include <csapex/utility/function_traits.hpp>

/// SYSTEM
#include <iostream>
#include <csignal>
#include <thread>
#include <sstream>

namespace csapex
{

class Subprocess
{
public:
    Subprocess(const std::string& name_space);
    ~Subprocess();

    void handleSignal(int signal);
    pid_t fork(std::function<int()> child);

    template <typename Callable>
    typename std::enable_if<std::is_arithmetic< typename csapex::function_traits<Callable>::result_type >::value, pid_t >::type
    fork(Callable child)
    {
        std::function<int()> wrapper = [child]() -> int {
            return child();
        };
        return fork(wrapper);
    }

    template <typename Callable>
    typename std::enable_if<!std::is_arithmetic< typename csapex::function_traits<Callable>::result_type >::value, pid_t >::type
    fork(Callable child)
    {
        std::function<int()> wrapper = [child]() -> int {
            child();
            return 0;
        };
        return fork(wrapper);
    }

    std::string getChildStdOut() const;
    std::string getChildStdErr() const;

    bool isActive() const;

    bool isParent() const;
    bool isChild() const;

    int join();

public:
    SubprocessChannel in;
    SubprocessChannel out;

private:
    void readCtrlOut();

private:
    SubprocessChannel ctrl_in;
    SubprocessChannel ctrl_out;

private:
    pid_t pid_;
    bool active_;

    std::thread subprocess_worker_;
    std::thread parent_worker_;

    std::stringstream child_cout;
    std::stringstream child_cerr;

    int pipe_in[2];
    int pipe_out[2];
    int pipe_err[2];

    bool is_shutdown;
    int return_code;
};


}

#endif // SUBPROCESS_H
