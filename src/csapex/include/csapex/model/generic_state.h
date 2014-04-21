#ifndef GENERIC_STATE_H
#define GENERIC_STATE_H

/// COMPONENT
#include <csapex/model/memento.h>

/// PROJECT
#include <utils_param/parameter_map.h>

/// SYSTEM
#include <boost/signals2.hpp>

namespace csapex
{

class GenericState : public Memento, public param::Parameter::access
{
public:
    typedef boost::shared_ptr<GenericState> Ptr;

public:
    GenericState();
    GenericState::Ptr clone() const;

    void addParameter(param::Parameter::Ptr param);

    void addTemporaryParameter(const param::Parameter::Ptr& param);
    void removeTemporaryParameters();

    void setParameterSetSilence(bool silent);
    void triggerParameterSetChanged();

    param::Parameter& operator [] (const std::string& name) const;
    param::Parameter::Ptr getParameter(const std::string& name) const;
    std::vector<param::Parameter::Ptr> getParameters() const;

    template <typename T>
    const T param (const std::string& name) const {
        try {
            return getParameter(name)->as<T>();
        } catch(const std::out_of_range& e) {
            throw std::runtime_error(std::string("unknown parameter '") + name + "'");
        }
    }

    void setFrom(const GenericState& rhs);

    virtual void writeYaml(YAML::Emitter& out) const;
    virtual void readYaml(const YAML::Node& node);

public:
    std::map<std::string, param::Parameter::Ptr> params;
    std::map<std::string, bool> temporary;
    std::vector<std::string> order;

    bool silent_;

    boost::shared_ptr<boost::signals2::signal<void()> > parameter_set_changed;
};

}

#endif // GENERIC_STATE_H
