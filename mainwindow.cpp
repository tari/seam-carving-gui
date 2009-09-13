// Copyright (C) 2009  Gabe Rudy
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
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
// Gabe Rudy: gaberudy+seamcarving@gmail.com
// http://code.google.com/p/seam-carving-gui

#include <QtGui>
#include <QList>
#include <QByteArray>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "mainwindow.h"

#include "cair/CAIR_CML.h"
#include "cair/CAIR.h"
#include <vector>

//Decent MAX_ATTEMPTS value
#define MAX_ATTEMPTS 5

QProgressDialog *gProg;
bool updateCallback(float percDone)
{
  qApp->processEvents();
  gProg->setValue((int)(percDone*100));
  if(gProg->wasCanceled())
    return false; //exit out
  return true;
}

//assumes dest is already set to have the save size as source
static void QImagetoCML(QImage source, CML_color &dest)
{
  CML_RGBA p;
  for( int j=0; j<source.height(); j++ )
  {
    for( int i=0; i<source.width(); i++ )
    {
      p.red = qRed( source.pixel(i, j) );
      p.green = qGreen( source.pixel(i, j) );
      p.blue = qBlue( source.pixel(i, j) );
      p.alpha = qAlpha( source.pixel(i, j) );
      dest(i,j) = p;
    }
  }
}

static QImage CMLtoQImage(CML_color &source)
{
  CML_RGBA p;
  QImage newImg = QImage(source.Width(), source.Height(), QImage::Format_RGB32);
  for( int j=0; j<source.Height(); j++ )
  {
    for( int i=0; i<source.Width(); i++ )
    {
      p = source(i,j);
      newImg.setPixel(i, j, qRgba( p.red, p.green, p.blue, p.alpha ));
    }
  }
  return newImg;
}


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
  QGraphicsScene::mouseMoveEvent(e);
}
void ImageScene::mousePressEvent(QGraphicsSceneMouseEvent * e )
{
  if(e->buttons() & Qt::LeftButton)
    emit(mouseMoved(e->lastScenePos(), e->scenePos()));
  QGraphicsScene::mousePressEvent(e);
}

MainWindow::MainWindow()
  : _imgItem(0), _maskItem(0), _undoStackPos(0)
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

  setAcceptDrops(true);
  _view->viewport()->installEventFilter(this);

  createActions();
  createMenus();

  setWindowTitle(tr("Seam Carving GUI"));

  //Dock widget for resizing
  _resizeDock = new QDockWidget(tr("Seam Carving Resize"), this);
  _resizeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  QWidget *holderWidget = new DockWrapper(_resizeDock);
  _resizeWidget.setupUi(holderWidget);
  QIntValidator *validator = new QIntValidator(1, 2000000, holderWidget);
  _resizeWidget.widthLineEdit->setValidator(validator);
  _resizeWidget.heightLineEdit->setValidator(validator);
  _resizeWidget.weightScaleLineEdit->setValidator(validator);
  connect(_resizeWidget.resizeButton, SIGNAL(clicked()), this, SLOT(resizeButtonClicked()));
  connect(_resizeWidget.removeButton, SIGNAL(clicked()), this, SLOT(removeButtonClicked()));
  connect(_resizeWidget.clearButton, SIGNAL(clicked()), this, SLOT(clearMask()));
  connect(_resizeWidget.brushSizeSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateCursor()));
  connect(_resizeWidget.edgeDetector, SIGNAL(activated(int)), this, SLOT(updateView()));
  _resizeDock->setWidget(holderWidget);
  holderWidget->resize(50,50);
  addDockWidget(Qt::RightDockWidgetArea, _resizeDock);
  _viewMenu->addAction(_resizeDock->toggleViewAction());
  _resizeDock->setEnabled(false);

  QString weightScaleToolTip = tr(
    "The maximum possible weight value applied for protection/removal. This\n"
    "value determines what the Brush Weight slider uses for its max value.");
  QString hdToolTip = tr(
    "Enable \"High Definition\" mode. This mode is useful when decreasing the\n"
    "dimensions of an image in both dimensions and produces somewhat better\n"
    "results, but can take nearly twice as long.");
  QString energyToolTip = tr(
    "Use the Forward Energy algorithm. This algorithm greatly increases quality\n"
    "in some situations, but comes at a 5% performance cost.");
  QString iterateToolTip = tr(
    "Under some circumstances the remove algorithm leave some red marked areas\n"
    "on the image, this choice will re-run the remove algorithm five times\n"
    "which should be sufficient to remove any areas marked for removal.");
  QString removeToolTip = tr(
    "Resize the image in one dimension to remove all the areas marked for\n"
    "removal and then resize the image back to its original dimensions.");
    
  _resizeWidget.weightScaleLabel->setToolTip(weightScaleToolTip);
  _resizeWidget.weightScaleLineEdit->setToolTip(weightScaleToolTip);
  _resizeWidget.iterateCheckBox->setToolTip(iterateToolTip);
  _resizeWidget.removeButton->setToolTip(removeToolTip);
  _resizeWidget.hdCheckBox->setToolTip(hdToolTip);
  _resizeWidget.energyCheckBox->setToolTip(energyToolTip);

  resize(700, 400);
}

