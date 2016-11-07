#include <QColorDialog>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QTransform>
#include <iostream>
#include <QPoint>
#include <string>
#include <QDebug>
#include "SpriteMainWindow.h"
#include "ui_SpriteMainWindow.h"
#include "GetResolutionDialog.h"

#include <QScrollBar>

SpriteMainWindow::SpriteMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SpriteMainWindow)
{
    GetResolutionDialog welcomeScreen;
    connect(&welcomeScreen, SIGNAL(okClicked(int,int)), this, SLOT(initialResolution(int,int)));
    welcomeScreen.exec();


    ui->setupUi(this);
    this->setWindowTitle("Pixels Sprite Editor");
    //this->setWindowFlags(Qt::FramelessWindowHint);

    // default to pen
    on_penTool_clicked();

    //penColor is recorded so that when a color picker is selected,
    //the beginning color will be the current penColor.
    penColor = qRgb(255, 198, 6);
    pen.setColor(penColor);
    penWidthSelected = 1;
    transparentGridIsVisible = true;
    pen.setWidth(penWidthSelected);
    clickedInsideWorkspace = false;     //TODO: keep or remove later
    timesScaled = 0;
    mainWindowOriginalGeometry = this->saveGeometry();


    mousePressed = false;
    drawStarted = false;


    filename = "";
    isModified = false;

    // Assign buttons to the button group (with ids). These
    // ID's can be used to indentify which button was pressed.

    toolsButtonsGroup= new QButtonGroup(this);
    toolsButtonsGroup->addButton(ui->penTool, 0);
    toolsButtonsGroup->addButton(ui->eraserTool, 1);
    toolsButtonsGroup->addButton(ui->lineTool, 2);
    toolsButtonsGroup->addButton(ui->rectangleTool, 3);
    toolsButtonsGroup->addButton(ui->ellipseTool, 4);
    toolsButtonsGroup->addButton(ui->stampTool, 5);
    //toolsButtonsGroup->addButton(ui->undoButton, 6);
    //toolsButtonsGroup->addButton(ui->redoButton, 7);
    //toolsButtonsGroup->addButton(ui->flipVerticalButton, 8);
    //toolsButtonsGroup->addButton(ui->flipHorizontalButton, 9);
    //toolsButtonsGroup->addButton(ui->rotateCounterClockButton, 10);
    //toolsButtonsGroup->addButton(ui->rotateClockwiseButton, 11);
    //toolsButtonsGroup->addButton(ui->unassignedButton_3, 12);
    //toolsButtonsGroup->addButton(ui->unassignedButton_4, 13);
    //toolsButtonsGroup->addButton(ui->unassignedButton_5, 14);
    //toolsButtonsGroup->addButton(ui->unassignedButton_6, 15);

    brushSizeButtonsGroup = new QButtonGroup(this);
    brushSizeButtonsGroup->addButton(ui->brushSize1Button, 0);
    brushSizeButtonsGroup->addButton(ui->brushSize2Button, 1);
    brushSizeButtonsGroup->addButton(ui->brushSize3Button, 2);
    brushSizeButtonsGroup->addButton(ui->brushSize4Button, 3);

    // Set pixmap's resolution, color and set it to the workspace.
    //workspacePixMap = QPixmap(this->spriteWidth, this->spriteHeight).scaled(this->WORKSPACE_DIMENSION);
    workspacePixMap = QPixmap(this->spriteWidth, this->spriteHeight);
    qDebug() << this->spriteWidth;
    qDebug() << this->spriteHeight;

    workspacePixMap.fill(this->backgroundColor);
    ui->workspaceLabel->setPixmap(workspacePixMap);

    mousePressed = false;

    shapeShouldNowBeDrawn = false;

    // Connections for undo,redo,zooming buttons
    connect(ui->undoButton, &QPushButton::clicked, this, &SpriteMainWindow::on_actionUndo_triggered);
    connect(ui->redoButton, &QPushButton::clicked, this, &SpriteMainWindow::on_actionRedo_triggered);
    connect(ui->zoomInButton, &QPushButton::clicked, this, &SpriteMainWindow::on_action2x_Workspace_triggered);
    connect(ui->zoomOutButton, &QPushButton::clicked, this, &SpriteMainWindow::on_actionZoom_Out_triggered);
    connect(ui->oneToOneButton, &QPushButton::clicked, this, &SpriteMainWindow::on_actionReset_Size_triggered);

    // Install SpriteMainWindow as an event handler for the workspaceLabel
    ui->workspaceLabel->installEventFilter(this);
    ui->previewLabelMap->installEventFilter(this);

    //create the sprite
    Sprite temp(spriteWidth, spriteHeight, 0, tr("MySprite"));
    currentSprite = temp;

    connect(&currentSprite, SIGNAL(frameClicked(Frame*)), this, SLOT(frameClicked(Frame*)));

    QVBoxLayout* layout = new QVBoxLayout;
    //Frame* something = &currentSprite.getFrame(0);

    Frame* s =currentSprite.getFrames().last();
    s->setMinimumSize(160,160);
    s->setMaximumSize(160,160);
    s->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    s->setScaledContents(true);
    layout->addWidget(s);
    ui->scrollAreaWidgetContents->setLayout(layout);
    // Frame* something = new Frame();
    //ui->scrollAreaWidgetContents->layout()->

    //sets the current frame to the first frame
    currentFrame = currentSprite.getFrames().last();

    currentFrame->makeFrameActive();

    connect(currentFrame, SIGNAL(clicked(Frame*)), this, SLOT(frameClicked(Frame*)));

    //currentFrame->setPixmap(workspacePixMap.scaled(this->FRAME_VIEW_DIMENSION));
    currentFrame->setPixmap(workspacePixMap.copy());

    QPixmap tempPixmap = *currentSprite.getFrame(0).pixmap();
    //ui->previewLabelMap->setPixmap(tempPixmap.scaled(PREVIEW_DIMENSION));
    ui->previewLabelMap->setPixmap(tempPixmap.copy());

    // Setup and start the preview timer
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(whenTimerUpdates()));
    timer->start(ui->fpsSlider->value());



    connect(&(this->previewWindow), SIGNAL(fpsChanged(int)), this, SLOT(setFps(int)));

    //Add loading custom stamp stuff here
    numCustomStamps = 0;
    selector = new StampSelector();
    connect(this, SIGNAL(sendRes(QRect)), selector, SLOT(resolution(QRect)));
    connect(selector, SIGNAL(selectedStamp(int)), this, SLOT(changeStamp(int)));
}

