#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_resizewidget.h"

#include <QMainWindow>
#include <QPrinter>
#include <QGraphicsScene>

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
class QDockWidget;
class QGraphicsView;
class QGraphicsPixmapItem;

class ImageScene : public QGraphicsScene
{
  Q_OBJECT;
public:
  ImageScene(QObject *parent=0) : QGraphicsScene(parent){}
  void mouseMoveEvent(QGraphicsSceneMouseEvent * e );
  void mousePressEvent(QGraphicsSceneMouseEvent * e );
signals:
  void mouseMoved(QPointF oldPos, QPointF newPos);
};

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
  MainWindow();

private slots:
  void open();
  void save();
  void copy();
  void paste();  
  void print();
  void resizeButtonClicked();
  void scResize(int newWidth, int newHeight);
  void clearMask();
  void paintMask(QPointF oldPos, QPointF newPos);
  void zoomIn();
  void zoomOut();
  void normalSize();
  void about();

private:
  void createActions();
  void createMenus();
  void updateActions();
  void scaleImage(double factor);
  void adjustScrollBar(QScrollBar *scrollBar, double factor);

  QString _filter;
  QImage _img;
  QPixmap _maskPix;
  QDockWidget *_resizeDock;
  Ui::ResizeWidget _resizeWidget;
  ImageScene *_scene;
  QGraphicsView *_view;
  QGraphicsPixmapItem *_imgItem;
  QGraphicsPixmapItem *_maskItem;
  double _scaleFactor;

  QPrinter _printer;

  QAction *_openAct;
  QAction *_saveAct;  
  QAction *_printAct;
  QAction *_exitAct;
  QAction *_copyAct;
  QAction *_pasteAct;
  QAction *_zoomInAct;
  QAction *_zoomOutAct;
  QAction *_normalSizeAct;
  QAction *_aboutAct;

  QMenu *_fileMenu;
  QMenu *_editMenu;  
  QMenu *_viewMenu;
  QMenu *_helpMenu;
};

#endif