void MainWindow::open()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), _filter);
  if (!fileName.isEmpty()) {
    openFile(fileName);
  }
}

void MainWindow::openFile(QString fileName)
{
  QImage image(fileName);
  if (image.isNull()) {
    QMessageBox::information(this, tr("Seam Carving GUI"),
                             tr("Cannot load %1.").arg(fileName));
    return;
  }
  openImage(image);
}

void MainWindow::openImage(QImage image, QPixmap mask)
{
  _img = image;
  delete _scene;
    
  _scene = new ImageScene(this);
  connect(_scene, SIGNAL(mouseMoved(QPointF, QPointF)), this, SLOT(paintMask(QPointF, QPointF)));
  _scene->setBackgroundBrush(palette().dark());
  _imgItem = _scene->addPixmap(QPixmap::fromImage(_img));
  _imgItem->setZValue(1);

  if(!mask.isNull())
  {
    _maskPix = mask;
  }else{
    _maskPix = QPixmap(_img.width(), _img.height());
    _maskPix.fill(Qt::transparent);
  }
  _maskItem = _scene->addPixmap(_maskPix);
  _maskItem->setZValue(2); //Always in front of imgItem

  _view->setScene(_scene);
    
  _scaleFactor = 1.0;

  _printAct->setEnabled(true);
  _saveAct->setEnabled(true);
  _openMaskAct->setEnabled(true);
  _saveMaskAct->setEnabled(true);
  _resizeDock->setEnabled(true);
  _copyAct->setEnabled(true);
  _viewImage->setEnabled(true);
  _viewGreyscale->setEnabled(true);
  _viewEdge->setEnabled(true);
  _viewVEnergy->setEnabled(true);
  _viewHEnergy->setEnabled(true);
  _resizeWidget.heightLineEdit->setText(QString::number(_img.height()));
  _resizeWidget.widthLineEdit->setText(QString::number(_img.width()));
  updateActions();
  updateCursor();
}

void MainWindow::save()
{
  QString f = QFileDialog::getSaveFileName(this, "Save Image As...", QDir::currentPath(), _filter);
  if(f.isEmpty())
    return;
  if(!_img.save(f))
    QMessageBox::information(this,"Error Saving",QString("Could not save to file: %1").arg(f));
}

