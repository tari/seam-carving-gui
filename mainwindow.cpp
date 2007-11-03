// Copyright (C) 2007  Gabe Rudy
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as 
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Gabe Rudy: gabe@gabeiscoding.com
// http://gabeiscoding.com

#include <QtGui>
#include <QList>
#include <QByteArray>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "mainwindow.h"
#include "resize.h"

// #define DEBUG

/// To get a decent size on the dock widget
class DockWrapper : public QWidget
{
public:
  DockWrapper(QWidget *parent) : QWidget(parent) {}
  QSize sizeHint() const{ return QSize(50,50); }
};

/// To handle mouse events
void ImageScene::mouseMoveEvent(QGraphicsSceneMouseEvent * e )
{
  if(e->buttons() & Qt::LeftButton)
    emit(mouseMoved(e->lastScenePos(), e->scenePos()));
}
void ImageScene::mousePressEvent(QGraphicsSceneMouseEvent * e )
{
  if(e->buttons() & Qt::LeftButton)
    emit(mouseMoved(e->lastScenePos(), e->scenePos()));
}

MainWindow::MainWindow()
{
  //Create an image filter
  _filter = "Images (";
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  for(int i=0; i<formats.size(); i++)
  {
    _filter += QString("*.%1").arg(formats[i].constData());
    if(i != formats.size()-1)
      _filter += " ";
  }
  _filter += ")";
  
  _scene = new ImageScene(this);
  _scene->setBackgroundBrush(palette().dark());
  _imgItem = _scene->addPixmap(QPixmap());
  _view = new QGraphicsView(_scene);
  setCentralWidget(_view);

  createActions();
  createMenus();

  setWindowTitle(tr("Seam Carving GUI"));

  //Dock widget for resizing
  _resizeDock = new QDockWidget(tr("Seam Carving Resize"), this);
  _resizeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  QWidget *holderWidget = new DockWrapper(_resizeDock);
  _resizeWidget.setupUi(holderWidget);
  QIntValidator *validtor = new QIntValidator(1, 100000, holderWidget);
  _resizeWidget.widthLineEdit->setValidator(validtor);
  _resizeWidget.heightLineEdit->setValidator(validtor);
  connect(_resizeWidget.resizeButton, SIGNAL(clicked()), this, SLOT(resizeButtonClicked()));
  connect(_resizeWidget.clearButton, SIGNAL(clicked()), this, SLOT(clearMask()));  
  _resizeDock->setWidget(holderWidget);
  holderWidget->resize(50,50);
  addDockWidget(Qt::RightDockWidgetArea, _resizeDock);
  _viewMenu->addAction(_resizeDock->toggleViewAction());
  _resizeDock->setEnabled(false);

  resize(700, 400);
}

void MainWindow::open()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), _filter);
  if (!fileName.isEmpty()) {
    QImage image(fileName);
    if (image.isNull()) {
      QMessageBox::information(this, tr("Seam Carving GUI"),
                               tr("Cannot load %1.").arg(fileName));
      return;
    }
    _img = image;
    delete _scene;
    _scene = new ImageScene(this);
    connect(_scene, SIGNAL(mouseMoved(QPointF, QPointF)), this, SLOT(paintMask(QPointF, QPointF)));
    _scene->setBackgroundBrush(palette().dark());
    _imgItem = _scene->addPixmap(QPixmap::fromImage(_img));

    _maskPix = QPixmap(_img.width(), _img.height());
    _maskPix.fill(Qt::transparent);
    _maskItem = _scene->addPixmap(_maskPix);

    _view->setScene(_scene);
    
    _scaleFactor = 1.0;

    _printAct->setEnabled(true);
    _saveAct->setEnabled(true);
    _resizeDock->setEnabled(true);
    _copyAct->setEnabled(true);
    _pasteAct->setEnabled(true);
    _resizeWidget.heightLineEdit->setText(QString::number(_img.height()));
    _resizeWidget.widthLineEdit->setText(QString::number(_img.width()));
    updateActions();
  }
}

void MainWindow::save()
{
  QString f = QFileDialog::getSaveFileName(this, "Save Image As...", QDir::currentPath(), _filter);
  if(f.isEmpty())
    return;
  if(!_img.save(f))
    QMessageBox::information(this,"Error Saving",QString("Could not save to file: %1").arg(f));
}

