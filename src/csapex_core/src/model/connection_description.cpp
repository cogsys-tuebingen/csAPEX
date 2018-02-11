/// HEADER
#include <csapex/model/connection_description.h>

/// PROJECT
#include <csapex/model/connectable.h>
#include <csapex/serialization/serialization_buffer.h>
#include <csapex/model/fulcrum.h>

using namespace csapex;

ConnectionDescription::ConnectionDescription(const UUID& from, const UUID& to,
                                             const TokenDataConstPtr& type,
                                             int id, bool active,
                                             const std::vector<Fulcrum> &fulcrums)
    : from(from), to(to), from_label(""), to_label(""), type(type),
      id(id), active(active),
      fulcrums(fulcrums)
{
}

ConnectionDescription::ConnectionDescription(const ConnectionDescription& other)
    : from(other.from), to(other.to), from_label(other.from_label), to_label(other.to_label), type(other.type),
      id(other.id), active(other.active),
      fulcrums(other.fulcrums)
{
}

ConnectionDescription::ConnectionDescription()
{
}


ConnectionDescription& ConnectionDescription::operator = (const ConnectionDescription& other)
{
    from = other.from;
    to = other.to;
    from_label = other.from_label;
    to_label = other.to_label;
    type = other.type;
    id = other.id;
    active = other.active;
    fulcrums = other.fulcrums;

    return *this;
}

bool ConnectionDescription:: operator == (const ConnectionDescription& other) const
{
    return from == other.from && to_label == other.to_label;
}

std::shared_ptr<Clonable> ConnectionDescription::makeEmptyClone() const
{
    return std::shared_ptr<Clonable>( new ConnectionDescription );
}

void ConnectionDescription::serialize(SerializationBuffer &data) const
{
    data << from;
    data << to;
    data << from_label;
    data << to_label;
    data << type;
    data << id;
    data << active;
    data << fulcrums;
}
void ConnectionDescription::deserialize(const SerializationBuffer& data)
{
    data >> from;
    data >> to;
    data >> from_label;
    data >> to_label;
    data >> type;
    data >> id;
    data >> active;
    data >> fulcrums;
}