#include <csapex/model/graph.h>
#include <csapex/model/node.h>
#include <csapex/model/node_facade_local.h>
#include <csapex/model/node_handle.h>
#include <csapex/factory/node_factory.h>
#include <csapex/factory/node_wrapper.hpp>
#include <csapex/core/settings/settings_local.h>
#include <csapex/utility/uuid_provider.h>
#include <csapex/model/node.h>
#include <csapex/msg/io.h>
#include <csapex/scheduling/thread_pool.h>
#include <csapex/model/graph_facade.h>
#include <csapex/msg/generic_value_message.hpp>
#include <csapex/model/connection.h>
#include <csapex/signal/event.h>
#include <csapex/signal/slot.h>
#include <csapex/msg/any_message.h>
#include <csapex/model/subgraph_node.h>

#include "gtest/gtest.h"
#include "test_exception_handler.h"

namespace csapex {

class MockupState
{
public:
    void setup(NodeModifier& node_modifier)
    {
        in = node_modifier.addInput<int>("input");
        out = node_modifier.addOutput<int>("output");

        e = node_modifier.addEvent("event");
        s = node_modifier.addTypedSlot<connection_types::AnyMessage>("slot", [this](const TokenPtr& token) {
            e->triggerWith(token);
        });
    }

    void setupParameters(Parameterizable& /*parameters*/)
    {

    }

    void process(NodeModifier& node_modifier, Parameterizable& /*parameters*/)
    {
        int val = msg::getValue<int>(in);
        msg::publish(out, val);

        msg::trigger(e);
    }

private:
    Input* in;
    Output* out;
    Event* e;
    Slot* s;
};

class MockupActivitySource : public Node
{
public:
    MockupActivitySource()
        : i(0), can_process_(false)
    {
    }

    void setup(NodeModifier& node_modifier) override
    {
        out = node_modifier.addOutput<int>("generator");
        e = node_modifier.addEvent("event");
    }

    void setupParameters(Parameterizable& /*parameters*/) override
    {

    }

    bool canProcess() const override
    {
        return can_process_;
    }

    void sendToken()
    {
        can_process_ = true;
    }
    void triggerEvent()
    {
        msg::trigger(e);
        can_process_ = true;
    }

    void process() override
    {
        can_process_ = false;

        msg::publish(out, i++);
    }

private:
    Output* out;
    Event* e;

    int i;

    bool can_process_;

};

class MockupActivitySink
{
public:
    MockupActivitySink()
        : waiting(false), value(-1)
    {

    }

    void setup(NodeModifier& node_modifier)
    {
        in = node_modifier.addInput<int>("sink");
        s = node_modifier.addTypedSlot<connection_types::AnyMessage>("slot", [this](const TokenConstPtr& token) {
            std::unique_lock<std::recursive_mutex> lock(wait_mutex);
            waiting = false;
            stepping_done.notify_all();
        });
    }

    void setupParameters(Parameterizable& /*parameters*/)
    {

    }

    void process(NodeModifier& node_modifier, Parameterizable& /*parameters*/)
    {
        int i = msg::getValue<int>(in);
        value = i;

        std::unique_lock<std::recursive_mutex> lock(wait_mutex);
        waiting = false;
        stepping_done.notify_all();
    }

    int getValue() const
    {
        return value;
    }

    void setWaiting(bool w)
    {
        waiting = w;
    }

    void wait()
    {
        std::unique_lock<std::recursive_mutex> lock(wait_mutex);
        while(waiting) {
            stepping_done.wait(lock);
        }
    }

private:
    Input* in;
    Slot* s;

    bool waiting;

    std::recursive_mutex wait_mutex;
    std::condition_variable_any stepping_done;

    int value;
};


class ActivityTest : public ::testing::Test {
protected:

    ActivityTest()
        : factory(SettingsLocal::NoSettings, nullptr),
          executor(eh, false, false)
    {
        factory.registerNodeType(std::make_shared<NodeConstructor>("ActivitySource", std::bind(&ActivityTest::makeSource)));
        factory.registerNodeType(std::make_shared<NodeConstructor>("ActivitySink", std::bind(&ActivityTest::makeSink)));

        factory.registerNodeType(std::make_shared<NodeConstructor>("ActivityState", std::bind(&ActivityTest::makeState)));
    }

    virtual ~ActivityTest() {
        // You can do clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    virtual void SetUp() override {
        graph = std::make_shared<SubgraphNode>();
        graph_facade = std::make_shared<GraphFacade>(executor, graph);

        executor.setSteppingMode(true);

        source_p = factory.makeNode("ActivitySource", UUIDProvider::makeUUID_without_parent("ActivitySource"), graph);
        graph_facade->addNode(source_p);

        state = factory.makeNode("ActivityState", UUIDProvider::makeUUID_without_parent("ActivityState"), graph);
        graph_facade->addNode(state);

        sink_p = factory.makeNode("ActivitySink", UUIDProvider::makeUUID_without_parent("ActivitySink"), graph);
        graph_facade->addNode(sink_p);

    }

