#ifndef NODE_FACADE_REMOTE_H
#define NODE_FACADE_REMOTE_H

/// PROJECT
#include <csapex/model/node_facade.h>
#include <csapex/io/io_fwd.h>
#include <csapex/serialization/streamable.h>
#include <csapex/io/remote.h>

/// SYSTEM
#include <unordered_map>

namespace csapex
{

class CSAPEX_EXPORT NodeFacadeRemote : public NodeFacade, public Remote
{
public:
    NodeFacadeRemote(SessionPtr session, AUUID uuid,
                     NodeHandlePtr nh, NodeWorker *nw);

    ~NodeFacadeRemote();

    UUID getUUID() const override;
    AUUID getAUUID() const override;

    void setProfiling(bool profiling) override;

    bool isParameterInput(const UUID& id) override;
    bool isParameterOutput(const UUID& id) override;


    ConnectorPtr getConnector(const UUID& id) const override;
    ConnectorPtr getConnectorNoThrow(const UUID& id) const noexcept override;

    ConnectorPtr getParameterInput(const std::string& name) const override;
    ConnectorPtr getParameterOutput(const std::string& name) const override;


    // Parameterizable
    virtual std::vector<param::ParameterPtr> getParameters() const override;
    param::ParameterPtr getParameter(const std::string& name) const override;
    bool hasParameter(const std::string& name) const override;

    // Debug Access
    std::string getLoggerOutput(ErrorState::ErrorLevel level) const override;

    // TODO: proxies
    ProfilerPtr getProfiler() override;

    NodeStatePtr getNodeState() const override;
    NodeStatePtr getNodeStateCopy() const override;

    GenericStateConstPtr getParameterState() const override;

    /**
     * begin: generate getters
     **/
    #define HANDLE_ACCESSOR(_enum, type, function) \
    virtual type function() const override;

    #define HANDLE_STATIC_ACCESSOR(_enum, type, function) HANDLE_ACCESSOR(_enum, type, function)
    #define HANDLE_DYNAMIC_ACCESSOR(_enum, signal, type, function) HANDLE_ACCESSOR(_enum, type, function)
    #define HANDLE_SIGNAL(_enum, signal)

    #include <csapex/model/node_facade_remote_accessors.hpp>
    /**
     * end: generate getters
     **/


public:
    slim_signal::ObservableSignal<void(StreamableConstPtr)> remote_data_connection;

private:
    void handleBroadcast(const BroadcastMessageConstPtr& message) override;

    void connectNodeHandle();
    void connectNodeWorker();

    void createConnectorProxy(const UUID &uuid);

private:
    AUUID uuid_;

    io::ChannelPtr node_channel_;

    /**
     * begin: generate caches
     **/
    #define HANDLE_ACCESSOR(_enum, type, function)
    #define HANDLE_STATIC_ACCESSOR(_enum, type, function) \
    mutable bool has_##function##_; \
    mutable type cache_##function##_;
    #define HANDLE_DYNAMIC_ACCESSOR(_enum, signal, type, function) \
    mutable bool has_##function##_; \
    mutable type value_##function##_;
    #define HANDLE_SIGNAL(_enum, signal)

    #include <csapex/model/node_facade_remote_accessors.hpp>
    /**
     * end: generate caches
     **/


    NodeHandlePtr nh_;
    NodeWorker* nw_;

    std::unordered_map<UUID, ConnectorPtr, UUID::Hasher> remote_connectors_;
};

}

#endif // NODE_FACADE_REMOTE_H