void MainWindow::copy()
{
  QMessageBox::information(this,"Copy", "Copy comming soon!");
}

void MainWindow::paste()
{
  QMessageBox::information(this,"Paste", "Paste comming soon!");
}

void MainWindow::print()
{
  if(_img.isNull())
    return;
  QPrintDialog dialog(&_printer, this);
  if (dialog.exec()) {
    QPainter painter(&_printer);
    QRect rect = painter.viewport();
    QSize size = _img.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
    painter.setWindow(_img.rect());
    painter.drawImage(0, 0, _img);
  }
}

void MainWindow::resizeButtonClicked()
{
  int newWidth = _resizeWidget.widthLineEdit->text().toInt();
  int newHeight = _resizeWidget.heightLineEdit->text().toInt();
  scResize(newWidth, newHeight);
}

void MainWindow::scResize(int newWidth, int newHeight)
{
  //Convert pixmap to image format:
  int width = _img.width();
  int height = _img.height();
  if(width == newWidth && height == newHeight)
    return;
  Image tmp = createImage(width,height);
  unsigned char *data = imageData(tmp);
  Mask msk = createMask(width, height);
  signed char *mskData = maskData(msk);

  QImage mskImg = _maskPix.toImage();

  //Figure out how much we are changing and set up a progress
  int widthAdjustment = (width < newWidth ? newWidth - width : width - newWidth);
  int heightAdjustment = (height < newHeight ? newHeight - height : height - newHeight);
  QProgressDialog p("Resizing", "&Cancel", 0, widthAdjustment + heightAdjustment, this);
  int howMuch; //scratch var
  qApp->processEvents();

  //Copy image data
  for (int y = height - 1; y >= 0; y--) {
    for (int x = 0; x < width; x++) {
      QRgb c = _img.pixel(x,y);
      data[(y * width + x) * 3] = qBlue(c);
      data[(y * width + x) * 3 + 1] = qGreen(c);
      data[(y * width + x) * 3 + 2] = qRed(c);
      QRgb m = mskImg.pixel(x,y);
      if(qGreen(m) == 255)
        mskData[y * width + x] = 1;
      else if(qRed(m) == 255)
        mskData[y * width + x] = -1;
      else
        mskData[y * width + x] = 0;
    }
  }

  //Horizontal scaling
  if(widthAdjustment)
  {
    if(width < newWidth)
    {
      //Expand width
      howMuch = newWidth - width; 
      for(int i=0; i<howMuch; i++)
      {
        tmp = makeWider(tmp,msk);
        p.setValue(p.value()+1);
        qApp->processEvents();
        if(p.wasCanceled())
        {
          destroyImage(tmp);
          destroyMask(msk);
          return;
        }
      }
      width += howMuch;
    }else
    {
      //Reduce width
      howMuch = width - newWidth;
      for(int i=0; i<howMuch; i++)
      {
        tmp = makeNarrower(tmp,msk);
        p.setValue(p.value()+1);
        qApp->processEvents();
        if(p.wasCanceled())
        {
          destroyImage(tmp);
          destroyMask(msk);
          return;
        }
      }
      width -= howMuch;
    }
    data = imageData(tmp); //In case it was reallocated
  }
  if(heightAdjustment)
  {
    //Flipt the image
    Image htmp = createImage(height, width);//backward dimentions
    unsigned char *hdata = imageData(htmp);
    Mask  hmsk = createMask(height, width);
    signed char *hmskData = maskData(hmsk);
    
    for (int y = height - 1; y >= 0; y--) {
      for (int x = 0; x < width; x++) {
        hdata[(x * height + y) * 3] = data[(y * width + x) * 3];
        hdata[(x * height + y) * 3 + 1] = data[(y * width + x) * 3 + 1];
        hdata[(x * height + y) * 3 + 2] = data[(y * width + x) * 3 + 2];
        hmskData[x * height + y] = mskData[y * width + x];
      }
    }
    destroyImage(tmp);
    destroyMask(msk);
#ifdef DEBUG
    saveImage(htmp, "beforeout.bmp");
#endif
    //Now run the algorithm
    if(height < newHeight)
    {
      //Expand width
      howMuch = newHeight - height;
      for(int i=0; i<howMuch; i++)
      {
        htmp = makeWider(htmp,msk);
        p.setValue(p.value()+1);
        qApp->processEvents();
        if(p.wasCanceled())
        {
          destroyImage(htmp);
          destroyMask(hmsk);          
          return;
        }
      }
      height += howMuch;
    }else
    {
      //Reduce width
      howMuch = height - newHeight;
      for(int i=0; i<howMuch; i++)
      {
        htmp = makeNarrower(htmp,hmsk);
        p.setValue(p.value()+1);
        qApp->processEvents();
        if(p.wasCanceled())
        {
          destroyImage(htmp);
          destroyMask(hmsk);                    
          return;
        }
      }
      height -= howMuch;
    }
#ifdef DEBUG
    saveImage(htmp, "afterout.bmp");
#endif    
    //Flip back the image
    hdata = imageData(htmp);
    tmp = createImage(width,height);
    data = imageData(tmp);
    for (int y = height - 1; y >= 0; y--) {
      for (int x = 0; x < width; x++) {
        data[(y * width + x) * 3] = hdata[(x * height + y) * 3];
        data[(y * width + x) * 3 + 1] = hdata[(x * height + y) * 3 + 1];
        data[(y * width + x) * 3 + 2] = hdata[(x * height + y) * 3 + 2];
      }
    }
    destroyImage(htmp);
    destroyMask(hmsk);          
  }
#ifdef DEBUG
  saveImage(tmp, "finalout.bmp");
#endif
  QImage newImg(width,height, QImage::Format_RGB32); //In future, use constructor that takes data pointer
  data = imageData(tmp); //In case it was reallocated
  for (int y = height - 1; y >= 0; y--) {
    for (int x = 0; x < width; x++) {
      newImg.setPixel(x, y, qRgb( data[(y * width + x) * 3 + 2],
                                  data[(y * width + x) * 3 + 1],
                                  data[(y * width + x) * 3] ));
    }
  }  
  destroyImage(tmp);
  destroyMask(msk);
  _img = newImg;
  _imgItem->setPixmap(QPixmap::fromImage(_img));
  clearMask();
  _scaleFactor = 1.0;
}

