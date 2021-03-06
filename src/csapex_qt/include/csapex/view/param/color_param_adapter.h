#ifndef COLOR_PARAM_ADAPTER_H
#define COLOR_PARAM_ADAPTER_H

/// COMPONENT
#include <csapex/view/param/param_adapter.h>
#include <csapex/param/color_parameter.h>

class QHBoxLayout;

namespace csapex
{
class ParameterContextMenu;

class CSAPEX_QT_EXPORT ColorParameterAdapter : public ParameterAdapter
{
public:
    ColorParameterAdapter(param::ColorParameter::Ptr p);

    QWidget* setup(QBoxLayout* layout, const std::string& display_name) override;

    void setupContextMenu(ParameterContextMenu* context_handler) override;

private:
    param::ColorParameterPtr color_p_;
};

}  // namespace csapex

#endif  // COLOR_PARAM_ADAPTER_H