SpriteMainWindow::~SpriteMainWindow()
{
    delete ui;
}

// Handles the events inside a QLabel. QLabel unlike QWidget cant emit signals
// for events like mouse click, etc. Thus, clicking and dragging anywhere on our
// SpriteMainWindow would draw on workspace (since it inherit from QWidget). But,
// this overidden method handles QLabel events, and if the event didnt occur in
// QLabel, then will pass it to the SpriteMainWindow.
//
// Can add more stuff (like for tools, etc). Much cleaner than creating a custom
// class and making it inherit from QLabel (and overide the mouse events).
bool SpriteMainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (qobject_cast<QLabel*>(watched)==ui->previewLabelMap && event->type() == QEvent::MouseButtonPress){
        //if(event->type() == QEvent::MouseButtonPress){

        qDebug() << "Preview Clicked";
            previewWindow.setPixmap(workspacePixMap);
            previewWindow.setFps(currentSprite.getFps());
            previewWindow.exec();
        //}
        return true;
    }
    if (watched == ui->workspaceLabel) {
        clickedInsideWorkspace = true;
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mousePressEvent = static_cast<QMouseEvent*>(event);
            int canvasX = mousePressEvent->pos().x() - ((ui->workspaceLabel->width()/2) - (workspacePixMap.width()/2));
            int canvasY = mousePressEvent->pos().y() - ((ui->workspaceLabel->height()/2) - (workspacePixMap.height()/2));
            qDebug() << "Left mouse pressed in workspace: (" << QString::number(canvasX) << ", " << QString::number(canvasY) << ")";

            if (brush == pencil || brush == eraser || brush == stamp){
                drawPoint.setX(canvasX);
                drawPoint.setY(canvasY);
                //mousePressed = true;
            }
            else if (brush == line) {
                mLine.setP1(QPoint(canvasX, canvasY));
                mLine.setP2(QPoint(canvasX, canvasY));
            } else if (brush == rect || brush == ellipse) {
                mRect.setTopLeft(QPoint(canvasX, canvasY));
                mRect.setBottomRight(QPoint(canvasX, canvasY));
            }

            mousePressed = true;

            // Save this pixmap, cap at 20 frames
            if(undoStack.size() >= 20) {
                undoStack.pop_front();
            }
            undoStack.push_back(workspacePixMap);

            return true;
        }
        if(event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseMoveEvent = static_cast<QMouseEvent*>(event);
            int canvasX = mouseMoveEvent->pos().x() - ((ui->workspaceLabel->width()/2) - (workspacePixMap.width()/2));
            int canvasY = mouseMoveEvent->pos().y() - ((ui->workspaceLabel->height()/2) - (workspacePixMap.height()/2));

            //qDebug() << "mouse is being moved in workspace: (" << QString::number(canvasX) << ", " << QString::number(canvasY) << ")";
            qDebug() << "Brush size:" << pen.width();

            if (brush == pencil || brush == eraser || brush == stamp){
                drawPoint.setX(canvasX);
                drawPoint.setY(canvasY);
            }
            else if (brush == line){
                mLine.setP2(QPoint(canvasX, canvasY));
            } else if (brush == rect || brush == ellipse) {
                mRect.setBottomRight(QPoint(canvasX, canvasY));
            }
            updateWorkspace();
            return true;
        }
        if(event->type() == QEvent::MouseButtonRelease) {
            qDebug() << "mouse left click released inside workspace";

            mousePressed = false;

            if (brush == line || brush == rect || brush == ellipse || brush == stamp){
                shapeShouldNowBeDrawn = true;
            }
            updateWorkspace();



            //NEED TO SCALE IT HERE FOR THE SIDE VIEW
            //currentFrame->setPixmap(workspacePixMap.scaled(this->FRAME_VIEW_DIMENSION));
            currentFrame->setPixmap(workspacePixMap.copy());



            //ui->workspaceLabel->setPixmap(currentFrame->pixmap()->scaled(this->WORKSPACE_DIMENSION));

            return true;
        }
        else {
            return false;
        }
    } else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(watched, event);
    }
}

