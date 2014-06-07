/// HEADER
#include <csapex/view/fulcrum_handle.h>

/// SYSTEM
#include <QEvent>
#include <QGraphicsSceneEvent>
#include <QGraphicsScene>
#include <QGraphicsWidget>
#include <QGraphicsLineItem>

using namespace csapex;

FulcrumHandle::FulcrumHandle(const QPointF& pos, QGraphicsItem *parent)
    : QGraphicsEllipseItem(parent)
{
    QPointF half_size_(5, 5);

    setFlag(QGraphicsItem::ItemIsMovable);
//    setFlag(QGraphicsItem::ItemIsSelectable);

    setPos(pos);
    setRect(QRectF(-half_size_, half_size_));
    setBrush(Qt::black);
}

bool FulcrumHandle::sceneEvent(QEvent *event)
{
    bool r =  QGraphicsEllipseItem::sceneEvent(event);

    switch(event->type()) {
    case QEvent::GraphicsSceneMouseMove:
        Q_EMIT moved(false);
        break;

    case QEvent::GraphicsSceneMouseRelease:
        Q_EMIT moved(true);
        break;

    default:
        break;
    }

    return r;
}