void MainWindow::clearMask()
{
  _maskPix = QPixmap(_img.width(), _img.height());
  _maskPix.fill(Qt::transparent);
  _maskItem->setPixmap(_maskPix);
}

void MainWindow::paintMask(QPointF oldPos, QPointF newPos)
{
  QPainter painter(&_maskPix);
  QColor penColor = (_resizeWidget.retainRadio->isChecked() ? Qt::green : Qt::red);
  painter.setPen(QPen( QBrush(penColor), _resizeWidget.brushSizeSlider->value()) );
  painter.drawLine(oldPos, newPos);
  _maskItem->setPixmap(_maskPix);
}
void MainWindow::zoomIn()
{
  scaleImage(1.25);
}

void MainWindow::zoomOut()
{
  scaleImage(0.8);
}

void MainWindow::normalSize()
{
  _scaleFactor = 1.0;
  //_view->resetTransform();
  _view->resetMatrix();
}

void MainWindow::about()
{
  QMessageBox::about(this, tr("About Seam Carving GUI"),
                     tr(
                       "<p>The <b>Seam Carving GUI</b> is a simple GUI front end to the "
                       "implementation of the Seam Carving algorithm by Andy Owen "
                       "(<a href=\"http://ultra-premium.com/b\">http://ultra-premium.com/b</a>).</p>"
                       "<p>I ran into Andy's comment to the Slashdot article about Content-Aware "
                       "Image Resizing "
                       "(<a href=\"http://science.slashdot.org/article.pl?sid=07/08/25/1835256\">http://science.slashdot.org/article.pl?sid=07/08/25/1835256</a>) and decided "
                       "his late night hack was cool enough to deserve a easy to use "
                       "interface. Besides, after a few days of seeing the impressive demonstration "
                       "video (<a href=\"http://www.youtube.com/watch?v=vIFCV2spKtg\">http://www.youtube.com/watch?v=vIFCV2spKtg</a>) about the SIGGRAPH "
                       "paper on seam carving (<a href=\"http://www.faculty.idc.ac.il/arik\">http://www.faculty.idc.ac.il/arik</a>) I found myself "
                       "wishing I could be doing seam carving on some images of my own.</p>"
                       "<p>Enjoy, if you have any questions or suffer from the undeniable urge to "
                       "lavishly complement this quick hack of a GUI, you can reach me at "
                       "<a href=\"mailto:gaberudy+seamcarving@gmail.com\">gaberudy+seamcarving@gmail.com</a></p>"
                        ));
}

