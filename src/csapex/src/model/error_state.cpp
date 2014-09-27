/// HEADER
#include <csapex/model/error_state.h>

/// SYSTEM
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

using namespace csapex;

ErrorState::ErrorState()
    : error_mutex_(new boost::mutex), error_(false)
{
}

ErrorState::~ErrorState()
{

}

void ErrorState::setError(bool e, const std::string& msg, ErrorLevel level)
{
    if(!isError() && !e) {
        return;
    }

    setErrorSilent(e, msg, level);

    errorEvent(error_, msg, level_);
}

void ErrorState::setErrorSilent(bool e, const std::string &msg, ErrorLevel level)
{
    {
        boost::lock_guard<boost::mutex> lock (*error_mutex_);

        if(!error_ && !e) {
            return;
        }

        std::string err;
        if(e) {
            unsigned line = 60;
            for(unsigned i = 0; i < msg.size(); ++i) {
                err += msg[i];
                if((i%line) == 0 && i != 0) {
                    err += '\n';
                }
            }
        }
        error_ = e;
        level_ = level;
        error_msg_ = msg;
    }

    errorChanged(error_);
}

void ErrorState::errorChanged(bool)
{

}

bool ErrorState::isError() const
{
    boost::lock_guard<boost::mutex> lock (*error_mutex_);
    return error_;
}

ErrorState::ErrorLevel ErrorState::errorLevel() const
{
    boost::lock_guard<boost::mutex> lock (*error_mutex_);
    return level_;
}

std::string ErrorState::errorMessage() const
{
    boost::lock_guard<boost::mutex> lock (*error_mutex_);
    return error_msg_;
}

void ErrorState::errorEvent(bool, const std::string &, ErrorLevel)
{

}