// Nofity when the mouse is clicked
void SpriteMainWindow::mousePressEvent(QMouseEvent *event) {

    event->ignore();
    // CRAP FOR QLABEL EVENTS. ONLY ADD FOR OTHER WIDGETS' EVENTS
}

// Track mouse moving events
//void SpriteMainWindow::mouseMoveEvent(QMouseEvent *event) {
//    if(mousePressed) {
//        drawPoint.setX(event->pos().x() -242);
//        drawPoint.setY(event->pos().y() - 50);
//    }
//    updateWorkspace();

    // CRAP FOR QLABEL EVENTS. ONLY ADD FOR OTHER WIDGETS' EVENTS
//}

// Notify when the mouse ie released
void SpriteMainWindow::mouseReleaseEvent(QMouseEvent *event) {

    event->ignore();

    // CRAP FOR QLABEL EVENTS. ONLY ADD FOR OTHER WIDGETS' EVENTS
}

// Draws on the workspace's pixmap and reassigns it. All the tools will
// paint in this method. (Replacement for paintEvent() method).
void SpriteMainWindow::updateWorkspace() {
   if (brush == pencil || brush == eraser || shapeShouldNowBeDrawn){
    painter.begin(&workspacePixMap);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    //painter.setRenderHints(QPainter::Antialiasing, QPainter::SmoothPixmapTransform);


    painter.setPen(pen);

    if (brush == pencil || brush == eraser){
        painter.drawPoint(drawPoint);
    }
    else if (brush == line){
        //draw the line once mouse is actually released
        painter.drawLine(mLine);
        shapeShouldNowBeDrawn = false;
    } else if (brush == rect) {
        painter.drawRect(mRect);
        shapeShouldNowBeDrawn = false;
    }else if (brush == ellipse) {
        painter.drawEllipse(mRect);
        shapeShouldNowBeDrawn = false;
    }else if (brush == stamp){
        painter.drawPixmap(drawPoint, selectedStamp);
        shapeShouldNowBeDrawn = false;
    }


     painter.end();

     ui->workspaceLabel->setPixmap(workspacePixMap);
    }
   //this is updating a temporary pixmap to the line before the mouse is released
   else if (brush == line || brush == rect || brush == ellipse || brush == stamp){

           QPixmap temp = QPixmap(workspacePixMap);
           QPainter tempPainter(&temp);

           //alternate way based off of link on Slack; I don't know how to make it work
           //painter.begin(&workspacePixMap);
           // painter.drawPixmap(canvasX,canvasY, workspacePixMap);
           //painter.drawLine(mLine);
           //painter.end();

           tempPainter.setPen(pen);

           if(brush == line) {
               tempPainter.drawLine(mLine);
           } else if(brush == rect) {
               tempPainter.drawRect(mRect);
           } else if(brush == ellipse) {
               tempPainter.drawEllipse(mRect);
           } else if(brush == stamp){
               tempPainter.drawPixmap(drawPoint, selectedStamp);
           }

           ui->workspaceLabel->setPixmap(temp);
   }


    isModified = true;
}

//void SpriteMainWindow::paintEvent(QPaintEvent *event) {

//}

//Slots

void SpriteMainWindow::on_colorPickButton_clicked()
{
    penColor = QColorDialog::getColor(penColor);
    pen.setColor(penColor);
    QString s = "#colorPickButton {background-color: rgb(";
    int rHover = penColor.red();
    int gHover = penColor.green();
    int bHover = penColor.blue();
    if(!(rHover < 40)) { rHover -= 40;}
    if(!(gHover < 40)) { gHover -= 40;}
    if(!(bHover < 40)) { bHover -= 40;}
    QString color = QString::number(penColor.red()).append(",").append(QString::number(penColor.green())).append(",").append(QString::number(penColor.blue()));
    QString hoverColor = QString::number(rHover).append(",").append(QString::number(gHover)).append(",").append(QString::number(bHover));

    s.append(color).append(");border: 6px solid rgb(252, 252, 252);border-radius: 50px;background-repeat: none;}#colorPickButton:hover{");
    s.append("background-color: rgb(").append(hoverColor).append(");border: 8px solid rgb(252, 252, 252);border-radius: 50px;}");
    ui->colorPickButton->setStyleSheet(s.toLatin1());
}

