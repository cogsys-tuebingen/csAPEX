/// HEADER
#include <csapex/command/add_node.h>

/// COMPONENT
#include <csapex/command/command.h>
#include <csapex/model/node_constructor.h>
#include <csapex/model/node_state.h>
#include <csapex/model/node_factory.h>
#include <csapex/model/node_worker.h>
#include <csapex/model/graph.h>
#include <csapex/model/node.h>
#include <csapex/utility/assert.h>
#include <csapex/view/widget_controller.h>

using namespace csapex::command;

AddNode::AddNode(const std::string &type, QPoint pos, const UUID &parent_uuid, const UUID& uuid, NodeState::Ptr state)
    : type_(type), pos_(pos), parent_uuid_(parent_uuid), uuid_(uuid)
{
    apex_assert_hard(!uuid.empty());

    if(state != MementoNullPtr) {
        NodeState::Ptr bs = boost::dynamic_pointer_cast<NodeState> (state);
        saved_state_ = bs;
    }
}

std::string AddNode::getType() const
{
    return "AddNode";
}

std::string AddNode::getDescription() const
{
    return std::string("added a node of type ") + type_ + " and UUID " + uuid_.getFullName();
}


bool AddNode::doExecute()
{
    if(uuid_.empty()) {
        uuid_ = UUID::make(graph_->makeUUIDPrefix(type_));
    }

    NodeWorker::Ptr node = widget_ctrl_->getNodeFactory()->makeNode(type_, uuid_, saved_state_);

    node->getNode()->getNodeState()->setPos(pos_);

    graph_->addNode(node);

    return true;
}

bool AddNode::doUndo()
{
    Node* node_ = graph_->findNode(uuid_);

    saved_state_ = node_->getNodeStateCopy();


    if(parent_uuid_.empty()) {
        graph_->deleteNode(node_->getUUID());
    }

    return true;
}

bool AddNode::doRedo()
{
    if(doExecute()) {
        graph_->findNode(uuid_)->setNodeState(saved_state_);
        return true;
    }

    return false;
}