void MainWindow::openMask()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), "Mask File (*.png)");
  if (!fileName.isEmpty()) {
    QImage mskImg(fileName);
    if (mskImg.isNull()) {
      QMessageBox::information(this, tr("Seam Carving GUI"),
                               tr("Cannot load %1.").arg(fileName));
      return;
    }
    if(mskImg.height() != _img.height() || mskImg.width() != _img.width())
    {
      QMessageBox::information(this, tr("Seam Carving GUI"),
                               tr("The mask image does not match the dimensions of the current image"));
      return;
    }
    for( int j=0; j<mskImg.height(); j++ )
    {
      for( int i=0; i<mskImg.width(); i++ )
      {
        QRgb m = mskImg.pixel(i,j);
        if(qGreen(m) > 0)
          mskImg.setPixel( i, j, qRgba(0, qGreen(m), 0, qAlpha(m)) );
        else if(qRed(m) > 0)
          mskImg.setPixel( i, j, qRgba(qRed(m), 0, 0, qAlpha(m)) );
        else
          mskImg.setPixel( i, j, qRgba(0, 0, 0, 0) );
      }
    }
    _maskPix = QPixmap::fromImage(mskImg);
    _maskItem->setPixmap(_maskPix);
  }
}

void MainWindow::saveMask()
{
  QString f = QFileDialog::getSaveFileName(this, "Save Image As...", QDir::currentPath(), "Mask File (*.png)");
  if(f.isEmpty())
    return;
  if(!f.endsWith(".png"))
    f += ".png";
  if(!_maskPix.save(f))
    QMessageBox::information(this,"Error Saving",QString("Could not save to file: %1").arg(f));
}

void MainWindow::undo()
{
  if(_undoStackPos > 0)
  {
    _undoStackPos--;
    openImage(_undoStackImg[_undoStackPos], _undoStackMask[_undoStackPos]);
    _undoAct->setEnabled( _undoStackPos > 0 );
    _repeatAct->setEnabled( _undoStackPos < _undoStackImg.size() );
  }
}

void MainWindow::repeat()
{
  if(_undoStackPos < _undoStackImg.size()-1 )
  {
    _undoStackPos++;
    openImage(_undoStackImg[_undoStackPos], _undoStackMask[_undoStackPos]);
    _undoAct->setEnabled( _undoStackPos > 0 );
    _repeatAct->setEnabled( _undoStackPos < _undoStackImg.size()-1 );
  }
}

void MainWindow::copy()
{
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setImage(_img);
}

