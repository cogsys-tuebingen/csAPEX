#ifndef MODEL_FWD_H
#define MODEL_FWD_H

/// shared_ptr
#include <memory>

#define FWD(name) \
    class name;\
    typedef std::shared_ptr<name> name##Ptr;\
    typedef std::unique_ptr<name> name##UniquePtr;\
    typedef std::weak_ptr<name> name##WeakPtr;\
    typedef std::shared_ptr<const name> name##ConstPtr;


namespace csapex
{
namespace graph
{
FWD(Vertex)
FWD(Edge)
}

FWD(NodeFacade)
FWD(NodeFacadeImplementation)
FWD(NodeFacadeProxy)
FWD(NodeHandle)
FWD(NodeConstructor)
FWD(NodeStatistics)
FWD(NodeRunner)
FWD(Connectable)
FWD(ConnectableOwner)
FWD(Memento)
FWD(Graph)
FWD(GraphImplementation)
FWD(SubgraphNode)
FWD(GraphFacade)
FWD(GraphFacadeImplementation)
FWD(TokenData)
FWD(Token)
FWD(GenericState)
FWD(Node)
FWD(NodeState)
FWD(NodeWorker)
FWD(NodeModifier)
FWD(Tag)
FWD(NodeCharacteristics)
FWD(ConnectorDescription)
FWD(Connector)

FWD(Connection)
FWD(ConnectionDescription)
FWD(Fulcrum)
}

#undef FWD

#endif // MODEL_FWD_H
