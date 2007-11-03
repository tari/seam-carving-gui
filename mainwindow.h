#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_resizewidget.h"

#include <QMainWindow>
#include <QPrinter>

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
class QDockWidget;

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
  void zoomIn();
  void zoomOut();
  void normalSize();
  void fitToWindow();
  void about();

private:
  void createActions();
  void createMenus();
  void updateActions();
  void scaleImage(double factor);
  void adjustScrollBar(QScrollBar *scrollBar, double factor);

  QString _filter;
  QLabel *_imageLabel;
  QImage _img;
  QDockWidget *_resizeDock;
  Ui::ResizeWidget _resizeWidget;
  QScrollArea *_scrollArea;
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
  QAction *_fitToWindowAct;
  QAction *_aboutAct;

  QMenu *_fileMenu;
  QMenu *_editMenu;  
  QMenu *_viewMenu;
  QMenu *_helpMenu;
};

#endif