void SpriteMainWindow::on_stampTool_clicked()
{   
    call();
    selector->exec();

    brush = stamp;
}

void SpriteMainWindow::call() {
    //qDebug() << this->width() << this->height();
    emit(sendRes(this->geometry()));
}

void SpriteMainWindow::changeStamp(int id){
    if(id == 0){
        selectedStamp.load(":/stamps/Retro-Mario-icon.png");
    }
    else if(id == 1){
        selectedStamp.load(":/stamps/orange-sun.png");
    }
    else if(id == 2){
        selectedStamp.load(":/stamps/cloud.png");
    }
    else if(id == 3){
        selectedStamp.load(":/stamps/pokeball.png");
    }
    else if(id == 4){
        selectedStamp.load(":/stamps/sword.png");
    }
    else if(id == 5){
        selectedStamp.load(":/stamps/sunglasses.png");
    }
}

void SpriteMainWindow::on_ellipseTool_clicked()
{
    pen.setColor(penColor);
    brush = ellipse;
}

void SpriteMainWindow::on_rectangleTool_clicked()
{
    pen.setColor(penColor);
    brush = rect;
}

void SpriteMainWindow::on_lineTool_clicked()
{
    pen.setColor(penColor);
    brush = line;
}

void SpriteMainWindow::on_eraserTool_clicked()
{
    pen.setColor(this->backgroundColor);
    brush = eraser;
}

void SpriteMainWindow::on_penTool_clicked()
{
    pen.setColor(penColor);
    brush = pencil;
    this->cursor().setShape(Qt::CrossCursor);
}

//Add a Frame
void SpriteMainWindow::on_addFrameButton_clicked()
{
    // Resize previous zoomed frame before switching
    currentFrame->setPixmap(currentFrame->pixmap()->scaled(spriteWidth, spriteHeight));
    currentFrame->makeFrameUnactive();

    currentSprite.addFrame();
    Frame* s =currentSprite.getFrames().last();
    s->setMinimumSize(160,160);
    s->setMaximumSize(160,160);
    s->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    s->setScaledContents(true);
    ui->scrollAreaWidgetContents->layout()->addWidget(s);

    currentFrame = currentSprite.getFrames().last();

    currentFrame->makeFrameActive();

    QPixmap map(spriteWidth, spriteHeight);
    map.fill(this->backgroundColor);
//    currentFrame->setPixmap(map.scaled(this->FRAME_VIEW_DIMENSION));
    currentFrame->setPixmap(map);

    //workspacePixMap = currentFrame->pixmap()->scaled(this->WORKSPACE_DIMENSION);
    workspacePixMap = currentFrame->pixmap()->copy();
    ui->workspaceLabel->setPixmap(workspacePixMap);

    //sets the scrollbar to the bottom
    QScrollBar* verticalScrollBar = ui->framesScrollArea->verticalScrollBar();
    verticalScrollBar->setMaximum(verticalScrollBar->maximum() + 170);
    verticalScrollBar->setSliderPosition(verticalScrollBar->maximum());

}

void SpriteMainWindow::on_fpsSlider_valueChanged(int value)
{
    currentSprite.setFps(value);
    if(value == 0)
        timer->start(10);
    else
        timer->start(1000/currentSprite.getFps());
}

//Menu Item Slots

void SpriteMainWindow::on_actionNew_triggered()
{
    //Check if the user wants to save any changes first, then trigger the reset action.
    if(maybeSave()){
        GetResolutionDialog welcomeScreen;
        connect(&welcomeScreen, SIGNAL(okClicked(int,int,int)), this, SLOT(initialResolution(int,int)));
        welcomeScreen.exec();
        this->on_actionReset_triggered();
    }
    //Clear the workspacePixMap
//    workspacePixMap = QPixmap(this->spriteWidth, this->spriteHeight).scaled(this->WORKSPACE_DIMENSION);
    workspacePixMap = QPixmap(this->spriteWidth, this->spriteHeight);
    workspacePixMap.fill(this->backgroundColor);
    ui->workspaceLabel->setPixmap(workspacePixMap);

    //Clear out the current Sprite
    Sprite newSprite(this->spriteWidth, this->spriteHeight, 0, "IDK");


    currentSprite = newSprite;

    //sets the current frame to the first frame
    currentFrame = currentSprite.getFrames().last();

    connect(currentFrame, SIGNAL(clicked(Frame*)), this, SLOT(frameClicked(Frame*)));
    //currentFrame->setPixmap(workspacePixMap.scaled(this->FRAME_VIEW_DIMENSION));
    currentFrame->setPixmap(workspacePixMap);

    currentFrame->makeFrameActive();

    //Deletes old layout and its children
    QLayoutItem* child;
    while((child = ui->scrollAreaWidgetContents->layout()->takeAt(0)) != 0){
        delete child->widget();
        delete child;
    }



    //Reset the frames display on the left
//    QVBoxLayout* layout = new QVBoxLayout;
//    layout->addWidget(currentFrame);
//    ui->scrollAreaWidgetContents->setLayout(layout);

    currentFrame->setMinimumSize(160,160);
    currentFrame->setMaximumSize(160,160);
    currentFrame->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    currentFrame->setScaledContents(true);
    ui->scrollAreaWidgetContents->layout()->addWidget(currentFrame);



}

