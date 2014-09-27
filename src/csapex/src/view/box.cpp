/// HEADER
#include <csapex/view/box.h>

/// COMPONENT
#include "ui_box.h"
#include <csapex/model/node.h>
#include <csapex/model/node_factory.h>
#include <csapex/msg/input.h>
#include <csapex/msg/output.h>
#include <csapex/model/node_worker.h>
#include <csapex/model/node_state.h>
#include <csapex/command/delete_node.h>
#include <csapex/command/meta.h>
#include <csapex/command/dispatcher.h>
#include <csapex/view/node_adapter.h>
#include <csapex/view/port.h>
#include <csapex/utility/context_menu_handler.h>
#include <csapex/utility/color.hpp>
#include <csapex/core/settings.h>

/// SYSTEM
#include <QDragMoveEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QMenu>
#include <QTimer>
#include <iostream>
#include <boost/foreach.hpp>
#include <cmath>

using namespace csapex;

const QString NodeBox::MIME = "csapex/model/box";

NodeBox::NodeBox(Settings& settings, NodeWorker::Ptr worker, NodeAdapter::Ptr adapter, QIcon icon, QWidget* parent)
    : QWidget(parent), ui(new Ui::Box), settings_(settings), node_worker_(worker), adapter_(adapter), icon_(icon),
      down_(false), info_compo(NULL), profiling_(false), is_placed_(false)
{
}

NodeBox::~NodeBox()
{
}


void NodeBox::setupUi()
{
    node_worker_->getNode()->checkConditions(true);

    if(!info_compo) {
        info_compo = new QLabel;
        info_compo->setProperty("component", true);
        ui->infos->addWidget(info_compo);
    }

    QObject::connect(node_worker_.get(), SIGNAL(messagesReceived()), this, SLOT(setupUiAgain()));
    adapter_->doSetupUi(ui->content);

    updateFlippedSides();

    Q_EMIT changed(this);
}

void NodeBox::setupUiAgain()
{
    adapter_->doSetupUi(ui->content);
    updateFlippedSides();
}

void NodeBox::construct()
{
    ui->setupUi(this);

    ui->input_layout->addSpacerItem(new QSpacerItem(16, 0));
    ui->output_layout->addSpacerItem(new QSpacerItem(16, 0));

    ui->enablebtn->setCheckable(true);
    ui->enablebtn->setChecked(node_worker_->getNode()->getNodeState()->isEnabled());

    ui->enablebtn->setIcon(icon_);

    setFocusPolicy(Qt::ClickFocus);

    const UUID& uuid = node_worker_->getNodeUUID();
    setToolTip(uuid.c_str());

    setObjectName(uuid.c_str());

    ui->content->installEventFilter(this);
    ui->label->installEventFilter(this);

    setLabel(node_worker_->getNode()->getNodeState()->getLabel());

    node_worker_->setMinimized(false);

    QObject::connect(ui->enablebtn, SIGNAL(toggled(bool)), this, SLOT(enableContent(bool)));

    QObject::connect(node_worker_.get(), SIGNAL(destroyed()), this, SLOT(deleteLater()));
    QObject::connect(node_worker_.get(), SIGNAL(nodeModelChanged()), this, SLOT(eventModelChanged()));
    QObject::connect(node_worker_.get(), SIGNAL(connectorCreated(Connectable*)), this, SLOT(registerEvent(Connectable*)));
    QObject::connect(node_worker_.get(), SIGNAL(connectorRemoved(Connectable*)), this, SLOT(unregisterEvent(Connectable*)));
    QObject::connect(node_worker_.get(), SIGNAL(nodeStateChanged()), this, SLOT(nodeStateChanged()));

    QObject::connect(node_worker_.get(), SIGNAL(enabled(bool)), this, SLOT(enabledChange(bool)));

    Q_FOREACH(Input* input, node_worker_->getMessageInputs()) {
        registerInputEvent(input);
    }
    Q_FOREACH(Output* output, node_worker_->getMessageOutputs()) {
        registerOutputEvent(output);
    }

    setupUi();
}

Node* NodeBox::getNode()
{
    return node_worker_->getNode();
}

NodeWorker* NodeBox::getNodeWorker()
{
    return node_worker_.get();
}

NodeAdapter::Ptr NodeBox::getNodeAdapter()
{
    return adapter_;
}

void NodeBox::enableContent(bool enable)
{
    node_worker_->setEnabled(enable);

    ui->label->setEnabled(enable);
}

void NodeBox::updateInformation(Graph* graph)
{
    int compo = graph->getComponent(node_worker_->getNodeUUID());
    if(compo < 0) {
        return;
    }

    std::stringstream info;
    info << compo;
    info_compo->setText(info.str().c_str());

    // set color using HSV rotation
    double hue =  (compo * 77) % 360;
    double r = 0, g = 0, b = 0;
    __HSV2RGB__(hue, 1., 1., r, g, b);
    double fr = 0, fb = 0, fg = 0;
    if(b > 100 && r < 100 && g < 100) {
        fr = fb = fg = 255;
    }
    std::stringstream ss;
    ss << "QLabel { background-color : rgb(" << r << "," << g << "," << b << "); color: rgb(" << fr << "," << fg << "," << fb << ");}";
    info_compo->setStyleSheet(ss.str().c_str());
}

