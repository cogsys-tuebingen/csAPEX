/// HEADER
#include "extract_keypoints.h"

/// COMPONENT
#include <csapex_vision_features/keypoint_message.h>

/// PROJECT
#include <utils/extractor.h>
#include <utils/extractor_factory.h>
#include <utils/extractor_manager.h>
#include <csapex/box.h>
#include <csapex/connector_out.h>
#include <csapex/connector_in.h>
#include <csapex_vision/cv_mat_message.h>
#include <csapex/qt_helper.hpp>

/// SYSTEM
#include <QFrame>
#include <pluginlib/class_list_macros.h>

PLUGINLIB_EXPORT_CLASS(csapex::ExtractKeypoints, csapex::BoxedObject)

using namespace csapex;
using namespace connection_types;

ExtractKeypoints::ExtractKeypoints()
    : selection_key(NULL), change(false), has_img(false), has_mask(false)
{
    addTag(Tag::get("Features"));
}

void ExtractKeypoints::messageArrived(ConnectorIn *source)
{
    if(source == in_img) {
        has_img = true;
    } else if(source == in_mask) {
        has_mask = true;
    }

    if(!extractor) {
        setError(true, "no extractor set");
        return;
    }

    if(change) {
        return;
    }

    bool use_mask = in_mask->isConnected();
    if(has_img && (has_mask || !use_mask)) {
        setError(false);

        has_img = false;
        has_mask = false;

        ConnectionType::Ptr msg = in_img->getMessage();
        CvMatMessage::Ptr img_msg = boost::dynamic_pointer_cast<CvMatMessage> (msg);

        KeypointMessage::Ptr key_msg(new KeypointMessage);

        {
            QMutexLocker lock(&extractor_mutex);
            if(use_mask) {
                ConnectionType::Ptr msg = in_mask->getMessage();
                CvMatMessage::Ptr mask_msg = boost::dynamic_pointer_cast<CvMatMessage> (msg);

                extractor->extractKeypoints(img_msg->value, mask_msg->value, key_msg->value);

            } else {
                extractor->extractKeypoints(img_msg->value, cv::Mat(), key_msg->value);
            }
        }

        out_key->publish(key_msg);
    }
}


void ExtractKeypoints::fill(QBoxLayout* layout)
{
    if(selection_key == NULL) {
        in_img = new ConnectorIn(box_, 0);
        in_img->setLabel("Image");
        box_->addInput(in_img);
        in_mask = new ConnectorIn(box_, 1);
        in_mask->setLabel("Mask (opt.)");
        box_->addInput(in_mask);

        out_key = new ConnectorOut(box_, 0);
        out_key->setType(csapex::connection_types::KeypointMessage::make());
        out_key->setLabel("Keypoints");
        box_->addOutput(out_key);

        ExtractorManager& manager = ExtractorManager::instance();

        selection_key = new QComboBox;
        typedef std::pair<std::string, ExtractorManager::ExtractorInitializer> Pair;
        foreach(Pair fc, manager.featureDetectors()) {
            std::string key = fc.second.getType();
            selection_key->addItem(key.c_str());

            state.params[key] = ExtractorManager::instance().featureDetectorParameters(key);
        }
        layout->addLayout(QtHelper::wrap("Keypoint", selection_key));

        opt = new QFrame;
        opt->setLayout(new QVBoxLayout);
        layout->addWidget(opt);

        QObject::connect(selection_key, SIGNAL(currentIndexChanged(int)), this, SLOT(update(int)));

        QObject::connect(box_, SIGNAL(placed()), this, SIGNAL(modelChanged()));
    }
}

void ExtractKeypoints::updateDynamicGui(QBoxLayout *layout)
{
    update(0);
}

template <typename T>
void ExtractKeypoints::updateParam(const std::string& name, T value)
{
    BOOST_FOREACH(vision::Parameter& para, state.params[state.key]) {
        if(para.name() == name) {
            para.set<T>(value);

            change = true;
            Q_EMIT guiChanged();

            return;
        }
    }
}

