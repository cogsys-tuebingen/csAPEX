#ifndef CTRL_TOOLPANEL_H
#define CTRL_TOOLPANEL_H

/// COMPONENT
#include "ctrl_cmpcore_bridge.h"
/// SYSTEM
#include <QObject>
#include <QPen>
/// DECLARATIONS
class QMainWindow;
class QComboBox;
class QPushButton;
class QDoubleSpinBox;
namespace Ui {
class ToolPanel;
}

class CtrlToolPanel : public QObject
{
    Q_OBJECT

public:
    typedef boost::shared_ptr<CtrlToolPanel> Ptr;

    CtrlToolPanel(QMainWindow *tool_bar, CMPCoreBridge::Ptr bridge);

    void setupUI(Ui::ToolPanel *ui);
    void sync();

Q_SIGNALS:
    void zoom(double factor);
    void boxSize(double size);
    void uncheckMov(bool check);
    void uncheckAdd(bool check);
    void uncheckDel(bool check);
    void uncheckSel(bool check);
    void classSelected(int id);
    void featuSelected(QString feat);
    void compute();

public Q_SLOTS:
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void zoomUpdate(double factor);
    void classChanged(int index);
    void featuChanged(int index);

    void buttonMov(bool checked);
    void buttonAdd(bool checked);
    void buttonDel(bool checked);
    void buttonSel(bool checked);
    void buttonCompute();

    void trainingFinished();
    void feedbackFinished();
    void classifierLoaded();
    void classAdded(int id);
    void classRemoved(int id);
    void classUpdated(int oldID, int newID);
    void colorUpdate(int id);



private:
    CMPCoreBridge::Ptr      bridge_;
    QMainWindow            *tool_bar_;
    QComboBox              *class_selection_;
    QComboBox              *featu_selection_;
    QDoubleSpinBox         *size_;
    QPen                    BlackPen;
    QPushButton            *button_compute_;
    QPushButton            *button_trash_;

    double zoom_;

    void snapZoom();
    QPixmap renderColorIcon(const int class_ID);

};

#endif // CTRL_TOOLPANEL_H
