#ifndef SETTINGS_IMPL_H
#define SETTINGS_IMPL_H

/// COMPONENT
#include <csapex/core/settings.h>
#include <csapex/model/observer.h>

namespace csapex
{
class SettingsImplementation : public Settings, public Observer
{
public:
    static SettingsImplementation NoSettings;

public:
    SettingsImplementation(bool load_from_config = true);

    bool knows(const std::string& name) const override;

    void add(csapex::param::Parameter::Ptr p, bool persistent) override;
    csapex::param::ParameterPtr get(const std::string& name) const override;
    csapex::param::ParameterPtr getNoThrow(const std::string& name) const override;

    void savePersistent() override;
    void loadPersistent() override;

    void saveTemporary(YAML::Node& node) override;
    void loadTemporary(YAML::Node& node) override;

private:
    struct Entry
    {
        csapex::param::Parameter::Ptr parameter;
        bool persistent;
    };

    std::map<std::string, Entry> settings_;
};

}  // namespace csapex

#endif  // SETTINGS_IMPL_H
