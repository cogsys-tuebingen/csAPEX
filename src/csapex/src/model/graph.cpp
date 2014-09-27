/// HEADER
#include <csapex/model/graph.h>

/// PROJECT
#include <csapex/command/add_connection.h>
#include <csapex/command/add_connector.h>
#include <csapex/command/add_node.h>
#include <csapex/command/delete_connection.h>
#include <csapex/command/delete_fulcrum.h>
#include <csapex/command/delete_node.h>
#include <csapex/command/meta.h>
#include <csapex/model/connectable.h>
#include <csapex/msg/input.h>
#include <csapex/msg/output.h>
#include <csapex/model/node.h>
#include <csapex/model/node_worker.h>
#include <csapex/utility/timer.h>

/// SYSTEM
#include <boost/foreach.hpp>

using namespace csapex;

Graph::Graph()
{
}

Graph::~Graph()
{

}

std::string Graph::makeUUIDPrefix(const std::string& name)
{
    int& last_id = uuids_[name];
    ++last_id;

    std::stringstream ss;
    ss << name << "_" << last_id;

    return ss.str();
}

void Graph::addNode(NodeWorker::Ptr node_worker)
{
    nodes_.push_back(node_worker);
    node_parents_[node_worker.get()] = std::vector<NodeWorker*>();
    node_children_[node_worker.get()] = std::vector<NodeWorker*>();

    node_worker->makeThread();

    buildConnectedComponents();

    Q_EMIT nodeAdded(node_worker);
}

void Graph::deleteNode(const UUID& uuid)
{
    NodeWorker* node_worker = findNodeWorker(uuid);
    Node* node = node_worker->getNode();

    node_worker->stop();

    /// assert that all connections have already been deleted
    apex_assert_hard(node_parents_[node_worker].empty());
    apex_assert_hard(node_children_[node_worker].empty());

    node_parents_.erase(node_worker);
    node_children_.erase(node_worker);

    NodeWorker::Ptr removed;

    for(std::vector<NodeWorker::Ptr>::iterator it = nodes_.begin(); it != nodes_.end();) {
        if((*it)->getNodeUUID() == node->getUUID()) {
            removed = *it;
            it = nodes_.erase(it);

        } else {
            ++it;
        }
    }

    if(removed) {
        Q_EMIT nodeRemoved(removed);
        buildConnectedComponents();
    }
}

int Graph::countNodes()
{
    return nodes_.size();
}



void Graph::foreachNode(boost::function<void (Node*)> f)
{
    Q_FOREACH(NodeWorker::Ptr b, nodes_) {
        f(b->getNode());
    }
}

void Graph::foreachNode(boost::function<void (Node*)> f, boost::function<bool (Node*)> pred)
{
    Q_FOREACH(NodeWorker::Ptr b, nodes_) {
        if(pred(b->getNode())) {
            f(b->getNode());
        }
    }
}

bool Graph::addConnection(Connection::Ptr connection)
{
    if(connection->from()->tryConnect(connection->to())) {
        Connectable* from = findConnector(connection->from()->getUUID());
        Connectable* to = findConnector(connection->to()->getUUID());

        connections_.push_back(connection);

        NodeWorker* n_from = findNodeWorkerForConnector(connection->from()->getUUID());
        NodeWorker* n_to = findNodeWorkerForConnector(connection->to()->getUUID());

        node_parents_[n_to].push_back(n_from);
        node_children_[n_from].push_back(n_to);

        if(node_component_[n_from] == node_component_[n_to]) {
            // if both nodes are already in the same component
            // we need to have the same seq no

            int highest_seq_no = -1;
            // search all parents of the target for the highest seq no
            Q_FOREACH(Input* input, n_to->getMessageInputs()) {
                if(!input->isConnected()) {
                    continue;
                }
                NodeWorker* ni = findNodeWorkerForConnector(input->getSource()->getUUID());

                Q_FOREACH(Output* output, ni->getMessageOutputs()) {
                    if(output->sequenceNumber() > highest_seq_no) {
                        highest_seq_no = output->sequenceNumber();
                    }
                }
            }
            if(highest_seq_no != -1) {
//                std::cerr << "setting the sequence numbers:\n";
                Q_FOREACH(Input* input, n_to->getMessageInputs()) {
                    input->setSequenceNumber(highest_seq_no);
                }
            }

        } else {
            // if both nodes are in different components we need to synchronize the two components
            // this connection is the only connection between the two components.
            // set the sequence no of the child component to the one given by this connector
            int seq_no = from->sequenceNumber();

//            std::cerr << "synchronize components" << std::endl;
            Q_FOREACH(NodeWorker::Ptr n, nodes_) {
                if(node_component_[n.get()] == node_component_[n_to]) {
                    Q_FOREACH(Output* output, n->getMessageOutputs()) {
                        output->setSequenceNumber(seq_no);
                    }
                    Q_FOREACH(Input* input, n->getMessageInputs()) {
                        input->setSequenceNumber(seq_no);
                    }
                }
            }
        }

        buildConnectedComponents();
        verify();



        Q_EMIT connectionAdded(connection.get());
        Q_EMIT from->connectionDone();
        Q_EMIT to->connectionDone();
        return true;
    }

    std::cerr << "cannot connect " << connection->from()->getUUID() << " (" <<( connection->from()->isInput() ? "i": "o" )<< ") to " << connection->to()->getUUID() << " (" <<( connection->to()->isInput() ? "i": "o" )<< ")"  << std::endl;
    return false;
}