//Open a file
void SpriteMainWindow::on_actionOpen_triggered()
{
    /*//Check if the user wants to save any changes first, then open a new project.
    if(maybeSave()){
        QFileDialog dialog;
        QString filename = dialog.getOpenFileName();

    }*/

    //creates a dialog and a popup appears asking for filename.
    QFileDialog dialog;
    QString nameOfFile= dialog.getOpenFileName(this, tr("Open File"), "", tr("Sprite (*.ssp)"));

    //if user canceled out of the dialog, return and do nothing.
    if (nameOfFile == ""){
        return;
    }

    //Check if the user wants to save any changes first, then open the project.
    maybeSave();

    //opening project
    currentSprite.loadFile(nameOfFile);



    //Deletes old layout and its children
    QLayoutItem* child;
    while((child = ui->scrollAreaWidgetContents->layout()->takeAt(0)) != 0){
        delete child->widget();
        delete child;
    }

    //Adds all the frames that are in the vector to the GUI layout
    for (int i = 0; i < currentSprite.getFrames().size(); i++){
        Frame* tmp = &currentSprite.getFrame(i);
        tmp->setMinimumSize(160,160);
        tmp->setMaximumSize(160,160);
        tmp->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        tmp->setScaledContents(true);
        tmp->makeFrameUnactive();
        ui->scrollAreaWidgetContents->layout()->addWidget(tmp);

    }

    currentFrame = (currentSprite.getFrames().first());

    //Clear the workspacePixMap
    //workspacePixMap = currentFrame->pixmap()->scaled(this->WORKSPACE_DIMENSION);
    workspacePixMap = currentFrame->pixmap()->copy();
    //workspacePixMap.fill(this->backgroundColor);
    ui->workspaceLabel->setPixmap(workspacePixMap);


    ui->framesScrollArea->verticalScrollBar()->setSliderPosition(ui->framesScrollArea->verticalScrollBar()->minimum());

    //ui->workspaceLabel->setPixmap(currentFrame->pixmap()->scaled(this->WORKSPACE_DIMENSION));

    currentFrame->makeFrameActive();



}

//Save a file
void SpriteMainWindow::on_actionSave_triggered()
{
    QFileDialog dialog;
    isModified = false;
    resetAllFrameSizes();

    currentSprite.saveFile(dialog.getSaveFileName(this, tr("Save File"), "/untitled.ssp", tr("Images (*.ssp)")));

    //This saves the pixmap to a png
    /*QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QPixmap map = workspacePixMap.scaled(this->spriteWidth, this->spriteHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    map.save(&file, "PNG");*/
}

//Slot for when the SpriteSheet menu item is selected.
void SpriteMainWindow::on_actionSprite_Sheet_triggered()
{

}

//Slot for when export as GIF is selected from the menu.
void SpriteMainWindow::on_actionExport_as_gif_triggered()
{
    QFileDialog dialog;
    isModified = false;
    currentSprite.exportToGif(dialog.getSaveFileName(this, tr("Save File"), "/untitled.gif", tr("Images (*.gif)")));
}

//Slot for when undo is selected from the menu.
void SpriteMainWindow::on_actionUndo_triggered()
{

    if(!undoStack.empty()) {
        // Save the current frame
        redoStack.push_back(workspacePixMap);
        workspacePixMap = undoStack.back();
        ui->workspaceLabel->setPixmap(workspacePixMap);
        //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
        currentFrame->setPixmap(workspacePixMap.copy());

        undoStack.pop_back();
    }
}

//Slot for when the redo button is selected the menu.
void SpriteMainWindow::on_actionRedo_triggered()
{
    if(!redoStack.empty()) {
        workspacePixMap = redoStack.back();
        ui->workspaceLabel->setPixmap(workspacePixMap);
        undoStack.push_back(redoStack.back());
        redoStack.pop_back();
        //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
        currentFrame->setPixmap(workspacePixMap.copy());
    }
}

//Slot for when the reset option is selected from the menu.
void SpriteMainWindow::on_actionReset_triggered()
{
    workspacePixMap = QPixmap(this->spriteWidth, this->spriteHeight);
    workspacePixMap.fill(this->backgroundColor);
    ui->workspaceLabel->setPixmap(workspacePixMap);
    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());
}

//Slot for when the Flip Horizontally option is selected from the menu.
void SpriteMainWindow::on_actionFlip_Horizontally_triggered()
{
    QImage image = workspacePixMap.toImage().mirrored(true, false);
    workspacePixMap = QPixmap::fromImage(image);
    ui->workspaceLabel->setPixmap(workspacePixMap);
    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());
}

