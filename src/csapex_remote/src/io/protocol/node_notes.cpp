/// HEADER
#include <csapex/io/protcol/node_notes.h>

/// PROJECT
#include <csapex/serialization/serialization_buffer.h>
#include <csapex/serialization/note_serializer.h>

CSAPEX_REGISTER_NOTE_SERIALIZER(NodeNote)

using namespace csapex;

//why are nodenotes not getting through at all?????!?!?!?!?
//are any notes getting through?????
NodeNote::NodeNote()
{
}

NodeNote::NodeNote(NodeNoteType request_type, const AUUID &uuid)
    : NoteImplementation(uuid),
      note_type_(request_type)
{

}

NodeNote::NodeNote(NodeNoteType request_type, const AUUID &uuid, const std::vector<boost::any> &payload)
    : NoteImplementation(uuid),
      note_type_(request_type),
      payload_(payload)
{

}

void NodeNote::serialize(SerializationBuffer &data) const
{
    Note::serialize(data);

    data << note_type_;
    data << payload_;
}

void NodeNote::deserialize(const SerializationBuffer& data)
{
    Note::deserialize(data);

    data >> note_type_;
    data >> payload_;
}