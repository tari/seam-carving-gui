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
  void undo();
  void repeat();  
  void copy();
  void paste();  
  void print();
  void resizeButtonClicked();
  void removeButtonClicked();
  void cairRemove();
  void cairResize(int newWidth, int newHeight);  
  void clearMask();
  void paintMask(QPointF oldPos, QPointF newPos);
  void zoomIn();
  void zoomOut();
  void normalSize();
  void about();
  void changeView(QAction* view);
  void updateCursor();

private:
  void dragEnterEvent(QDragEnterEvent *event);
  void dropEvent(QDropEvent *event);
  bool eventFilter(QObject *obj, QEvent *event);
  
  void openFile(QString fileName);
  void openImage(QImage image);
  void createActions();
  void createMenus();
  void updateActions();
  void scaleImage(double factor);
  void adjustScrollBar(QScrollBar *scrollBar, double factor);
  void addToUndoStack(QImage img);

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
  QVector<QImage> _undoStack;
  int _undoStackPos;

  QPrinter _printer;

  QAction *_openAct;
  QAction *_saveAct;  
  QAction *_printAct;
  QAction *_exitAct;
  QAction *_undoAct;
  QAction *_repeatAct;
  QAction *_copyAct;
  QAction *_pasteAct;
  QAction *_viewImage;
  QAction *_viewGreyscale;
  QAction *_viewEdge;
  QAction *_viewVEnergy;
  QAction *_viewHEnergy;  
  QActionGroup *_viewGroup; 
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