//Slot for when the flip Vertically option is selected from the menu.
void SpriteMainWindow::on_actionFlip_Vertically_triggered()
{
    QImage image = workspacePixMap.toImage().mirrored(false, true);
    workspacePixMap = QPixmap::fromImage(image);
    ui->workspaceLabel->setPixmap(workspacePixMap);
    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());
}

//Slot for when the rotate Horizontally option is selected from the menu.
void SpriteMainWindow::on_actionRotate_Horizontally_triggered()
{
    QTransform tran;
    tran.rotate(90);
    QImage img = workspacePixMap.toImage();
    img = img.transformed(tran);

    //workspacePixMap = QPixmap::fromImage(img).scaled(this->spriteWidth,this->spriteHeight);
    workspacePixMap = QPixmap::fromImage(img).copy();
    ui->workspaceLabel->setPixmap(workspacePixMap);
    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());

}

//Slot for when the rotate Counterclockwise option is selected from the menu.
void SpriteMainWindow::on_actionRotate_Counterclockwise_triggered()
{
    QTransform tran;
    tran.rotate(-90);
    QImage img = workspacePixMap.toImage();
    img = img.transformed(tran);

    //workspacePixMap = QPixmap::fromImage(img).scaled(this->spriteWidth,this->spriteHeight);
    workspacePixMap = QPixmap::fromImage(img).copy();
    ui->workspaceLabel->setPixmap(workspacePixMap);
    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());
}

//Slot for when the Show/Hide option is selected from the menu.
void SpriteMainWindow::on_actionShow_Hide_Frame_triggered()
{
    // Resize previous zoomed frame
    currentFrame->setPixmap(currentFrame->pixmap()->scaled(spriteWidth, spriteHeight));

    if(currentFrame->getIsVisible()){
        currentFrame->toggleVisibility(false);
    }
    else{
        currentFrame->toggleVisibility(true);
    }
}

//Slot for when the Duplicate option is selected from the menu.
void SpriteMainWindow::on_actionDuplicate_triggered()
{
    // Resize previous zoomed frame before switching
    currentFrame->setPixmap(currentFrame->pixmap()->scaled(spriteWidth, spriteHeight));
    this->on_duplicateFrameButton_clicked();
}

//Slot for when the delete option is selected from the menu.
//Remove a frame
void SpriteMainWindow::on_actionDelete_triggered()
{
    //Find the position of the frame to be deleted from the currentSprite's frames vector
    QVector<Frame*> frames = this->currentSprite.getFrames();
    if(frames.length() > 1){
        int frameID = -1;
        for(int i = 0; i < frames.length(); i++){
            if(frames.at(i) == currentFrame){
                frameID = i;
                break;
            }
        }

        if(currentSprite.removeFrame(currentFrame)){

            if (frameID != 0){
            currentFrame = this->currentSprite.getFrames().at(frameID - 1);
            }
            else{
                currentFrame = this->currentSprite.getFrames().first();
            }

            QLayoutItem* child = ui->scrollAreaWidgetContents->layout()->takeAt(frameID);
            delete child->widget();
            delete child;

            workspacePixMap = currentFrame->pixmap()->copy();
            ui->workspaceLabel->setPixmap(workspacePixMap);
            currentFrame->makeFrameActive();
        }
    }
    else{
        //Add a message here that you can't remove a frame if there is only one??
        QMessageBox::StandardButton warning;
        warning = QMessageBox::warning(this, "Warning", "Must have atleast one frame!", QMessageBox::Ok);
    }
}

//Slot for when the remove all option is selected from the menu.
void SpriteMainWindow::on_actionRemove_All_triggered()
{

}

//Slot for when the about option is selected from the menu.
void SpriteMainWindow::on_actionAbout_triggered()
{
    PopupWindow aboutPopup;
    aboutPopup.exec();
}

//Override the closeEvent for the window so that a warning popup may be displayed if there are unsaved changes.
void SpriteMainWindow::closeEvent(QCloseEvent *e){
    if (maybeSave()) {
        e->accept();
    } else {
        e->ignore();
    }
}

//Called when the window wants to close, to determine if there are any necessary changes to save.
bool SpriteMainWindow::maybeSave(){
    if (isModified) {
        isModified = false;
        resetAllFrameSizes();
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Warning"),
                                   tr("The sprite has been modified.\n"
                                      "Do you want to save your changes?"),
                                   QMessageBox::Save | QMessageBox::Discard
                                   | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            //Call the Save method here
            on_actionSave_triggered();
            return true;
        } else if (ret == QMessageBox::Cancel) {
            isModified = true;
            return false;
        }
    }
    return true;
}

//Change Brush Size Slots
void SpriteMainWindow::on_brushSize1Button_clicked()
{
    penWidthSelected = 1;
    pen.setWidth(penWidthSelected);
}

