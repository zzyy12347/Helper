#include "ScreenCaptureOverlay.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

namespace {
constexpr int kMinimumSelectionSize = 8;
}

ScreenCaptureOverlay::ScreenCaptureOverlay(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void ScreenCaptureOverlay::beginCapture()
{
    m_dragging = false;
    m_origin = {};
    m_current = {};
    m_virtualGeometry = desktopGeometry();
    setGeometry(m_virtualGeometry);
    show();
    raise();
    activateWindow();
    repaint();
}

void ScreenCaptureOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), QColor(15, 23, 42, 90));

    const QRect selection = currentSelection();
    if (selection.isValid()) {
        const QRect localSelection = selection.translated(-m_virtualGeometry.topLeft());
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(localSelection, Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setPen(QPen(QColor("#38bdf8"), 2));
        painter.drawRect(localSelection.adjusted(0, 0, -1, -1));
    }
}

void ScreenCaptureOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    m_dragging = true;
    m_origin = event->globalPos();
    m_current = m_origin;
    update();
}

void ScreenCaptureOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        return;
    }

    m_current = event->globalPos();
    update();
}

void ScreenCaptureOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_dragging || event->button() != Qt::LeftButton) {
        return;
    }

    m_dragging = false;
    m_current = event->globalPos();
    const QRect selection = currentSelection();
    hide();

    if (selection.width() < kMinimumSelectionSize || selection.height() < kMinimumSelectionSize) {
        emit captureCancelled();
        return;
    }

    emit selectionCaptured(selection);
}

void ScreenCaptureOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        emit captureCancelled();
        return;
    }

    QWidget::keyPressEvent(event);
}

QRect ScreenCaptureOverlay::desktopGeometry() const
{
    QRect combined;
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        combined = combined.united(screen->geometry());
    }
    return combined;
}

QRect ScreenCaptureOverlay::currentSelection() const
{
    return QRect(m_origin, m_current).normalized();
}
