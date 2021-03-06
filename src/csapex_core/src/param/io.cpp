/// HEADER
#include <csapex/param/io.h>

/// COMPONENT
#include <csapex/param/parameter_factory.h>

using namespace csapex;
using namespace param;
using namespace YAML;

Node YAML::convert<csapex::param::ParameterPtr>::encode(const csapex::param::ParameterPtr& rhs)
{
    YAML::Node n;
    rhs->serialize_yaml(n);
    return n;
}

bool YAML::convert<csapex::param::ParameterPtr>::decode(const Node& node, csapex::param::ParameterPtr& rhs)
{
    std::string type;

    if (node["type"].IsDefined()) {
        type = node["type"].as<std::string>();
    } else {
        type = "range";
    }

    rhs = factory::makeEmpty(type);
    rhs->deserialize_yaml(node);
    return true;
}

Node YAML::convert<csapex::param::ParameterConstPtr>::encode(const csapex::param::ParameterConstPtr& rhs)
{
    YAML::Node n;
    rhs->serialize_yaml(n);
    return n;
}

bool YAML::convert<csapex::param::ParameterConstPtr>::decode(const Node& node, csapex::param::ParameterConstPtr& rhs)
{
    std::string type;

    if (node["type"].IsDefined()) {
        type = node["type"].as<std::string>();
    } else {
        type = "range";
    }

    param::ParameterPtr res = factory::makeEmpty(type);
    res->deserialize_yaml(node);
    rhs = res;
    return true;
}
