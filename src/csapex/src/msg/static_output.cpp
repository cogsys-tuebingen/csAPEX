/// HEADER
#include <csapex/msg/static_output.h>

/// COMPONENT
#include <csapex/msg/message.h>
#include <csapex/msg/input.h>
#include <csapex/model/connection.h>
#include <csapex/utility/assert.h>
#include <csapex/msg/output_transition.h>
#include <csapex/msg/no_message.h>

/// SYSTEM
#include <iostream>

using namespace csapex;

StaticOutput::StaticOutput(const UUID &uuid)
    : Output(uuid), message_flags_(0)
{

}

void StaticOutput::addMessage(ConnectionType::ConstPtr message)
{
    setType(message->toType());

    // update buffer

    apex_assert_hard(message != nullptr);
    message_to_send_ = message;
}

bool StaticOutput::hasMessage()
{
    return (bool) message_to_send_;
}


void StaticOutput::nextMessage()
{
    setState(State::IDLE);
}

ConnectionTypeConstPtr StaticOutput::getMessage() const
{
    if(!committed_message_) {
        return connection_types::makeEmpty<connection_types::NoMessage>();
    } else {
        return committed_message_;
    }
}

void StaticOutput::setMultipart(bool multipart, bool last_part)
{
    message_flags_ = 0;
    if(multipart) {
        message_flags_ |= (int) ConnectionType::Flags::Fields::MULTI_PART;
        if(last_part) {
            message_flags_ |= (int) ConnectionType::Flags::Fields::LAST_PART;
        }
    }
}

void StaticOutput::commitMessages()
{
    assert(canSendMessages());

    activate();


    if(message_to_send_) {
        committed_message_ = message_to_send_;
        startReceiving();

    } else {
        if(!connections_.empty()) {
//            std::cout << getUUID() << " sends empty message" << std::endl;
        }
        committed_message_ = connection_types::makeEmpty<connection_types::NoMessage>();
    }

    ++seq_no_;
    committed_message_->setSequenceNumber(seq_no_);
    committed_message_->flags.data = message_flags_;

    ++count_;
    messageSent(this);
}

void StaticOutput::reset()
{
    Output::reset();
    committed_message_.reset();
}

void StaticOutput::disable()
{
    Output::disable();

    message_to_send_.reset();
    committed_message_.reset();
}

ConnectionType::ConstPtr StaticOutput::getMessage()
{
    return committed_message_;
}

void StaticOutput::startReceiving()
{
    message_to_send_.reset();
}