void NodeBox::contextMenuEvent(QContextMenuEvent* e)
{
    Q_EMIT showContextMenuForBox(this, e->globalPos());
}

void NodeBox::fillContextMenu(QMenu *menu, std::map<QAction*, boost::function<void()> >& handler, CommandDispatcher* dispatcher)
{
    ContextMenuHandler::addHeader(*menu, std::string("Node: ") + node_worker_->getNodeUUID().getShortName());

    if(isMinimizedSize()) {
        QAction* max = new QAction("maximize", menu);
        max->setIcon(QIcon(":/maximize.png"));
        max->setIconVisibleInMenu(true);
        handler[max] = boost::bind(&NodeBox::minimizeBox, this, false);
        menu->addAction(max);

    } else {
        QAction* min = new QAction("minimize", menu);
        min->setIcon(QIcon(":/minimize.png"));
        min->setIconVisibleInMenu(true);
        handler[min] = boost::bind(&NodeBox::minimizeBox, this, true);
        menu->addAction(min);
    }

    QAction* flip = new QAction("flip sides", menu);
    flip->setIcon(QIcon(":/flip.png"));
    flip->setIconVisibleInMenu(true);
    handler[flip] = boost::bind(&NodeBox::flipSides, this);
    menu->addAction(flip);

    menu->addSeparator();

    QAction* term = new QAction("terminate thread", menu);
    term->setIcon(QIcon(":/stop.png"));
    term->setIconVisibleInMenu(true);
    handler[term] = boost::bind(&NodeBox::killContent, this);
    menu->addAction(term);

    QAction* prof;
    if(profiling_) {
        prof = new QAction("stop profiling", menu);
        prof->setIcon(QIcon(":/stop_profiling.png"));
    } else {
        prof = new QAction("profiling", menu);
        prof->setIcon(QIcon(":/profiling.png"));
    }
    prof->setIconVisibleInMenu(true);
    handler[prof] = boost::bind(&NodeBox::showProfiling, this);
    menu->addAction(prof);

    QAction* info = new QAction("get information", menu);
    info->setIcon(QIcon(":/help.png"));
    info->setIconVisibleInMenu(true);
    handler[info] = boost::bind(&NodeBox::getInformation, this);
    menu->addAction(info);

    menu->addSeparator();

    QAction* del = new QAction("delete", menu);
    del->setIcon(QIcon(":/close.png"));
    del->setIconVisibleInMenu(true);
    handler[del] = boost::bind(&NodeBox::deleteBox, this, dispatcher);
    menu->addAction(del);
}

QBoxLayout* NodeBox::getInputLayout()
{
    return ui->input_layout;
}

QBoxLayout* NodeBox::getOutputLayout()
{
    return ui->output_layout;
}

bool NodeBox::isError() const
{
    return node_worker_->getNode()->isError();
}
ErrorState::ErrorLevel NodeBox::errorLevel() const
{
    return node_worker_->getNode()->errorLevel();
}
std::string NodeBox::errorMessage() const
{
    return node_worker_->getNode()->errorMessage();
}

void NodeBox::setLabel(const std::string& label)
{
    apex_assert_hard(node_worker_->getNode()->getNodeState());
    node_worker_->getNode()->getNodeState()->setLabel(label);
    ui->label->setText(label.c_str());
    ui->label->setToolTip(label.c_str());
}

void NodeBox::setLabel(const QString &label)
{
    node_worker_->getNode()->getNodeState()->setLabel(label.toStdString());
    ui->label->setText(label);
}

std::string NodeBox::getLabel() const
{
    return node_worker_->getNode()->getNodeState()->getLabel();
}

void NodeBox::registerEvent(Connectable* c)
{
    if(c->isOutput()) {
        registerOutputEvent(dynamic_cast<Output*>(c));
    } else {
        registerInputEvent(dynamic_cast<Input*>(c));
    }
}

void NodeBox::unregisterEvent(Connectable*)
{
}

void NodeBox::registerInputEvent(Input* in)
{
    in->setParent(NULL);

    Q_EMIT changed(this);
}

void NodeBox::registerOutputEvent(Output* out)
{
    apex_assert_hard(out);

    out->setParent(NULL);

    Q_EMIT changed(this);
}

void NodeBox::resizeEvent(QResizeEvent *)
{
    Q_EMIT changed(this);
}

void NodeBox::init()
{
    if(parent()) {
        setVisible(true);
    } else {
        setVisible(false);
    }

    move(node_worker_->getNode()->getNodeState()->getPos());
}

bool NodeBox::eventFilter(QObject* o, QEvent* e)
{
    QMouseEvent* em = dynamic_cast<QMouseEvent*>(e);

    if(o == ui->label) {
        if(e->type() == QEvent::MouseButtonDblClick && em->button() == Qt::LeftButton) {
            Q_EMIT renameRequest(this);
            e->accept();

            return true;
        }
    }

    return false;
}

