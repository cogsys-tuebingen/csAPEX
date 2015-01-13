#ifndef SIGNAL_H
#define SIGNAL_H

/// COMPONENT
#include <csapex/msg/message.h>

namespace csapex {
namespace connection_types {

struct Signal : public ConnectionType
{
    typedef std::shared_ptr<Signal> Ptr;

    Signal();

    virtual ConnectionType::Ptr clone() const override;
    virtual ConnectionType::Ptr toType() const override;

    bool acceptsConnectionFrom(const ConnectionType* other_side) const override;
};


/// TRAITS
template <>
struct type<Signal> {
    static std::string name() {
        return std::string("Signal");
    }
};

}
}

/// YAML
namespace YAML {
template <>
struct convert<csapex::connection_types::Signal> {
  static Node encode(const csapex::connection_types::Signal&) {
      Node node;
      return node;
  }

  static bool decode(const Node&, csapex::connection_types::Signal&) {
      return true;
  }
};
}

#endif // SIGNAL_H