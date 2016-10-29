#ifndef SPRITEMAINWINDOW_H
#define SPRITEMAINWINDOW_H

#include <QMainWindow>
#include <QPen>
#include <QCloseEvent>
#include "PopupWindow.h"
#include <QPainter>
#include "qevent.h"
#include "Sprite.h"
namespace Ui {
class SpriteMainWindow;
}

class SpriteMainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::SpriteMainWindow *ui;
    QString filename;

    // Drawing resources
    QPen pen;
    QPoint drawPoint;
    QColor penColor;
    QPixmap workspacePixMap;
    QPainter painter;

    // Flags
    bool isModified;
    bool mousePressed;
    bool maybeSave();
    bool clickedInsideWorkspace;     //TODO: keep or remove later

    // Helper methods
    void updateWorkspace();         // paints on the workspaceLabel's pixmap and updates it

    // Sprite
    Sprite currentSprite;

public:
    explicit SpriteMainWindow(QWidget *parent = 0);
    ~SpriteMainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event);      // Handles the QLabel events
    void closeEvent(QCloseEvent*);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    //void paintEvent(QPaintEvent *event);

private slots:

    //Button Slots

    void on_colorPickButton_clicked();

    void on_stampTool_clicked();

    void on_ellipseTool_clicked();

    void on_rectangleTool_clicked();

    void on_lineTool_clicked();

    void on_eraserTool_clicked();

    void on_penTool_clicked();

    void on_addFrameButton_clicked();

    void on_fpsSlider_valueChanged(int value);



    //Menu Item Slots

    void on_actionNew_triggered();

    void on_actionOpen_triggered();

    void on_actionSave_triggered();

    void on_actionStamp_triggered();

    void on_actionSprite_Sheet_triggered();

    void on_actionExport_as_gif_triggered();

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_actionReset_triggered();

    void on_actionFlip_Horizontally_triggered();

    void on_actionFlip_Vertically_triggered();

    void on_actionRotate_Horizontally_triggered();

    void on_actionRotate_Counterclockwise_triggered();

    void on_actionShow_Hide_Frame_triggered();

    void on_actionDuplicate_triggered();

    void on_actionDelete_triggered();

    void on_actionRemove_All_triggered();

    void on_actionAbout_triggered();

    void on_actionWalkthrough_triggered();
};

#endif // SPRITEMAINWINDOW_H