void NodeBox::enabledChange(bool val)
{
    ui->boxframe->setProperty("disabled", !val);

    refreshStylesheet();
}

void NodeBox::paintEvent(QPaintEvent*)
{
    if(!node_worker_ || !adapter_) {
        return;
    }

    bool is_error = node_worker_->getNode()->isError() && node_worker_->getNode()->errorLevel() == ErrorState::EL_ERROR;
    bool is_warn = node_worker_->getNode()->isError() && node_worker_->getNode()->errorLevel() == ErrorState::EL_WARNING;

    bool error_change = ui->boxframe->property("error").toBool() != is_error;
    bool warning_change = ui->boxframe->property("warning").toBool() != is_warn;

    ui->boxframe->setProperty("error", is_error);
    ui->boxframe->setProperty("warning", is_warn);

    if(error_change || warning_change) {
        if(is_error) {
            ui->label->setToolTip(node_worker_->getNode()->errorMessage().c_str());
        } else if(is_warn) {
            ui->label->setToolTip(node_worker_->getNode()->errorMessage().c_str());
        } else {
            ui->label->setToolTip(node_worker_->getNode()->getUUID().c_str());
        }

        refreshStylesheet();
    }

    resize(sizeHint());
}

void NodeBox::moveEvent(QMoveEvent* e)
{
    if(!is_placed_) {
        is_placed_ = true;
        return;
    }

    eventFilter(this, e);


    QPoint pos = e->pos();
    if(settings_.get("grid-lock", false)) {
        pos.setX(round(pos.x() / 10.0) * 10.0);
        pos.setY(round(pos.y() / 10.0) * 10.0);
        move(pos);
    }

    QPoint delta = pos - e->oldPos();

    node_worker_->getNode()->getNodeState()->setPos(pos);

    Q_EMIT moved(this, delta.x(), delta.y());
}

void NodeBox::triggerPlaced()
{
    Q_EMIT placed();
}

void NodeBox::setSelected(bool selected)
{
    ui->boxframe->setProperty("focused",selected);
    refreshStylesheet();
}

void NodeBox::keyPressEvent(QKeyEvent *)
{

}

void NodeBox::stop()
{
    QObject::disconnect(this);
    adapter_->stop();
}

void NodeBox::deleteBox(CommandDispatcher* dispatcher)
{
    dispatcher->execute(Command::Ptr(new command::DeleteNode(node_worker_->getNode()->getUUID())));
}

void NodeBox::getInformation()
{
    Q_EMIT helpRequest(this);
}

void NodeBox::refreshStylesheet()
{
    setStyleSheet(styleSheet());
}

void NodeBox::eventModelChanged()
{
    setupUi();

    adapter_->updateDynamicGui(ui->content);
}

void NodeBox::showProfiling()
{
    profiling_ = !profiling_;

    if(profiling_) {
        Q_EMIT profile(this);
    } else {
        Q_EMIT stopProfiling(this);
    }
}

void NodeBox::killContent()
{
    node_worker_->killExecution();
}

void NodeBox::flipSides()
{
    node_worker_->getNode()->getNodeState()->setFlipped(!node_worker_->getNode()->getNodeState()->isFlipped());
    updateFlippedSides();
}

void NodeBox::updateFlippedSides()
{
    bool flip = node_worker_->getNode()->getNodeState()->isFlipped();

    ui->boxframe->setLayoutDirection(flip ? Qt::RightToLeft : Qt::LeftToRight);
    ui->frame->setLayoutDirection(Qt::LeftToRight);

    Q_EMIT flipped(flip);
}

bool NodeBox::isMinimizedSize() const
{
    return node_worker_->getNode()->getNodeState()->isMinimized();
}

bool NodeBox::isFlipped() const
{
    return node_worker_->getNode()->getNodeState()->isFlipped();
}

void NodeBox::minimizeBox(bool minimize)
{    
    node_worker_->setMinimized(minimize);

    Q_EMIT minimized(minimize);

    if(minimize) {
        ui->frame->hide();
        ui->label->hide();
        ui->boxframe->setProperty("content_minimized", true);

    } else {
        ui->frame->show();
        ui->label->show();
        ui->boxframe->setProperty("content_minimized", false);
    }

    refreshStylesheet();

    resize(sizeHint());
}

bool NodeBox::hasSubGraph()
{
    return false;
}

Graph::Ptr NodeBox::getSubGraph()
{
    throw std::runtime_error("cannot call getSubGraph() on Box! Check with hasSubGraph()!");
}

void NodeBox::nodeStateChanged()
{
    minimizeBox(node_worker_->getNode()->getNodeState()->isMinimized());

    enableContent(node_worker_->getNode()->getNodeState()->isEnabled());
    ui->enablebtn->setChecked(node_worker_->getNode()->getNodeState()->isEnabled());

    setLabel(node_worker_->getNode()->getNodeState()->getLabel());
    ui->label->setToolTip(node_worker_->getNode()->getUUID().c_str());

    move(node_worker_->getNode()->getNodeState()->getPos());
}