void Graph::deleteConnection(Connection::Ptr connection)
{
    connection->from()->removeConnection(connection->to());

    for(std::vector<Connection::Ptr>::iterator c = connections_.begin(); c != connections_.end();) {
        if(*connection == **c) {
            Connectable* to = connection->to();
            to->setError(false);

            NodeWorker* n_from = findNodeWorkerForConnector(connection->from()->getUUID());
            NodeWorker* n_to = findNodeWorkerForConnector(connection->to()->getUUID());

            // erase pointer from TO to FROM
            // if there are multiple edges, this only erases one entry
            node_parents_[n_to].erase(std::find(node_parents_[n_to].begin(), node_parents_[n_to].end(), n_from));

            // erase pointer from FROM to TO
            node_children_[n_from].erase(std::find(node_children_[n_from].begin(), node_children_[n_from].end(), n_to));

            connections_.erase(c);

            buildConnectedComponents();
            verify();
            Q_EMIT connectionDeleted(connection.get());
        } else {
            ++c;
        }
    }

    Q_EMIT stateChanged();

}


void Graph::buildConnectedComponents()
{
    /* Find all connected sub components of this graph */
    //    std::map<Node*, int> old_node_component = node_component_;
    node_component_.clear();

    std::deque<NodeWorker*> unmarked;
    Q_FOREACH(NodeWorker::Ptr node, nodes_) {
        unmarked.push_back(node.get());
        node_component_[node.get()] = -1;
    }

    std::deque<NodeWorker*> Q;
    int component = 0;
    while(!unmarked.empty()) {
        // take a random unmarked node to start bfs from
        NodeWorker* start = unmarked.front();
        Q.push_back(start);

        node_component_[start] = component;

        while(!Q.empty()) {
            NodeWorker* front = Q.front();
            Q.pop_front();

            unmarked.erase(std::find(unmarked.begin(), unmarked.end(), front));

            // iterate all neighbors
            std::vector<NodeWorker*> neighbors;
            Q_FOREACH(NodeWorker* parent, node_parents_[front]) {
                neighbors.push_back(parent);
            }
            Q_FOREACH(NodeWorker* child, node_children_[front]) {
                neighbors.push_back(child);
            }

            Q_FOREACH(NodeWorker* neighbor, neighbors) {
                if(node_component_[neighbor] == -1) {
                    node_component_[neighbor] = component;
                    Q.push_back(neighbor);
                }
            }
        }

        ++component;
    }

    Q_EMIT structureChanged(this);
}

void Graph::verify()
{
    verifyAsync();
}

void Graph::verifyAsync()
{
    /* Foreach node look for paths to every other node.
     * If there are two or more paths from one node to another
     *   and on of them contains an async edge, make all others
     *   temporary async
     */

    Q_FOREACH(NodeWorker::Ptr node_worker, nodes_) {
        Q_FOREACH(Input* input, node_worker->getMessageInputs()) {
            input->setTempAsync(false);
        }
    }

    Q_FOREACH(NodeWorker::Ptr node_worker, nodes_) {
        std::deque<NodeWorker*> Q;
        std::map<NodeWorker*,bool> has_async_input;

        NodeWorker* current = node_worker.get();

        Q.push_back(current);

        while(!Q.empty()) {
            NodeWorker* front = Q.front();
            Q.pop_front();

            bool visited = has_async_input.find(front) != has_async_input.end();
            if(!visited) {
                has_async_input[front] = false;
            }

            Q_FOREACH(Output* output, front->getMessageOutputs()) {
                for(typename Output::TargetIterator in = output->beginTargets(); in != output->endTargets(); ++in) {
                    Input* input = *in;

                    NodeWorker* next_node = findNodeWorkerForConnector(input->getUUID());

                    if(input->isAsync() || has_async_input[front]) {
                        has_async_input[next_node] = true;
                    }

                    Q.push_back(next_node);
                }
            }
        }


        Q_FOREACH(Output* output, node_worker->getMessageOutputs()) {
            for(Output::TargetIterator in = output->beginTargets(); in != output->endTargets(); ++in) {
                Input* input = *in;

                NodeWorker* next_node = findNodeWorkerForConnector(input->getUUID());

                if(!input->isAsync()) {
                    bool a = has_async_input[next_node];
                    input->setTempAsync(a);
                }
            }
        }
    }
}

Command::Ptr Graph::clear()
{
    command::Meta::Ptr clear(new command::Meta("Clear Graph"));

    Q_FOREACH(NodeWorker::Ptr node, nodes_) {
        Command::Ptr cmd(new command::DeleteNode(node->getNodeUUID()));
        clear->add(cmd);
    }

    return clear;
}