void ExtractKeypoints::update(int slot)
{
    state.key = selection_key->currentText().toStdString();

    QtHelper::clearLayout(opt->layout());
    QBoxLayout* layout = dynamic_cast<QBoxLayout*> (opt->layout());
    assert(layout);

    foreach(QObject* cb, callbacks) {
        delete cb;
    }
    callbacks.clear();

    foreach(const vision::Parameter& para, state.params[state.key]) {
        std::string name = para.name();

        if(para.is<int>()) {
            QSlider* slider = QtHelper::makeSlider(layout, name , para.as<int>(), para.min<int>(), para.max<int>());
            slider->setValue(para.as<int>());

            boost::function<void()> cb = boost::bind(&ExtractKeypoints::updateParam<int>, this, name, boost::bind(&QSlider::value, slider));
            qt_helper::Call* call = new qt_helper::Call(cb);
            callbacks.push_back(call);

            QObject::connect(slider, SIGNAL(valueChanged(int)), call, SLOT(call()));

        } else if(para.is<double>()) {
            QDoubleSlider* slider = QtHelper::makeDoubleSlider(layout, name , para.as<double>(), para.min<double>(), para.max<double>(), para.step<double>());
            slider->setDoubleValue(para.as<double>());

            boost::function<void()> cb = boost::bind(&ExtractKeypoints::updateParam<double>, this, name, boost::bind(&QDoubleSlider::doubleValue, slider));
            qt_helper::Call* call = new qt_helper::Call(cb);
            callbacks.push_back(call);

            QObject::connect(slider, SIGNAL(valueChanged(int)), call, SLOT(call()));

        } else if(para.is<bool>()) {
            QCheckBox* box = new QCheckBox;
            box->setChecked(para.as<bool>());

            layout->addLayout(QtHelper::wrap(name, box));

            boost::function<void()> cb = boost::bind(&ExtractKeypoints::updateParam<bool>, this, name, boost::bind(&QCheckBox::isChecked, box));
            qt_helper::Call* call = new qt_helper::Call(cb);
            callbacks.push_back(call);

            QObject::connect(box, SIGNAL(toggled(bool)), call, SLOT(call()));

        } else {
            opt->layout()->addWidget(new QLabel((name + "'s type is not yet implemented").c_str()));
        }
    }


    update();
}

void ExtractKeypoints::updateModel()
{
    if(change) {
        change = false;
        update();
    }
}

void ExtractKeypoints::update()
{
    Extractor::Ptr next = ExtractorFactory::create(state.key, "", vision::StaticParameterProvider(state.params[state.key]));

    QMutexLocker lock(&extractor_mutex);
    extractor = next;
    std::cout << "warning: set!" << std::endl;
}

Memento::Ptr ExtractKeypoints::getState() const
{
    return boost::shared_ptr<State>(new State(state));
}

void ExtractKeypoints::setState(Memento::Ptr memento)
{
    boost::shared_ptr<ExtractKeypoints::State> m = boost::dynamic_pointer_cast<ExtractKeypoints::State> (memento);
    assert(m.get());

    //    state = *m;
    state.key = m->key;

    typedef std::pair<std::string, std::vector<vision::Parameter> > Pair;
    foreach(Pair pair, m->params) {
        foreach(const vision::Parameter& para, pair.second) {
            std::vector<vision::Parameter>& target = state.params[pair.first];
            BOOST_FOREACH(vision::Parameter& existing_param, target) {
                if(existing_param.name() == para.name()) {
                    existing_param.setFrom(para);
                }
            }
        }
    }

    int slot = 0;
    for(int i = 0, n = selection_key->count(); i < n; ++i) {
        if(selection_key->itemText(i).toStdString() == state.key) {
            slot = i;
            break;
        }
    }
    selection_key->setCurrentIndex(slot);
}


void ExtractKeypoints::State::writeYaml(YAML::Emitter& out) const {
    out << YAML::Key << "key" << YAML::Value << key;
    out << YAML::Key << "params" << YAML::Value << params;
}
void ExtractKeypoints::State::readYaml(const YAML::Node& node) {
    if(node.FindValue("params")) {
        node["params"] >> params;
    }
    if(node.FindValue("key")) {
        node["key"] >> key;
    }
}