void MainWindow::createActions()
{
  _openAct = new QAction(tr("&Open..."), this);
  _openAct->setShortcut(tr("Ctrl+O"));
  connect(_openAct, SIGNAL(triggered()), this, SLOT(open()));

  _saveAct = new QAction(tr("&Save..."), this);
  _saveAct->setShortcut(tr("Ctrl+S"));
  _saveAct->setEnabled(false);
  connect(_saveAct, SIGNAL(triggered()), this, SLOT(save()));
    
  _printAct = new QAction(tr("&Print..."), this);
  _printAct->setShortcut(tr("Ctrl+R"));
  _printAct->setEnabled(false);
  connect(_printAct, SIGNAL(triggered()), this, SLOT(print()));

  _exitAct = new QAction(tr("E&xit"), this);
  _exitAct->setShortcut(tr("Ctrl+Q"));
  connect(_exitAct, SIGNAL(triggered()), this, SLOT(close()));

  _copyAct = new QAction(tr("&Copy"), this);
  _copyAct->setShortcut(tr("Ctrl+C"));
  _copyAct->setEnabled(false);
  connect(_copyAct, SIGNAL(triggered()), this, SLOT(copy()));

  _pasteAct = new QAction(tr("&Paste"), this);
  _pasteAct->setShortcut(tr("Ctrl+V"));
  _pasteAct->setEnabled(false);
  connect(_pasteAct, SIGNAL(triggered()), this, SLOT(paste()));
    
  _zoomInAct = new QAction(tr("Zoom &In (25%)"), this);
  QList<QKeySequence> inSc;
  inSc << tr("Ctrl+=") << tr("Ctrl++");
  _zoomInAct->setShortcuts(inSc);
  _zoomInAct->setEnabled(false);
  connect(_zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

  _zoomOutAct = new QAction(tr("Zoom &Out (25%)"), this);
  QList<QKeySequence> outSc;
  outSc << tr("Ctrl+_") << tr("Ctrl+-");
  _zoomOutAct->setShortcuts(outSc);
  _zoomOutAct->setEnabled(false);
  connect(_zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

  _normalSizeAct = new QAction(tr("&Normal Size"), this);
  _normalSizeAct->setShortcut(tr("Ctrl+S"));
  _normalSizeAct->setEnabled(false);
  connect(_normalSizeAct, SIGNAL(triggered()), this, SLOT(normalSize()));

  _aboutAct = new QAction(tr("&About"), this);
  connect(_aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createMenus()
{
  _fileMenu = new QMenu(tr("&File"), this);
  _fileMenu->addAction(_openAct);
  _fileMenu->addAction(_saveAct);    
  _fileMenu->addAction(_printAct);
  _fileMenu->addSeparator();
  _fileMenu->addAction(_exitAct);

  _editMenu = new QMenu(tr("&Edit"), this);
  _editMenu->addAction(_copyAct);
  _editMenu->addAction(_pasteAct);    

  _viewMenu = new QMenu(tr("&View"), this);
  _viewMenu->addAction(_zoomInAct);
  _viewMenu->addAction(_zoomOutAct);
  _viewMenu->addAction(_normalSizeAct);
  _viewMenu->addSeparator();

  _helpMenu = new QMenu(tr("&Help"), this);
  _helpMenu->addAction(_aboutAct);

  menuBar()->addMenu(_fileMenu);
  menuBar()->addMenu(_editMenu);
  menuBar()->addMenu(_viewMenu);
  menuBar()->addMenu(_helpMenu);
}

void MainWindow::updateActions()
{
  _zoomInAct->setEnabled(true);
  _zoomOutAct->setEnabled(true);
  _normalSizeAct->setEnabled(true);
}

void MainWindow::scaleImage(double factor)
{
  adjustScrollBar(_view->horizontalScrollBar(), factor);
  adjustScrollBar(_view->verticalScrollBar(), factor);

  _scaleFactor *= factor;  
  _view->scale(factor, factor);

  _zoomInAct->setEnabled(_scaleFactor < 3.0);
  _zoomOutAct->setEnabled(_scaleFactor > 0.333);
}

void MainWindow::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
  scrollBar->setValue(int(factor * scrollBar->value()
                          + ((factor - 1) * scrollBar->pageStep()/2)));
}
