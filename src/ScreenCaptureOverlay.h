#pragma once

#include <QRect>
#include <QWidget>

class ScreenCaptureOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ScreenCaptureOverlay(QWidget *parent = nullptr);

    void beginCapture();

signals:
    void selectionCaptured(const QRect &globalRect);
    void captureCancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QRect desktopGeometry() const;
    QRect currentSelection() const;

    QRect m_virtualGeometry;
    QPoint m_origin;
    QPoint m_current;
    bool m_dragging = false;
};
