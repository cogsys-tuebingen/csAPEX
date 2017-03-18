#ifndef PACKET_SERIALIZER_H
#define PACKET_SERIALIZER_H

/// PROJECT
#include <csapex/io/io_fwd.h>
#include <csapex/utility/singleton.hpp>
#include <csapex/serialization/serialization_fwd.h>
#include <csapex/serialization/serialization_buffer.h>

/// SYSTEM
#include <vector>
#include <map>

namespace csapex
{

class Serializer
{
public:
    virtual ~Serializer();

    virtual void serialize(const SerializablePtr& packet, SerializationBuffer &data) = 0;
    virtual SerializablePtr deserialize(SerializationBuffer& data) = 0;
};

/**
 * @brief The Serializer interface has to be implemented for packet types
 */

/**
 * @brief The PacketSerializer class is the facade to all existing packet serializers.
 *        It is responsible for (de-)serializing any Packet.
 */
class PacketSerializer : public Singleton<PacketSerializer>, public Serializer
{
    friend class Singleton<PacketSerializer>;

public:
    static SerializationBuffer serializePacket(const SerializablePtr& packet);
    static SerializablePtr deserializePacket(SerializationBuffer &serial);
    static void registerSerializer(uint8_t type, Serializer* serializer);

public:
    void serialize(const SerializablePtr& packet, SerializationBuffer &data);
    SerializablePtr deserialize(SerializationBuffer &data);


private:
    std::map<uint8_t, Serializer*> serializers_;
};

/**
 * @brief The SerializerRegistered template is used to register implementations of Serializer
 */
template <typename S>
struct SerializerRegistered
{
    SerializerRegistered(uint8_t type, S* instance) {
        PacketSerializer::registerSerializer(type, instance);
    }
};

}

#endif // PACKET_SERIALIZER_H