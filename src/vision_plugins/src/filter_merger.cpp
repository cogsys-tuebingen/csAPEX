#include "filter_merger.h"

/// PROJECT
#include <designer/box.h>
#include <vision_evaluator/box_manager.h>
#include <evaluator/messages_default.hpp>
#include <designer/connector_in.h>
#include <designer/connector_out.h>
#include <vision_evaluator/qt_helper.hpp>

/// SYSTEM
#include <QLabel>
#include <pluginlib/class_list_macros.h>

PLUGINLIB_EXPORT_CLASS(vision_evaluator::Merger, vision_evaluator::BoxedObject)

using namespace vision_evaluator;

Merger::Merger() :
    output_(NULL)
{
}

void Merger::fill(QBoxLayout *layout)
{
    if(output_ == NULL) {
        /// add output
        output_ = new ConnectorOut(box_, 0);
        box_->addOutput(output_);

        /// inputs
        input_count_ = QtHelper::makeSpinBox(layout, "Inputs: ", 2, 2, MERGER_INPUT_MAX);
        QSpinBox::connect(input_count_, SIGNAL(valueChanged(int)), this, SLOT(updateInputs(int)));
        updateInputs(2);
    }
}

Memento::Ptr Merger::getState() const
{
    boost::shared_ptr<State> memento(new State);
    *memento = state_;

    return memento;
}

void Merger::setState(Memento::Ptr memento)
{
    boost::shared_ptr<State> m = boost::dynamic_pointer_cast<State> (memento);
    assert(m.get());

    state_ = *m;
    input_count_->setValue(state_.input_count);
}

void Merger::messageArrived(ConnectorIn *source)
{
    input_arrivals_[source] = true;

    if(!gotAllArrivals())
        return;


    std::vector<cv::Mat> msgs;
    collectMessage(msgs);
    cv::Mat out_img;
    try {
        cv::merge(msgs, out_img);
        CvMatMessage::Ptr out_msg(new CvMatMessage);
        out_msg->value = out_img;
        output_->publish(out_msg);
    } catch (cv::Exception e) {
        std::cerr << " ERROR " << e.what() << std::endl;
    }

    resetInputArrivals();

}

void Merger::updateInputs(int value)
{
    state_.input_count = value;
    int current_amount = box_->countInputs();
    if(current_amount > value) {
        for(int i = current_amount; i > value ; i--) {
            ConnectorIn *ptr = box_->getInput(i - 1);
            if(ptr->isConnected())
                ptr->removeAllConnectionsUndoable();
            box_->removeInput(ptr);
            input_arrivals_.erase(ptr);
        }
    } else {
        int to_add = value - current_amount;
        for(int i = 0 ; i < to_add ; i++) {
            ConnectorIn* input = new ConnectorIn(box_, box_->countInputs() + i);
            box_->addInput(input);
            assert(QObject::connect(input, SIGNAL(messageArrived(ConnectorIn*)), this, SLOT(messageArrived(ConnectorIn*))));
            input_arrivals_.insert(std::pair<ConnectorIn*, bool>(input, false));

        }
    }

}

void Merger::collectMessage(std::vector<cv::Mat> &messages)
{
    for(int i = 0 ; i < box_->countInputs() ; i++) {
        ConnectorIn *in = box_->getInput(i);
        if(in->isConnected()) {
            CvMatMessage::Ptr msg = boost::dynamic_pointer_cast<CvMatMessage>(in->getMessage());
            messages.push_back(msg->value);
        }
    }
}

bool Merger::gotAllArrivals()
{
    bool ready = true;
    for(std::map<ConnectorIn*, bool>::iterator it = input_arrivals_.begin() ; it != input_arrivals_.end() ; it++) {
        ready &= !it->first->isConnected() || it->second;
    }
    return ready;
}

void Merger::resetInputArrivals()
{
    for(std::map<ConnectorIn*,bool>::iterator it = input_arrivals_.begin() ; it != input_arrivals_.end() ; it++) {
        it->second = false;
    }
}