void MainWindow::paste()
{
  QClipboard *clipboard = QApplication::clipboard();
  QImage image = clipboard->image();
  if(image.isNull())
    QMessageBox::information(this, tr("Seam Carving GUI"),
                             tr("Cannot paste an image from the clipboard."));
  else
    openImage(image);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if(event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
  if(event->mimeData()->hasUrls())
  {
    QList<QUrl> urls = event->mimeData()->urls();
    if(urls.size() < 1)
      return;
    openFile(urls[0].toLocalFile());
  }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if(event->type() == QEvent::DragEnter) {
    QDragEnterEvent *e = static_cast<QDragEnterEvent *>(event);
    dragEnterEvent(e);
    return true;
  }
  if(event->type() == QEvent::Drop) {
    QDropEvent *e = static_cast<QDropEvent *>(event);
    dropEvent(e);
    return true;
  }
  return QObject::eventFilter(obj,event);
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
  cairResize(newWidth, newHeight);
}

void MainWindow::removeButtonClicked()
{
  cairRemove();
}

void MainWindow::cairRemove()
{
  int width = _img.width();
  int height = _img.height();
  int weight_scale = (int)(_resizeWidget.weightScaleLineEdit->text().toInt() * (_resizeWidget.brushWeightSlider->value() / 100.0));
  int attempts = _resizeWidget.iterateCheckBox->isChecked() ? MAX_ATTEMPTS : 1;
  CAIR_convolution conv = V1;
  switch( _resizeWidget.edgeDetector->currentIndex() )
  {
    case 0 : conv = V1; break;
    case 1 : conv = V_SQUARE; break;
    case 2 : conv = PREWITT; break;
    case 3 : conv = SOBEL; break;
    case 4 : conv = LAPLACIAN; break;
  }

  CAIR_energy ener = BACKWARD;
  if( _resizeWidget.energyCheckBox->isChecked() )
  {
    ener = FORWARD;
  }

  CAIR_direction choice = AUTO;
  if(_resizeWidget.removeMode->currentIndex() == 1)
    choice = VERTICAL;
  else if(_resizeWidget.removeMode->currentIndex() == 2)
    choice = HORIZONTAL;

  //Transfer the image over to cair image format.
  CML_color source(width, height);
  CML_color dest(1,1);
  CML_int source_weights(width, height);
  CML_int dest_weights(1,1);
  QImagetoCML(_img,source);
  QImage mskImg = _maskPix.toImage();
  for( int j=0; j<height; j++ )
  {
    for( int i=0; i<width; i++ )
    {
      QRgb m = mskImg.pixel(i,j);
      if(qGreen(m) > 0)
        source_weights(i,j) = (int)(qGreen(m) * weight_scale);
      else if(qRed(m) > 0)
        source_weights(i,j) = (int)(qRed(m) * -weight_scale);
      else
        source_weights(i,j) = 0;
    }
  }

  int negative_x = 0;
  int negative_y = 0;
  int total_time = 0;

  //count how many negative columns exist
  for( int x = 0; x < source_weights.Width(); x++ )
  {
    for( int y = 0; y < source_weights.Height(); y++ )
    {
      if( source_weights(x,y) < 0 )
      {
        negative_x++;
        break;
      }
    }
  }

  //count how many negative rows exist
  for( int y = 0; y < source_weights.Height(); y++ )
  {
    for( int x = 0; x < source_weights.Width(); x++ )
    {
      if( source_weights(x,y) < 0 )
      {
        negative_y++;
        break;
      }
    }
  }

  switch( choice )
  {
  case AUTO :
    if( negative_x > negative_y )
    {
      total_time = negative_y * 2 * attempts;
    }
    else
    {
      total_time = negative_x * 2 * attempts;
    }
    break;
  case HORIZONTAL :
    total_time = negative_y * 2 * attempts;
    break;
  case VERTICAL :
    total_time = negative_x * 2 * attempts;
    break;
  }

  if( total_time == 0 )
  {
    QMessageBox::information(this, tr("Seam Carving GUI"),
                             tr("Please mark an area for removal first."));
    return;
  }

  QProgressDialog prog("Removing...", "&Cancel", 0, 100, this);
  gProg = &prog;
  qApp->processEvents();

  //Call CAIR
  CAIR_Removal( &source, &source_weights, choice, attempts, conv, ener, &dest_weights, &dest, updateCallback );
  if(prog.wasCanceled())
    return;
  QImage newImg = CMLtoQImage(dest);
  saveInUndoStack(); //Old image
  _img = newImg;
  _imgItem->setPixmap(QPixmap::fromImage(_img));
  //Set the weight mask to the now reduced size version shrunk by CAIR
  QImage maskImg(_img.width(), _img.height(), QImage::Format_ARGB32);
  maskImg.fill(qRgba(0,0,0,0));
  for( int j=0; j<_img.height(); j++ )
  {
    for( int i=0; i<_img.width(); i++ )
    {
      if(dest_weights(i,j) > 0)
        maskImg.setPixel(i, j, qRgba( 0, dest_weights(i,j) / weight_scale, 0, dest_weights(i,j) / weight_scale));
      else if(dest_weights(i,j) < 0)
        maskImg.setPixel(i, j, qRgba( dest_weights(i,j) / -weight_scale, 0, 0, dest_weights(i,j) / -weight_scale));
    }
  }
  _maskPix = QPixmap::fromImage(maskImg);
  _maskItem->setPixmap(_maskPix);
  _scaleFactor = 1.0;
  _resizeWidget.heightLineEdit->setText(QString::number(_img.height()));
  _resizeWidget.widthLineEdit->setText(QString::number(_img.width()));
  addToUndoStack(); //New image
}

void MainWindow::cairResize(int newWidth, int newHeight)
{
  int width = _img.width();
  int height = _img.height();
  int weight_scale = (int)(_resizeWidget.weightScaleLineEdit->text().toInt() * (_resizeWidget.brushWeightSlider->value() / 100.0));
  CAIR_convolution conv = V1;
  switch( _resizeWidget.edgeDetector->currentIndex() )
  {
    case 0 : conv = V1; break;
    case 1 : conv = V_SQUARE; break;
    case 2 : conv = PREWITT; break;
    case 3 : conv = SOBEL; break;
    case 4 : conv = LAPLACIAN; break;
  }

  CAIR_energy ener = BACKWARD;
  if( _resizeWidget.energyCheckBox->isChecked() )
  {
    ener = FORWARD;
  }

  if(newWidth < 1 || newHeight < 1)
  {
    QMessageBox::information(this, tr("Seam Carving GUI"),
                             tr("Invalid dimensions."));
    return;
  }
  if(width == newWidth && height == newHeight)
    return;

  //int widthAdjustment = (width < newWidth ? newWidth - width : width - newWidth);
  //int heightAdjustment = (height < newHeight ? newHeight - height : height - newHeight);
  QProgressDialog prog("Resizing...", "&Cancel", 0, 100, this);
  gProg = &prog;
  qApp->processEvents();
  
  //Transfer the image over to cair image format.
  CML_color source(width, height);
  CML_color dest(1, 1);
  CML_int source_weights(width, height);
  CML_int dest_weights(1,1);
  QImagetoCML(_img,source);
  QImage mskImg = _maskPix.toImage();
  for( int j=0; j<height; j++ )
  {
    for( int i=0; i<width; i++ )
    {
      QRgb m = mskImg.pixel(i,j);
      if(qGreen(m) > 0)
        source_weights(i,j) = (int)(qGreen(m) * weight_scale);
      else if(qRed(m) > 0)
        source_weights(i,j) = (int)(qRed(m) * -weight_scale);
      else
        source_weights(i,j) = 0;
    }
  }
  //Call CAIR
  if( !_resizeWidget.hdCheckBox->isChecked() )
  {
    CAIR( &source, &source_weights, newWidth, newHeight, conv, ener, &dest_weights, &dest, updateCallback );
  }
  else
  {
    CAIR_HD( &source, &source_weights, newWidth, newHeight, conv, ener, &dest_weights, &dest, updateCallback );
  }
  if(prog.wasCanceled())
    return;
  QImage newImg = CMLtoQImage(dest);
  saveInUndoStack();
  _img = newImg;
  _imgItem->setPixmap(QPixmap::fromImage(_img));
  //Set the weight mask to the now reduced size version shrunk by CAIR
  QImage maskImg(_img.width(), _img.height(), QImage::Format_ARGB32);
  maskImg.fill(qRgba(0,0,0,0));
  for( int j=0; j<_img.height(); j++ )
  {
    for( int i=0; i<_img.width(); i++ )
    {
      if(dest_weights(i,j) > 0)
        maskImg.setPixel(i, j, qRgba( 0, dest_weights(i,j) / weight_scale, 0, dest_weights(i,j) / weight_scale));
      else if(dest_weights(i,j) < 0)
        maskImg.setPixel(i, j, qRgba( dest_weights(i,j) / -weight_scale, 0, 0, dest_weights(i,j) / -weight_scale));
    }
  }
  _maskPix = QPixmap::fromImage(maskImg);
  _maskItem->setPixmap(_maskPix);
  _scaleFactor = 1.0;
  addToUndoStack(); //new image
}

void MainWindow::clearMask()
{
  _maskPix = QPixmap(_img.width(), _img.height());
  _maskPix.fill(Qt::transparent);
  _maskItem->setPixmap(_maskPix);
}

void MainWindow::paintMask(QPointF oldPos, QPointF newPos)
{
  //qDebug("paintMaks %f %f %f %f", oldPos.x(), oldPos.y(), newPos.x(), newPos.y());
  QPainter painter(&_maskPix);
  if(_resizeWidget.clearRadio->isChecked())
  {
    painter.setPen(QPen( QBrush(Qt::transparent), _resizeWidget.brushSizeSlider->value() / _scaleFactor, Qt::SolidLine, Qt::RoundCap) );
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
  }else
  {
    QColor penColor(_resizeWidget.retainRadio->isChecked() ? Qt::green : Qt::red);
    penColor.setAlpha( int(255 * (_resizeWidget.brushWeightSlider->value() / 180.0)) );
    painter.setPen(QPen( QBrush(penColor), _resizeWidget.brushSizeSlider->value() / _scaleFactor, Qt::SolidLine, Qt::RoundCap) );
  }
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
"<p>The <b>Seam Carving GUI</b> is a GUI front end to <a href=\"http://brain.recall.googlepages.com/cair\">CAIR</a>, which is an "
"implementation of Arial Shamir's seam carving algorithm.</p>"
""
"<p>I ran into a comment by Andy Owen re the <a href=\"http://science.slashdot.org/article.pl?sid=07/08/25/1835256\">Slashdot article</a> about "
"Content-Aware Image Resizing and decided his late night hack was cool "
"enough to deserve an easy to use interface. Besides, after a few days of "
"seeing the impressive <a href=\"http://www.youtube.com/watch?v=vIFCV2spKtg\">demonstration video</a> about the SIGGRAPH <a href=\"http://www.faculty.idc.ac.il/arik\">paper on "
"seam carving</a> I found myself wishing I could be doing seam carving on "
"some images of my own.</p>"
""
"<p>Thus version 1 and 2 of the <b>Seam Carving GUI</b> came to be. Andy has "
"moved onto other things and I got an email from Brain_Recall (Joe) about "
"his work on writing a more true to the paper form of the algorithm. His "
"stuff looked really quite impressive so I egged him on. Once his code was "
"functionally complete I change the <b>Seam Carving GUI</b> to use CAIR for the "
"backend. Not only does his code work much better for stretching images "
"(and in general), but it's also multi-threaded and has a couple cool new "
"features. So with the new backend, the Seam Carving GUI reaches version "
"3.</p>"
""
"<p>Enjoy! If you have questions, complaints, or have a cool project you used "
"this on, feel free to try and reach me at <a href=\"mailto:gaberudy+seamcarving@gmail.com\">gaberudy+seamcarving@gmail.com</a>, "
"and if I haven't completely abandoned this project I may get back to you!</p>"
""
"<p>For more information and updates goto: <a href=\"http://code.google.com/p/seam-carving-gui/\">http://code.google.com/p/seam-carving-gui/</a></p>"
                        ));
}