void SpriteMainWindow::on_brushSize2Button_clicked()
{
    penWidthSelected = 10;      //TODO: make them programmatic
    pen.setWidth(penWidthSelected);
}

void SpriteMainWindow::on_brushSize3Button_clicked()
{
    penWidthSelected = 30;
    pen.setWidth(penWidthSelected);
}

void SpriteMainWindow::on_brushSize4Button_clicked()
{
    penWidthSelected = this->spriteWidth / 4;
    pen.setWidth(penWidthSelected);
}

void SpriteMainWindow::on_action2x_Workspace_triggered()
{
    scaleWorkspaceSizeUp();
    update();
}

void SpriteMainWindow::on_actionZoom_Out_triggered()
{
    qDebug() << "In zoom out action";
    scaleWorkspaceSizeDown();
    update();
}

void SpriteMainWindow::on_actionReset_Size_triggered()
{
    qDebug() << "In reset action";
    resetWorkspaceSizeToOriginal();
    update();
}

void SpriteMainWindow::scaleWorkspaceSizeUp() {
    timesScaled++;
    int wspWidth, wspHeight;
    qDebug() << "in scale up";

    //if(timesScaled >= 0) {
        //pen.setWidth(pen.width()+10);
        //scaleFactor = 2;
    //    int wspWidth = workspacePixMap.width() * scaleFactor;
    //    int wspHeight = workspacePixMap.height() * scaleFactor;

        wspWidth = workspacePixMap.width() * 2;
        wspHeight = workspacePixMap.height() * 2;

//        workspacePixMap = workspacePixMap.scaled(wspWidth, wspHeight);
//        ui->workspaceLabel->setPixmap(workspacePixMap);


    //if(timesScaled != 1) {
        workspacePixMap = workspacePixMap.copy().scaled(wspWidth, wspHeight);
        ui->workspaceLabel->setPixmap(workspacePixMap);
    //}

//    update();

//    if(wspWidth > this->width()) {
//        this->resize(this->width() + wspWidth, this->height());
//        if(wspHeight > this->height()) {
//            this->resize(this->width(), this->height() + wspHeight);
//        }
//    }
//    update();
}

void SpriteMainWindow::scaleWorkspaceSizeDown() {
    timesScaled--;
    qDebug() << "in scale down";
    int wspWidth, wspHeight;
    wspWidth = workspacePixMap.width() / 2;
    wspHeight = workspacePixMap.height() / 2;
    workspacePixMap = workspacePixMap.copy().scaled(wspWidth, wspHeight);
    ui->workspaceLabel->setPixmap(workspacePixMap);
}

void SpriteMainWindow::resetWorkspaceSizeToOriginal() {
    timesScaled = 0;
//    int wspWidth = workspacePixMap.width() * scaleFactor;
//    int wspHeight = workspacePixMap.height() * scaleFactor;

//    int wspWidth = this->spriteWidth;
//    int wspHeight = this->spriteHeight;
    qDebug() << "in reset";
    workspacePixMap = currentFrame->pixmap()->copy().scaled(this->spriteWidth, this->spriteHeight);
    ui->workspaceLabel->setPixmap(workspacePixMap);
    //pen.setWidth(penWidthSelected);      //TODO: replace 10 by the current brush size.
}

void SpriteMainWindow::initialResolution(int width, int backColor){
    //See which color the user selected for the background
    if(backColor == 0){
        //Set Transparent Background
        this->backgroundColor = QColor(255,255,255,6);
    }
    else if(backColor == 1){
        //Set White Background
        this->backgroundColor = QColor(255,255,255);
    }
    else{
        //Set Black Background
        this->backgroundColor = QColor(0,0,0);
    }

    this->spriteWidth = width;
    this->spriteHeight = width;

    // TODO: scale the image to worksapce
}

void SpriteMainWindow::frameClicked(Frame* other){
    // Resize previous zoomed frame before switching
    if(other != currentFrame) {
        currentFrame->setPixmap(currentFrame->pixmap()->scaled(spriteWidth, spriteHeight));
    }

    currentFrame->makeFrameUnactive();
    currentFrame = other;
    currentFrame->makeFrameActive();

    qDebug() << "Frame Clicked";

//    workspacePixMap = currentFrame->pixmap()->scaled(this->WORKSPACE_DIMENSION);
    workspacePixMap = currentFrame->pixmap()->copy();
    ui->workspaceLabel->setPixmap(workspacePixMap);

    //If the frame you clicked on is partially cut off because it is at the top or bottom of the scroll area,
    //then the scroll area moves so you see it.
    ui->framesScrollArea->ensureWidgetVisible(currentFrame, 0, 0);

}

void SpriteMainWindow::on_rotateCounterClockButton_clicked()
{
    QTransform tran;
    tran.rotate(-90);
    QImage img = workspacePixMap.toImage();
    img = img.transformed(tran);

    //workspacePixMap = QPixmap::fromImage(img).scaled(this->spriteWidth,this->spriteHeight);
    workspacePixMap = QPixmap::fromImage(img).copy();
    ui->workspaceLabel->setPixmap(workspacePixMap);
    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());
}


