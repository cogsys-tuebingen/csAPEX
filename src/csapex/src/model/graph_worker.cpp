/// HEADER
#include <csapex/model/graph_worker.h>

/// COMPONENT
#include <csapex/model/graph.h>
#include <csapex/model/node.h>
#include <csapex/model/node_worker.h>
#include <csapex/model/connection.h>
#include <csapex/core/settings.h>

/// SYSTEM
#include <QTimer>

using namespace csapex;

GraphWorker::GraphWorker(Graph* graph)
    : graph_(graph), timer_(new QTimer)
{
    timer_->setInterval(1000. / 30.);
    timer_->start();

    QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(tick()));
}

Graph* GraphWorker::getGraph()
{
    return graph_;
}

void GraphWorker::tick()
{
    //    Q_FOREACH(Node::Ptr n, nodes_) {
    //        if(n->isEnabled()) {
    //            n->tick();
    //        }
    //    }

    Q_FOREACH(Node::Ptr node, graph_->nodes_) {
        node->getNodeWorker()->checkParameters();
    }

    Q_FOREACH(const Connection::Ptr& connection, graph_->connections_) {
        connection->tick();
    }
}

void GraphWorker::reset()
{
    stop();

    graph_->settings_.setProcessingAllowed(true);

    graph_->uuids_.clear();
    graph_->connections_.clear();
}

bool GraphWorker::isPaused() const
{
    return !timer_->isActive();
}

void GraphWorker::setPause(bool pause)
{
    if(pause == isPaused()) {
        return;
    }

    Q_FOREACH(Node::Ptr node, graph_->nodes_) {
        node->getNodeWorker()->pause(pause);
    }
    if(pause) {
        timer_->stop();
    } else {
        timer_->start();
    }

    Q_EMIT paused(pause);
}


void GraphWorker::stop()
{
    Q_FOREACH(Node::Ptr node, graph_->nodes_) {
        node->getNodeWorker()->setEnabled(false);
    }
    Q_FOREACH(Node::Ptr node, graph_->nodes_) {
        node->getNodeWorker()->stop();
    }

    graph_->nodes_.clear();
}