int Graph::getComponent(const UUID &node_uuid) const
{
    NodeWorker* node = findNodeWorkerNoThrow(node_uuid);
    if(!node) {
        return -1;
    }

    return node_component_.at(node);
}

Node* Graph::findNode(const UUID& uuid) const
{
    Node* node = findNodeNoThrow(uuid);
    if(node) {
        return node;
    }
    throw NodeNotFoundException(uuid.getFullName());
}

NodeWorker* Graph::findNodeWorker(const UUID& uuid) const
{
    NodeWorker* node_worker = findNodeWorkerNoThrow(uuid);
    if(node_worker) {
        return node_worker;
    }
    throw NodeWorkerNotFoundException(uuid.getFullName());
}

Node* Graph::findNodeNoThrow(const UUID& uuid) const
{
    Q_FOREACH(NodeWorker::Ptr b, nodes_) {
        if(b->getNodeUUID() == uuid) {
            return b->getNode();
        }
    }

    return NULL;
}


NodeWorker* Graph::findNodeWorkerNoThrow(const UUID& uuid) const
{
    Q_FOREACH(const NodeWorker::Ptr b, nodes_) {
        if(b->getNodeUUID() == uuid) {
            return b.get();
        }
    }

    return NULL;
}

Node* Graph::findNodeForConnector(const UUID &uuid) const
{
    UUID l = UUID::NONE;
    UUID r = UUID::NONE;
    uuid.split(UUID::namespace_separator, l, r);

    try {
        return findNode(l);

    } catch(const std::exception& e) {
        throw std::runtime_error(std::string("cannot find node of connector \"") + uuid.getFullName());
    }
}

NodeWorker* Graph::findNodeWorkerForConnector(const UUID &uuid) const
{
    UUID l = UUID::NONE;
    UUID r = UUID::NONE;
    uuid.split(UUID::namespace_separator, l, r);

    try {
        return findNodeWorker(l);

    } catch(const std::exception& e) {
        throw std::runtime_error(std::string("cannot find worker of connector \"") + uuid.getFullName());
    }
}

std::vector<NodeWorker*> Graph::getAllNodeWorkers()
{
    std::vector<NodeWorker*> node_workers;
    foreach(const NodeWorkerPtr& node, nodes_) {
        node_workers.push_back(node.get());
    }

    return node_workers;
}

Connectable* Graph::findConnector(const UUID &uuid)
{
    UUID l = UUID::NONE;
    UUID r = UUID::NONE;
    uuid.split(UUID::namespace_separator, l, r);

    NodeWorker* owner = findNodeWorker(l);
    apex_assert_hard(owner);

    Connectable* result = owner->getInput(uuid);
    if(result == NULL) {
        result = owner->getOutput(uuid);
    }

    apex_assert_hard(result);

    return result;
}

Connection::Ptr Graph::getConnectionWithId(int id)
{
    BOOST_FOREACH(Connection::Ptr& connection, connections_) {
        if(connection->id() == id) {
            return connection;
        }
    }

    return ConnectionNullPtr;
}

Connection::Ptr Graph::getConnection(Connection::Ptr c)
{
    BOOST_FOREACH(Connection::Ptr& connection, connections_) {
        if(*connection == *c) {
            return connection;
        }
    }

    std::cerr << "error: cannot get connection for " << *c << std::endl;

    return ConnectionNullPtr;
}

int Graph::getConnectionId(Connection::Ptr c)
{
    Connection::Ptr internal = getConnection(c);

    if(internal != ConnectionNullPtr) {
        return internal->id();
    }

    return -1;
}
Command::Ptr Graph::deleteConnectionByIdCommand(int id)
{
    Q_FOREACH(const Connection::Ptr& connection, connections_) {
        if(connection->id() == id) {
            return Command::Ptr(new command::DeleteConnection(connection->from(), connection->to()));
        }
    }

    return Command::Ptr();
}

Command::Ptr Graph::deleteConnectionFulcrumCommand(int connection, int fulcrum)
{
    return Command::Ptr(new command::DeleteFulcrum(connection, fulcrum));
}

Command::Ptr Graph::deleteAllConnectionFulcrumsCommand(int connection)
{
    command::Meta::Ptr meta(new command::Meta("Delete All Connection Fulcrums"));

    if(connection >= 0) {
        int n = getConnectionWithId(connection)->getFulcrumCount();
        for(int i = n - 1; i >= 0; --i) {
            meta->add(deleteConnectionFulcrumCommand(connection, i));
        }
    }

    return meta;
}

Command::Ptr Graph::deleteAllConnectionFulcrumsCommand(Connection::Ptr connection)
{
    return deleteAllConnectionFulcrumsCommand(getConnectionId(connection));
}


Command::Ptr Graph::deleteConnectionById(int id)
{
    Command::Ptr cmd(deleteConnectionByIdCommand(id));

    return cmd;
}
