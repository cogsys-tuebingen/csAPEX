#ifndef REGISTER_PLUGIN_H
#define REGISTER_PLUGIN_H

/// HEADER
#include <csapex/core/core_plugin.h>

namespace csapex {

class RegisterTransformPlugin : public CorePlugin
{
public:
    RegisterTransformPlugin();

    void init();
};

}

#endif // REGISTER_PLUGIN_H