void SpriteMainWindow::on_rotateClockwiseButton_clicked()
{
    QTransform tran;
    tran.rotate(90);
    QImage img = workspacePixMap.toImage();
    img = img.transformed(tran);

    //workspacePixMap = QPixmap::fromImage(img).scaled(this->spriteWidth,this->spriteHeight);
    workspacePixMap = QPixmap::fromImage(img).copy();
    ui->workspaceLabel->setPixmap(workspacePixMap);
    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());
}

void SpriteMainWindow::on_flipVerticalButton_clicked()
{
    QImage image = workspacePixMap.toImage().mirrored(false, true);
    workspacePixMap = QPixmap::fromImage(image);
    ui->workspaceLabel->setPixmap(workspacePixMap);

    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());
}

void SpriteMainWindow::on_flipHorizontalButton_clicked()
{
    QImage image = workspacePixMap.toImage().mirrored(true, false);
    workspacePixMap = QPixmap::fromImage(image);
    ui->workspaceLabel->setPixmap(workspacePixMap);

    //currentFrame->setPixmap(workspacePixMap.scaled(spriteWidth, spriteHeight));
    currentFrame->setPixmap(workspacePixMap.copy());
}

void SpriteMainWindow::whenTimerUpdates()
{
    if(currentSprite.getFps() > 0){
        it++;
    }
    if(it >= currentSprite.getFrames().size()){
        it = 0;
    }

    //counts how many non visible frames there are
    int numOfNotVisible = 0;
    //if the current frame is not visible, iterate until there is one visible.
    while (!currentSprite.getFrame(it).getIsVisible()){
        numOfNotVisible++;
        it++;
        if(it >= currentSprite.getFrames().size()){
            it = 0;
        }
        //if all of the frames are not visible, return
        if (numOfNotVisible >= currentSprite.getFrames().size() ){
            return;
        }

    }


    ui->previewLabelMap->setPixmap((currentSprite.getFrame(it).pixmap()->scaled(spriteWidth, spriteHeight)).copy());
    previewWindow.setPixmap((currentSprite.getFrame(it).pixmap()->scaled(spriteWidth, spriteHeight)).copy());
}

void SpriteMainWindow::setFps(int fps){
    ui->fpsSlider->setValue(fps);
    currentSprite.setFps(fps);
}

void SpriteMainWindow::on_duplicateFrameButton_clicked()
{
    Frame* f = new Frame();
    QPixmap map = currentFrame->pixmap()->scaled(spriteWidth,spriteHeight).copy();
    f->setPixmap(map);

    currentFrame->setPixmap(currentFrame->pixmap()->scaled(spriteWidth, spriteHeight));
    currentFrame->makeFrameUnactive();

    currentSprite.appendFrame(f);

    currentFrame = currentSprite.getFrames().last();

    Frame* s =currentSprite.getFrames().last();
    s->setMinimumSize(160,160);
    s->setMaximumSize(160,160);
    s->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    s->setScaledContents(true);
    ui->scrollAreaWidgetContents->layout()->addWidget(s);

    currentFrame->makeFrameActive();

    //workspacePixMap = currentFrame->pixmap()->scaled(this->WORKSPACE_DIMENSION);
    workspacePixMap = map;
    ui->workspaceLabel->setPixmap(workspacePixMap);

    //sets the scrollbar to the bottom
    QScrollBar* verticalScrollBar = ui->framesScrollArea->verticalScrollBar();
    verticalScrollBar->setMaximum(verticalScrollBar->maximum() + 170);
    verticalScrollBar->setSliderPosition(verticalScrollBar->maximum());
}

void SpriteMainWindow::resetAllFrameSizes() {
    QVector<Frame*> frames = currentSprite.getFrames();
    for(int i = 0; i < frames.size(); i++) {
        frames[i]->setPixmap(frames[i]->pixmap()->scaled(spriteWidth, spriteHeight));
    }
}

void SpriteMainWindow::on_transparentGridButton_clicked()
{
    if(transparentGridIsVisible) {
        ui->workspaceLabel->setStyleSheet("#workspaceLabel { background-color: rgb(29,29,29); border: none;}");
        transparentGridIsVisible = false;
    }
    else {
        ui->workspaceLabel->setStyleSheet("#workspaceLabel { background-color: rgb(29,29,29); "
                                          "background-image: url(:transparent pattern 3.png);border: none;}");
        transparentGridIsVisible = true;
    }
}

void SpriteMainWindow::on_actionStamp_triggered(){
    QImage stamp = currentFrame->pixmap()->toImage();
    QString filename = QString("/Users/tylerdailey/Desktop/stamp%1.png").arg(QString::number(numCustomStamps));

    stamp.save(filename, "PNG");
    numCustomStamps++;
    selector->addStamp(filename);
}