    virtual void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    static NodePtr makeState() {
        return NodePtr(new NodeWrapper<MockupState>());
    }

    static NodePtr makeSource() {
        return NodePtr(new MockupActivitySource());
    }
    static NodePtr makeSink() {
        return NodePtr(new NodeWrapper<MockupActivitySink>());
    }


    NodeFactory factory;
    TestExceptionHandler eh;
    
    ThreadPool executor;

    SubgraphNodePtr graph;
    GraphFacade::Ptr graph_facade;

    NodeFacadePtr source_p;
    NodeFacadePtr state;
    NodeFacadePtr sink_p;

};


TEST_F(ActivityTest, ActivityIsConveyedViaMessages) {

    std::shared_ptr<MockupActivitySource> source = std::dynamic_pointer_cast<MockupActivitySource>(source_p->getNode());
    ASSERT_NE(nullptr, source);
    std::shared_ptr<MockupActivitySink> sink = std::dynamic_pointer_cast<MockupActivitySink>(sink_p->getNode());
    ASSERT_NE(nullptr, sink);

    ConnectionPtr c1 = graph_facade->connect(source_p->getNodeHandle().get(), "generator",
                                             state->getNodeHandle().get(), "input");
    ConnectionPtr c2 = graph_facade->connect(state->getNodeHandle().get(), "output",
                                             sink_p->getNodeHandle().get(), "sink");

    c1->setActive(true);
    c2->setActive(true);

    ASSERT_EQ(-1, sink->getValue());

    executor.start();

    for(int iter = 0; iter < 5; ++iter) {
        if(iter == 2) {
            // send activation token in this iteration
            source_p->setActive(true);
        }
        if(iter == 2) {
            ASSERT_TRUE(source_p->isActive());
        } else {
            ASSERT_TRUE(!source_p->isActive());
        }

        source->sendToken();

        sink->setWaiting(true);
        executor.step();
        sink->wait();

        ASSERT_EQ(iter, sink->getValue());

        if(iter >= 2) {
            // the sink is never deactivated
            ASSERT_TRUE(sink_p->isActive());

        } else {
            ASSERT_TRUE(!sink_p->isActive());
        }
    }
}

TEST_F(ActivityTest, ActivityIsConveyedViaMessagesOnlyForActiveConnections) {
    std::shared_ptr<MockupActivitySource> source = std::dynamic_pointer_cast<MockupActivitySource>(source_p->getNode());
    ASSERT_NE(nullptr, source);
    std::shared_ptr<MockupActivitySink> sink = std::dynamic_pointer_cast<MockupActivitySink>(sink_p->getNode());
    ASSERT_NE(nullptr, sink);

    ConnectionPtr c1 = graph_facade->connect(source_p->getNodeHandle().get(), "generator",
                                             state->getNodeHandle().get(), "input");
    graph_facade->connect(state->getNodeHandle().get(), "output",
                          sink_p->getNodeHandle().get(), "sink");

    c1->setActive(true);

    ASSERT_EQ(-1, sink->getValue());

    executor.start();

    for(int iter = 0; iter < 5; ++iter) {
        if(iter == 2) {
            // send activation token in this iteration
            source_p->setActive(true);
        }
        if(iter == 2) {
            ASSERT_TRUE(source_p->isActive());
        } else {
            ASSERT_TRUE(!source_p->isActive());
        }

        source->sendToken();

        sink->setWaiting(true);
        executor.step();
        sink->wait();

        ASSERT_EQ(iter, sink->getValue());

        if(iter >= 2) {
            // the intermediate state is never deactivated
            ASSERT_TRUE(state->isActive());

        } else {
            ASSERT_TRUE(!state->isActive());
        }

        ASSERT_TRUE(!sink_p->isActive());
    }
}


TEST_F(ActivityTest, ActivityIsConveyedViaEvents) {

    std::shared_ptr<MockupActivitySource> source = std::dynamic_pointer_cast<MockupActivitySource>(source_p->getNode());
    ASSERT_NE(nullptr, source);
    std::shared_ptr<MockupActivitySink> sink = std::dynamic_pointer_cast<MockupActivitySink>(sink_p->getNode());
    ASSERT_NE(nullptr, sink);

    ConnectionPtr c1 = graph_facade->connect(source_p->getNodeHandle().get(), "event",
                                             state->getNodeHandle().get(), "slot");
    ConnectionPtr c2 = graph_facade->connect(state->getNodeHandle().get(), "event",
                                             sink_p->getNodeHandle().get(), "slot");

    c1->setActive(true);
    c2->setActive(true);

    executor.start();

    for(int iter = 0; iter < 5; ++iter) {
        if(iter == 2) {
            // send activation token in this iteration
            source_p->setActive(true);
        }
        if(iter == 2) {
            ASSERT_TRUE(source_p->isActive());
        } else {
            ASSERT_TRUE(!source_p->isActive());
        }

        source->triggerEvent();

        sink->setWaiting(true);
        executor.step();
        sink->wait();

        ASSERT_TRUE(!state->isActive());

        if(iter >= 2) {
            // the sink is never deactivated
            ASSERT_TRUE(sink_p->isActive());

        } else {
            ASSERT_TRUE(!sink_p->isActive());
        }
    }
}

TEST_F(ActivityTest, ActivityIsConveyedViaEventsOnlyForActiveConnections) {

    std::shared_ptr<MockupActivitySource> source = std::dynamic_pointer_cast<MockupActivitySource>(source_p->getNode());
    ASSERT_NE(nullptr, source);
    std::shared_ptr<MockupActivitySink> sink = std::dynamic_pointer_cast<MockupActivitySink>(sink_p->getNode());
    ASSERT_NE(nullptr, sink);

    ConnectionPtr c1 = graph_facade->connect(source_p->getNodeHandle().get(), "event",
                                             state->getNodeHandle().get(), "slot");
    graph_facade->connect(state->getNodeHandle().get(), "event",
                          sink_p->getNodeHandle().get(), "slot");

    c1->setActive(true);

    executor.start();

    for(int iter = 0; iter < 5; ++iter) {
        if(iter == 2) {
            // send activation token in this iteration
            source_p->setActive(true);
        }
        if(iter == 2) {
            ASSERT_TRUE(source_p->isActive());
        } else {
            ASSERT_TRUE(!source_p->isActive());
        }

        source->triggerEvent();

        sink->setWaiting(true);
        executor.step();
        sink->wait();

        if(iter >= 2) {
            // the sink is never deactivated
            ASSERT_TRUE(state->isActive());

        } else {
            ASSERT_TRUE(!state->isActive());
        }

        ASSERT_TRUE(!sink_p->isActive());
    }
}

TEST_F(ActivityTest, ActivityIsConveyedViaEventsAndMessages) {

    std::shared_ptr<MockupActivitySource> source = std::dynamic_pointer_cast<MockupActivitySource>(source_p->getNode());
    ASSERT_NE(nullptr, source);
    std::shared_ptr<MockupActivitySink> sink = std::dynamic_pointer_cast<MockupActivitySink>(sink_p->getNode());
    ASSERT_NE(nullptr, sink);

    ConnectionPtr c1 = graph_facade->connect(source_p->getNodeHandle().get(), "generator",
                                             state->getNodeHandle().get(), "input");
    ConnectionPtr c2 = graph_facade->connect(state->getNodeHandle().get(), "event",
                                             sink_p->getNodeHandle().get(), "slot");

    c1->setActive(true);
    c2->setActive(true);

    executor.start();

    for(int iter = 0; iter < 5; ++iter) {
        if(iter == 2) {
            // send activation token in this iteration
            source_p->setActive(true);
        }
        if(iter == 2) {
            ASSERT_TRUE(source_p->isActive());
        } else {
            ASSERT_TRUE(!source_p->isActive());
        }

        source->sendToken();

        sink->setWaiting(true);
        executor.step();
        sink->wait();

        ASSERT_TRUE(!state->isActive());

        if(iter >= 2) {
            // the sink is never deactivated
            ASSERT_TRUE(sink_p->isActive());

        } else {
            ASSERT_TRUE(!sink_p->isActive());
        }
    }
}

TEST_F(ActivityTest, ActivityTriggersEvents) {

    std::shared_ptr<MockupActivitySource> source = std::dynamic_pointer_cast<MockupActivitySource>(source_p->getNode());
    ASSERT_NE(nullptr, source);
    std::shared_ptr<MockupActivitySink> sink = std::dynamic_pointer_cast<MockupActivitySink>(sink_p->getNode());
    ASSERT_NE(nullptr, sink);

    ConnectionPtr c1 = graph_facade->connect(source_p->getNodeHandle().get(), "generator",
                                             state->getNodeHandle().get(), "input");
    ConnectionPtr c2 = graph_facade->connect(state->getNodeHandle().get(), "event",
                                             sink_p->getNodeHandle().get(), "slot");

    c1->setActive(true);
    c2->setActive(true);

    executor.start();

    for(int iter = 0; iter < 5; ++iter) {
        if(iter == 2) {
            // send activation token in this iteration
            source_p->setActive(true);
        }
        if(iter == 2) {
            ASSERT_TRUE(source_p->isActive());
        } else {
            ASSERT_TRUE(!source_p->isActive());
        }

        source->sendToken();

        sink->setWaiting(true);
        executor.step();
        sink->wait();

        ASSERT_TRUE(!state->isActive());

        if(iter >= 2) {
            // the sink is never deactivated
            ASSERT_TRUE(sink_p->isActive());

        } else {
            ASSERT_TRUE(!sink_p->isActive());
        }
    }
}


}