void MainWindow::changeView(QAction *view)
{
  if(view == _viewImage)
  {
    _imgItem->setPixmap(QPixmap::fromImage(_img));
    return;
  }
  CML_color source(_img.width(), _img.height());
  CML_color dest(_img.width(), _img.height());
  QImagetoCML(_img,source);
  CAIR_convolution conv = V1;
  switch( _resizeWidget.edgeDetector->currentIndex() )
  {
    case 0 : conv = V1; break;
    case 1 : conv = V_SQUARE; break;
    case 2 : conv = PREWITT; break;
    case 3 : conv = SOBEL; break;
    case 4 : conv = LAPLACIAN; break;
  }
  CAIR_energy ener = BACKWARD;
  if( _resizeWidget.energyCheckBox->isChecked() )
  {
    ener = FORWARD;
  }
  if(view == _viewGreyscale)
    CAIR_Grayscale( &source, &dest );
  if(view == _viewEdge)
    CAIR_Edge( &source, conv, &dest );
  if(view == _viewVEnergy)
    CAIR_V_Energy( &source, conv, ener, &dest );
  if(view == _viewHEnergy)
    CAIR_H_Energy( &source, conv, ener, &dest );
  _imgItem->setPixmap(QPixmap::fromImage(CMLtoQImage(dest)));
}

