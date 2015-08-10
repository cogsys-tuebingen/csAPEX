#ifndef OUTPUT_TRANSITION_H
#define OUTPUT_TRANSITION_H

/// COMPONENT
#include <csapex/msg/transition.h>

namespace csapex
{
class OutputTransition : public Transition
{
public:
    OutputTransition(NodeWorker* node);

    void connectionAdded(Connection* connection);
    void connectionRemoved(Connection *connection);

    bool isSink() const;
    bool canStartSendingMessages() const;
    void sendMessages();
    void publishNextMessage();

    void clearOutputs();
    void abortSendingMessages();
    void setConnectionsReadyToReceive();
    bool areOutputsIdle() const;

    virtual void reset() override;

    void establishConnections();

private:

    void fillConnections();
};
}

#endif // OUTPUT_TRANSITION_H