void MainWindow::updateView()
{
  changeView(_viewGroup->checkedAction());
}

void MainWindow::updateCursor()
{
  if(!_imgItem) return;
  int size = _resizeWidget.brushSizeSlider->value();
  QPixmap pix(size+1,size+1);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.drawEllipse( 0, 0, size, size);
  _imgItem->setCursor( QCursor(pix) );
}

void MainWindow::createActions()
{
  _openAct = new QAction(tr("&Open..."), this);
  _openAct->setShortcut(QKeySequence::Open);
  connect(_openAct, SIGNAL(triggered()), this, SLOT(open()));

  _saveAct = new QAction(tr("&Save..."), this);
  _saveAct->setShortcut(QKeySequence::Save);
  _saveAct->setEnabled(false);
  connect(_saveAct, SIGNAL(triggered()), this, SLOT(save()));

  _openMaskAct = new QAction(tr("&Open Mask..."), this);
  _openMaskAct->setEnabled(false);
  connect(_openMaskAct, SIGNAL(triggered()), this, SLOT(openMask()));

  _saveMaskAct = new QAction(tr("&Save Mask..."), this);
  _saveMaskAct->setEnabled(false);
  connect(_saveMaskAct, SIGNAL(triggered()), this, SLOT(saveMask()));
  
  _printAct = new QAction(tr("&Print..."), this);
  _printAct->setShortcut(QKeySequence::Print);
  _printAct->setEnabled(false);
  connect(_printAct, SIGNAL(triggered()), this, SLOT(print()));

  _exitAct = new QAction(tr("E&xit"), this);
  _exitAct->setShortcut(tr("Ctrl+Q"));
  connect(_exitAct, SIGNAL(triggered()), this, SLOT(close()));

  _undoAct = new QAction(tr("&Undo"), this);
  _undoAct->setShortcut(QKeySequence::Undo);
  _undoAct->setEnabled(false);
  connect(_undoAct, SIGNAL(triggered()), this, SLOT(undo()));

  _repeatAct = new QAction(tr("&Redo"), this);
  _repeatAct->setShortcut(QKeySequence::Redo);
  _repeatAct->setEnabled(false);
  connect(_repeatAct, SIGNAL(triggered()), this, SLOT(repeat()));
  
  _copyAct = new QAction(tr("&Copy"), this);
  _copyAct->setShortcut(QKeySequence::Copy);
  _copyAct->setEnabled(false);
  connect(_copyAct, SIGNAL(triggered()), this, SLOT(copy()));

  _pasteAct = new QAction(tr("&Paste"), this);
  _pasteAct->setShortcut(QKeySequence::Paste);
  connect(_pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

  _viewImage = new QAction(tr("View Image"), this);
  _viewImage->setShortcut(tr("Ctrl+I"));
  _viewImage->setCheckable(true);
  _viewImage->setEnabled(false);
  _viewGreyscale = new QAction(tr("View Greyscale"), this);
  _viewGreyscale->setShortcut(tr("Ctrl+G"));
  _viewGreyscale->setCheckable(true);
  _viewGreyscale->setEnabled(false);
  _viewEdge = new QAction(tr("View Edge"), this);
  _viewEdge->setShortcut(tr("Ctrl+E"));
  _viewEdge->setCheckable(true);
  _viewEdge->setEnabled(false);
  _viewVEnergy = new QAction(tr("View Vertical Energy"), this);
  _viewVEnergy->setShortcut(tr("Ctrl+N"));
  _viewVEnergy->setCheckable(true);
  _viewVEnergy->setEnabled(false);
  _viewHEnergy = new QAction(tr("View Horizontal Energy"), this);
  _viewHEnergy->setShortcut(tr("Ctrl+H"));
  _viewHEnergy->setCheckable(true);
  _viewHEnergy->setEnabled(false);
  _viewGroup = new QActionGroup(this);
  _viewGroup->addAction(_viewImage);
  _viewGroup->addAction(_viewGreyscale);
  _viewGroup->addAction(_viewEdge);
  _viewGroup->addAction(_viewVEnergy);
  _viewGroup->addAction(_viewHEnergy);
  connect(_viewGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeView(QAction*)));
  _viewImage->setChecked(true);
  
  _zoomInAct = new QAction(tr("Zoom &In (25%)"), this);
  _zoomInAct->setShortcut(QKeySequence::ZoomIn);
  _zoomInAct->setEnabled(false);
  connect(_zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

  _zoomOutAct = new QAction(tr("Zoom &Out (25%)"), this);
  _zoomOutAct->setShortcut(QKeySequence::ZoomOut);
  _zoomOutAct->setEnabled(false);
  connect(_zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

  _normalSizeAct = new QAction(tr("&Normal Size"), this);
  _normalSizeAct->setShortcut(tr("Ctrl+0"));
  _normalSizeAct->setEnabled(false);
  connect(_normalSizeAct, SIGNAL(triggered()), this, SLOT(normalSize()));

  _aboutAct = new QAction(tr("&About Seam Carving Gui"), this);
  _aboutAct->setShortcut(QKeySequence::HelpContents);
  connect(_aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createMenus()
{
  _fileMenu = new QMenu(tr("&File"), this);
  _fileMenu->addAction(_openAct);
  _fileMenu->addAction(_saveAct);
  _fileMenu->addAction(_openMaskAct);
  _fileMenu->addAction(_saveMaskAct);
  _fileMenu->addAction(_printAct);
  _fileMenu->addSeparator();
  _fileMenu->addAction(_exitAct);

  _editMenu = new QMenu(tr("&Edit"), this);
  _editMenu->addAction(_undoAct);
  _editMenu->addAction(_repeatAct);
  _editMenu->addSeparator();
  _editMenu->addAction(_copyAct);
  _editMenu->addAction(_pasteAct);

  _viewMenu = new QMenu(tr("&View"), this);
  _viewMenu->addAction(_viewImage);
  _viewMenu->addAction(_viewGreyscale);
  _viewMenu->addAction(_viewEdge);
  _viewMenu->addAction(_viewVEnergy);
  _viewMenu->addAction(_viewHEnergy);
  _viewMenu->addSeparator();
  _viewMenu->addAction(_zoomInAct);
  _viewMenu->addAction(_zoomOutAct);
  _viewMenu->addAction(_normalSizeAct);
  _viewMenu->addSeparator();

  _helpMenu = new QMenu(tr("&Help"), this);
  _helpMenu->addAction("About Qt", qApp, SLOT(aboutQt()));
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

void MainWindow::saveInUndoStack()
{
  if(_undoStackImg.size() < _undoStackPos+1 )
  {
    _undoStackImg.resize(_undoStackPos+1);
    _undoStackMask.resize(_undoStackPos+1);
  }
  _undoStackImg[_undoStackPos] = _img;
  _undoStackMask[_undoStackPos] = _maskPix;
  _undoAct->setEnabled( _undoStackPos > 0 );
  _repeatAct->setEnabled( _undoStackPos < _undoStackImg.size()-1 );
}

void MainWindow::addToUndoStack()
{
  _undoStackPos++;
  if(_undoStackImg.size() < _undoStackPos+1 )
  {
    _undoStackImg.resize(_undoStackPos+1);
    _undoStackMask.resize(_undoStackPos+1);
  }
  _undoStackImg[_undoStackPos] = _img;
  _undoStackMask[_undoStackPos] = _maskPix;
  _undoAct->setEnabled( _undoStackPos > 0 );
  _repeatAct->setEnabled( _undoStackPos < _undoStackImg.size()-1 );
}
