#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QApplication>
#include <QButtonGroup>
#include <QChar>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QStringList>
#include <QStyle>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTime>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUndoCommand>
#include <QtCore/qmath.h>
#include <QtCore>

#include <cstring>
#include <iostream>
#include <fstream>
#include <set>
#include <sstream>
using namespace std;


#include "HapnetWindow.h"
#include "ColourDialog.h"
#include "HapAppError.h"
#include "MoveCommand.h"
#include "GeoTrait.h"
#include "XPM.h"
#include "Edge.h"
#include "EdgeItem.h"
#include "HapNet.h"
#include "MinSpanNet.h"
#include "MedJoinNet.h"
#include "ParsimonyNet.h"
#include "IntNJ.h"
#include "TCS.h"
#include "TightSpanWalker.h"
#include "NetworkError.h"
#include "SeqParser.h"
#include "NexusParser.h"
#include "SeqParseError.h"
#include "SequenceError.h"
#include "TreeError.h"

// TODO Features to add for user input:
// AMP: add user cutoff for how frequently an ancestral sequence is sampled
// TODO use QMessageBox instead of QErrorMessage
//   this would allow exception details to be written to "detailedText"

HapnetWindow::HapnetWindow(QWidget *parent, Qt::WindowFlags flags)
  : QMainWindow(parent, flags), _debug(true)
{
  _clipboard = QApplication::clipboard();
  _history = new QUndoStack(this);
  _netThread = new QThread(this);
  connect(_netThread, SIGNAL(finished()), this, SLOT(displayNetwork()));


  _statThread = new QThread(this);

  //_drawThread = new QThread(this);
  //connect(_drawThread, SIGNAL(finished()), this, SLOT(finaliseDisplay()));
  //connect(_drawThread, SIGNAL(started()), this, SLOT(setModel()));

  _progress = new QProgressDialog(this);
  _progress->setMinimum(0);
  _progress->setMaximum(100);
  QPushButton *cancelButton = new QPushButton("Can't Cancel", _progress);
  cancelButton->setEnabled(false);
  _progress->setCancelButton(cancelButton);

  int seed = QDateTime::currentDateTime().toTime_t();
  qsrand(seed);
  QStatusBar *sbar = statusBar();
  
  resize(800, 600);
  
  _centralContainer = new QStackedWidget(this);
  setCentralWidget(_centralContainer);

  _netModel = 0;
  _netView = new NetworkView(this);
  _netView->resize(width(), height() * 2/3);
  _view = Net;
  //setCentralWidget(_netView);
  _centralContainer->addWidget(_netView);
  connect(_netView, SIGNAL(itemsMoved(QList<QPair<QGraphicsItem *, QPointF> >)), this, SLOT(graphicsMove(QList<QPair<QGraphicsItem *, QPointF> > )));
  connect(_netView, SIGNAL(legendItemClicked(int)), this, SLOT(changeColour(int)));
  connect(_netView, SIGNAL(networkDrawn()), this, SLOT(finaliseDisplay()));
  //connect(_netView, SIGNAL(progressUpdated(int)), _progress, SLOT(setValue(int)));
  //connect(_netView, SIGNAL(caughtException(const QString &)), this, SLOT(showNetError(const QString &)));

  /*_messageConsole = new QPlainTextEdit(this);
  _messageConsole->setTextInteractionFlags(Qt::TextSelectableByMouse);
  _messageConsole->resize(400, 200);
  _messageConsole->setMinimumHeight(0);*/

   _mapView = new MapView(this);
   //_mapView->setVisible(false);
   _centralContainer->addWidget(_mapView);
   _mapTraitsSet = false;
   connect(_mapView, SIGNAL(positionChanged(const QString &)), sbar, SLOT(showMessage(const QString &)));
   connect(_mapView, SIGNAL(seqColourChangeRequested(int)), this, SLOT(changeColour(int)));

  _stats = 0;
  _alModel = 0;
  _alView = new AlignmentView(this);
  _tModel = 0;
  _tView = new TraitView(this);
  _tView->setSelectionBehavior(QAbstractItemView::SelectRows);
  _tView->setSelectionMode(QAbstractItemView::ContiguousSelection);
  _dataWidget = new QTabWidget(this);
  _dataWidget->addTab(_tView, "Traits");
  _dataWidget->addTab(_alView, "Alignment");

  QDockWidget *dockWidget = new QDockWidget("Data Viewer", this);
  dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetMovable);
  dockWidget->setAllowedAreas(Qt::BottomDockWidgetArea|Qt::LeftDockWidgetArea);
  dockWidget->setWidget(_dataWidget);//new QLabel("Placeholder", this));
  addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
  
  
  setupActions();
  setupMenus();
  setupTools();
  toggleNetActions(false);
  toggleAlignmentActions(false);

  setWindowTitle("PopART");
  setWindowIcon(QIcon(QPixmap(xpm::popart2)));
  
  // TODO add console
  //_networkArea->writeToConsole("Console text here.");
  _g = 0;
  
  //_messageConsole->setPlainText("Message output here.");
}

HapnetWindow::~HapnetWindow()
{
  // unnecessary? window's destruction takes care of this
  /*delete _networkArea;
  delete _messageConsole;*/
  
  /*if (_netThread->isRunning())
  {
    _netThread->terminate();
    
    cout << "starting timer..." << endl;
    sleep(2);    
    
    cout << "still running? " << _netThread->isRunning() << endl;
    //_netThread->wait();
  }*/
  
  closeTraits();
  closeAlignment();
}

void HapnetWindow::setupActions()
{
  
  _openAct = new QAction(tr("&Open..."), this);
  _openAct->setShortcut(QKeySequence::Open);
  _openAct->setStatusTip(tr("Open an existing alignment"));
  connect(_openAct, SIGNAL(triggered()), this, SLOT(openAlignment()));
  
  _closeAct = new QAction(tr("&Close"), this);
  _closeAct->setShortcut(QKeySequence::Close);
  _closeAct->setStatusTip(tr("Close current alignment"));
  connect(_closeAct, SIGNAL(triggered()), this, SLOT(closeAlignment()));
  connect(_closeAct, SIGNAL(triggered()), this, SLOT(closeTraits()));

  _importAlignAct = new QAction(tr("&Alignment"), this);
  _importAlignAct->setStatusTip(tr("Import Phylip-format alignment"));
  connect(_importAlignAct, SIGNAL(triggered()), this, SLOT(importAlignment()));

  _importTraitsAct = new QAction(tr("&Traits"), this);
  _importTraitsAct->setStatusTip(tr("Import traits from table"));
  connect(_importTraitsAct, SIGNAL(triggered()), this, SLOT(importTraits()));

  _importGeoTagsAct = new QAction(tr("&Geo Tags"), this);
  _importGeoTagsAct->setStatusTip(tr("Import geo tags from table"));
  connect(_importGeoTagsAct, SIGNAL(triggered()), this, SLOT(importGeoTags()));

  _saveGraphicsAct = new QAction(tr("Export &graphics"), this);
  _saveGraphicsAct->setStatusTip(tr("Export network as an image"));
  _saveGraphicsAct->setEnabled(false);
  connect(_saveGraphicsAct, SIGNAL(triggered()), this, SLOT(saveGraphics()));
  
  _exportAct = new QAction(tr("E&xport network as table"), this);
  _exportAct->setStatusTip(tr("Export network as a tab-delimited table"));
  _exportAct->setEnabled(false);
  connect(_exportAct, SIGNAL(triggered()), this, SLOT(exportNetwork()));
  
  _quitAct = new QAction(tr("&Quit"), this);
  _quitAct->setShortcut(QKeySequence::Quit);
  _quitAct->setStatusTip(tr("Quit HapApp"));
  connect(_quitAct, SIGNAL(triggered()), this, SLOT(quit()));
  
  _undoAct = _history->createUndoAction(this, tr("&Undo"));
  _undoAct->setShortcuts(QKeySequence::Undo);
  
  _redoAct = _history->createRedoAction(this, tr("&Redo"));
  _redoAct->setShortcuts(QKeySequence::Redo);
  
  _traitColourAct = new QAction(tr("Set trait &colour"), this);
  _traitColourAct->setStatusTip(tr("Change a colour associated with a trait"));
  _traitColourAct->setEnabled(false);
  connect(_traitColourAct, SIGNAL(triggered()), this, SLOT(setTraitColour()));
  
  _vertexColourAct = new QAction(tr("Set default vertex &colour"), this);
  _vertexColourAct->setStatusTip(tr("Change the default vertex colour"));
  _vertexColourAct->setEnabled(false);
  connect(_vertexColourAct, SIGNAL(triggered()), this, SLOT(changeVertexColour()));

  _vertexSizeAct = new QAction(tr("Set default vertex &size"), this);
  _vertexSizeAct->setStatusTip(tr("Change the default vertex size"));
  _vertexSizeAct->setEnabled(false);
  connect(_vertexSizeAct, SIGNAL(triggered()), this, SLOT(changeVertexSize()));

  _edgeColourAct = new QAction(tr("Set edge &colour"), this);
  _edgeColourAct->setStatusTip(tr("Change edge colour"));
  _edgeColourAct->setEnabled(false);
  connect(_edgeColourAct, SIGNAL(triggered()), this, SLOT(changeEdgeColour()));
  
  _backgroundColourAct = new QAction(tr("Set background &colour"), this);
  _backgroundColourAct->setStatusTip(tr("Change network background colour"));
  _backgroundColourAct->setEnabled(false);
  connect(_backgroundColourAct, SIGNAL(triggered()), this, SLOT(changeBackgroundColour()));

  _legendFontAct = new QAction(tr("Set &legend font"), this);
  _legendFontAct->setStatusTip(tr("Change font used in legend"));
  _legendFontAct->setEnabled(false);
  connect(_legendFontAct, SIGNAL(triggered()), this, SLOT(changeLegendFont()));

  _labelFontAct = new QAction(tr("Set &label font"), this);
  _labelFontAct->setStatusTip(tr("Change font used for node labels"));
  _labelFontAct->setEnabled(false);
  connect(_labelFontAct, SIGNAL(triggered()), this, SLOT(changeLabelFont()));
  
  _redrawAct = new QAction(tr("Re&draw network"), this);
  _redrawAct->setStatusTip(tr("Redraw the current network"));
  _redrawAct->setEnabled(false);
  connect(_redrawAct, SIGNAL(triggered()), this, SLOT(redrawNetwork()));//_netView, SLOT(redraw()));
  
  _msnAct = new QAction(tr("&Minimum Spanning Network"), this);
  _msnAct->setStatusTip(tr("Build minimum spanning network"));
  connect(_msnAct, SIGNAL(triggered()), this, SLOT(buildMSN()));
  
  _mjnAct = new QAction(tr("Median &Joining Network"), this);
  _mjnAct->setStatusTip(tr("Build median joining network"));
  connect(_mjnAct, SIGNAL(triggered()), this, SLOT(buildMJN()));
  
  _apnAct = new QAction(tr("Ancestral M&P Network"), this);
  _apnAct->setStatusTip(tr("Build ancestral maximum parsimony network"));
  connect(_apnAct, SIGNAL(triggered()), this, SLOT(buildAPN()));
  
  _injAct = new QAction(tr("&Integer NJ Net"), this);
  _injAct->setStatusTip(tr("Build integer neightbour-joining network"));
  connect(_injAct, SIGNAL(triggered()), this, SLOT(buildIntNJ()));

  _tswAct = new QAction(tr("Tight Span &Walker"), this);
  _tswAct->setStatusTip(tr("Build Tight Span Walker network"));
  connect(_tswAct, SIGNAL(triggered()), this, SLOT(buildTSW()));
  
  _tcsAct = new QAction(tr("&TCS Network"), this);
  _tcsAct->setStatusTip(tr("Build TCS network"));
  connect(_tcsAct, SIGNAL(triggered()), this, SLOT(buildTCS()));

  /*_umpAct = new QAction(tr("UMP Network"), this);
  _umpAct->setShortcut(tr("Ctrl+U"));
  _umpAct->setStatusTip(tr("Build UMP network"));
  connect(_umpAct, SIGNAL(triggered()), this, SLOT(buildUMP()));*/

  _toggleViewAct = new QAction(tr("Switch to map &view"), this);
  _toggleViewAct->setStatusTip(tr("Toggle between network and map view"));
  connect(_toggleViewAct, SIGNAL(triggered()), this, SLOT(toggleView()));

  QActionGroup *viewActions = new QActionGroup(this);
  _dashViewAct = new QAction(tr("Show &hatch marks"), viewActions);
  _dashViewAct->setStatusTip(tr("Show mutations as hatch marks along edges"));
  _dashViewAct->setCheckable(true);
  _dashViewAct->setChecked(true);
  _nodeViewAct = new QAction(tr("Show &1-step edges"), viewActions);
  _nodeViewAct->setStatusTip(tr("Show single-mutation edges with intermediate vertices"));
  _nodeViewAct->setCheckable(true);
  _numViewAct = new QAction(tr("Show mutation &count"), viewActions);
  _numViewAct->setStatusTip(tr("Show numbers of mutations along edges"));
  _numViewAct->setCheckable(true);
  connect(viewActions, SIGNAL(triggered(QAction*)), this, SLOT(changeEdgeMutationView(QAction*)));
  
  _identicalAct = new QAction(tr("&Identical sequences"), this);
  _identicalAct->setStatusTip(tr("Show identical sequences"));
  connect(_identicalAct, SIGNAL(triggered()), this, SLOT(showIdenticalSeqs()));
  
  _ntDiversityAct = new QAction(tr("Nucleotide &diversity"), this);
  _ntDiversityAct->setStatusTip(tr("Compute nucleotide diversity"));
  connect(_ntDiversityAct, SIGNAL(triggered()), this, SLOT(showNucleotideDiversity()));
  
  _nSegSitesAct = new QAction(tr("&Segregating sites"), this);
  _nSegSitesAct->setStatusTip(tr("Count segregating sites"));
  connect(_nSegSitesAct, SIGNAL(triggered()), this, SLOT(showSegSites()));
  
  _nParsimonyAct = new QAction(tr("&Parsimony-informative sites"), this);
  _nParsimonyAct->setStatusTip(tr("Count parsimony-informative sites"));
  connect(_nParsimonyAct, SIGNAL(triggered()), this, SLOT(showParsimonySites()));
  
  _tajimaAct = new QAction(tr("&Tajima's D"), this);
  _tajimaAct->setStatusTip(tr("Compute Tajima's D statsitic"));
  connect(_tajimaAct, SIGNAL(triggered()), this, SLOT(showTajimaD()));
  
  _allStatsAct = new QAction(tr("&All stats"), this);
  _allStatsAct->setStatusTip(tr("Compute all statistics"));
  connect(_allStatsAct, SIGNAL(triggered()), this, SLOT(showAllStats()));
  
  _colourAct = new QAction(QIcon(QPixmap(xpm::colourTheme)), tr("Change colour theme"), this);
  _colourAct->setStatusTip(tr("Change colour theme"));
  connect(_colourAct, SIGNAL(triggered()), this, SLOT(changeColourTheme()));
  
   _zoomInAct = new QAction(QIcon(QPixmap(xpm::zoomin)), tr("Zoom in"), this);
  _zoomInAct->setEnabled(false);
  connect(_zoomInAct, SIGNAL(triggered()), _netView, SLOT(zoomIn()));
  
  _zoomOutAct = new QAction(QIcon(QPixmap(xpm::zoomout)), tr("Zoom out"), this);
  _zoomOutAct->setEnabled(false);
  connect(_zoomOutAct, SIGNAL(triggered()), _netView, SLOT(zoomOut()));
 
  _rotateLAct = new QAction(QIcon(QPixmap(xpm::rotateL)), tr("Rotate left"), this);
  _rotateLAct->setEnabled(false);
  connect(_rotateLAct, SIGNAL(triggered()), _netView, SLOT(rotateL()));

  _rotateRAct = new QAction(QIcon(QPixmap(xpm::rotateR)), tr("Rotate right"), this);
  _rotateRAct->setEnabled(false);
  connect(_rotateRAct, SIGNAL(triggered()), _netView, SLOT(rotateR()));
  
  _searchAct = new QAction(QIcon(QPixmap(xpm::search)), tr("Search for a node"), this);
  _searchAct->setShortcut(QKeySequence::Find);//tr("Ctrl+F"));
  _searchAct->setEnabled(false);
  connect(_searchAct, SIGNAL(triggered()), this, SLOT(search()));

  _barchartAct = new QAction(QIcon(QPixmap(xpm::barchart)), tr("Toggle show barcharts"), this);
  _barchartAct->setEnabled(false);
  _barchartAct->setCheckable(true);
  connect(_barchartAct, SIGNAL(toggled(bool)), _netView, SLOT(toggleShowBarcharts(bool)));
  connect(_barchartAct,  SIGNAL(toggled(bool)), this, SLOT(fixTaxBoxButton(bool)));
  
  _taxBoxAct = new QAction(QIcon(QPixmap(xpm::taxbox)), tr("Toggle show identical taxa"), this);
  _taxBoxAct->setEnabled(false);
  _taxBoxAct->setCheckable(true);
  connect(_taxBoxAct, SIGNAL(toggled(bool)), _netView, SLOT(toggleShowTaxBox(bool)));
  connect(_taxBoxAct, SIGNAL(toggled(bool)), this, SLOT(fixBarchartButton(bool)));
}

void HapnetWindow::setupMenus()
{  
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(_openAct);
  fileMenu->addAction(_closeAct);
  fileMenu->addSeparator();
  
  QMenu *importMenu = fileMenu->addMenu(tr("Import..."));
  importMenu->addAction(_importAlignAct);
  importMenu->addAction(_importTraitsAct);
  importMenu->addAction(_importGeoTagsAct);

  fileMenu->addAction(_exportAct);
  fileMenu->addAction(_saveGraphicsAct);
  fileMenu->addSeparator();

  fileMenu->addAction(_quitAct);
  
  QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    
  editMenu->addAction(_undoAct);
  editMenu->addAction(_redoAct);
  editMenu->addSeparator();

  QAction *dataEditAction = new QAction("Data editing disabled", this);
  dataEditAction->setEnabled(false);
  editMenu->addAction(dataEditAction);
  editMenu->addSeparator();
  editMenu->addAction(_traitColourAct);
  editMenu->addAction(_vertexColourAct);
  editMenu->addAction(_vertexSizeAct);
  editMenu->addAction(_edgeColourAct);
  editMenu->addAction(_backgroundColourAct);
  editMenu->addSeparator();
  editMenu->addAction(_labelFontAct);
  editMenu->addAction(_legendFontAct);
  editMenu->addSeparator();
  editMenu->addAction(_redrawAct);
  
  
  _networkMenu = menuBar()->addMenu(tr("&Network"));
  _networkMenu->addAction(_msnAct);
  _networkMenu->addAction(_mjnAct);
  _networkMenu->addAction(_apnAct);
  _networkMenu->addAction(_injAct);
  _networkMenu->addAction(_tswAct);
  _networkMenu->addAction(_tcsAct);
  //_networkMenu->addAction(_umpAct);
  //_networkMenu->setEnabled(false);
  
  _viewMenu = menuBar()->addMenu(tr("&View"));
  _viewMenu->addAction(_toggleViewAct);

  _viewMenu->addSeparator();//->setText(tr("Network"));
  // Add maps: make a group for network vs. map view, and a setSeparator(true) for both, maybe
  // Disable mutation view options when map view selected
  _viewMenu->addAction(_dashViewAct);
  _viewMenu->addAction(_nodeViewAct);
  _viewMenu->addAction(_numViewAct);



  _statsMenu =  menuBar()->addMenu(tr("&Statistics"));
  _statsMenu->addAction(_identicalAct);
  _statsMenu->addSeparator();
  _statsMenu->addAction(_ntDiversityAct);
  _statsMenu->addAction(_nSegSitesAct);
  _statsMenu->addAction(_nParsimonyAct);
  _statsMenu->addAction(_tajimaAct);
  _statsMenu->addAction(_allStatsAct);
  
  QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->setEnabled(false);

}

void HapnetWindow::setupTools()
{
  QToolBar *toolbar = addToolBar(tr("Toolbar"));
  //toolbar->addAction(tr("Tools here"));
  QStyle *toolstyle = toolbar->style();
  
  _nexusToolAct = toolbar->addAction(QIcon(QPixmap(xpm::nexus)), "Open Nexus file");
  connect(_nexusToolAct, SIGNAL(triggered()), this, SLOT(openAlignment()));
  
  _exportToolAct = toolbar->addAction(toolstyle->standardIcon(QStyle::SP_DialogSaveButton), "Export network");
  connect(_exportToolAct, SIGNAL(triggered()), this, SLOT(exportNetwork()));
  _exportToolAct->setEnabled(false);
  
  toolbar->addSeparator();
  
  QSlider *timeSlider = new QSlider(Qt::Horizontal, toolbar);
  timeSlider->setMinimum(-50);
  timeSlider->setMaximum(0);
  timeSlider->setTickInterval(10);
  
  timeSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  QLabel *timeLabel = new QLabel(tr("Time (kYA)"), toolbar);
  
  timeSlider->setEnabled(false);
  timeLabel->setEnabled(false);
  timeSlider->setVisible(false);
  timeLabel->setVisible(false);
  //connect(timeSlider, SIGNAL(isEnabled(bool)), timeLabel, SLOT(enable(bool)));

  toolbar->addWidget(timeLabel);
  toolbar->addWidget(timeSlider);
  
  // important for time slider, if I add it again
  //toolbar->addSeparator();
  
  toolbar->addAction(_colourAct);
  toolbar->addAction(_zoomInAct);
  toolbar->addAction(_zoomOutAct);
  toolbar->addAction(_rotateLAct);
  toolbar->addAction(_rotateRAct);
  toolbar->addAction(_searchAct);
  toolbar->addAction(_taxBoxAct);
  toolbar->addAction(_barchartAct);
}

void HapnetWindow::showErrorDlg(const QString &text, const QString &detailedText, const QString &informativeText)
{
  QMessageBox errorBox;
  errorBox.setIcon(QMessageBox::Critical);
  errorBox.setText(text);
  if (! detailedText.isEmpty())
    errorBox.setDetailedText(detailedText);
  if (! informativeText.isEmpty())
    errorBox.setInformativeText(informativeText);
  errorBox.setStandardButtons(QMessageBox::Ok);
  errorBox.setDefaultButton(QMessageBox::Ok);
  errorBox.exec();

}

void HapnetWindow::showWarnDlg(const QString &text, const QString &detailedText, const QString &informativeText)
{
  QMessageBox warnBox;
  warnBox.setIcon(QMessageBox::Warning);
  warnBox.setText(text);
  if (! detailedText.isEmpty())
    warnBox.setDetailedText(detailedText);
  if (! informativeText.isEmpty())
    warnBox.setInformativeText(informativeText);
  warnBox.setStandardButtons(QMessageBox::Ok);
  warnBox.setDefaultButton(QMessageBox::Ok);
  warnBox.exec();

}


void HapnetWindow::openAlignment()
{
  QString oldname(_filename);

  //_filename = QFileDialog::getOpenFileName(this, "Open alignment file", ".", "Fasta Files (*.fas *.fa *.fasta);;Phylip Files (*.seq *.phy *.phylip);;Nexus files (*.nex *.mac);;All Files(*)").toStdString();
  _filename = QFileDialog::getOpenFileName(this, "Open alignment file", tr("."), "Nexus files (*.nex *.mac);;All Files(*)");
  
  if (_filename != "")
  {  
    if (! _alignment.empty())
    {
       closeAlignment();
       closeTraits();
    }

    // TODO check whether these functions can produce traits exceptions that aren't caught
    bool success = loadAlignmentFromFile();
    bool traitsuccess = loadTraitsFromParser();

    if (success)
    {
      statusBar()->showMessage(tr("Loaded file %1").arg(_filename));
      //_networkMenu->setEnabled(true);
      
      //if (traitsuccess)  _stats->setFreqsFromTraits(_traitVect);
      QFileInfo fileInfo(_filename);
      setWindowTitle(tr("PopART: %1").arg(fileInfo.fileName()));
      toggleAlignmentActions(true);

      if (! _alModel)
      {
        _alModel = new AlignmentModel(_alignment);
        _alView->setModel(_alModel);

        //_alModel->setCharType(_datatype);
        _alModel->setCharType(_datatype);
        
        if (! _mask.empty())
        {
          for (unsigned i = 0; i < _mask.size(); i++)
            if (! _mask.at(i))
              _alModel->maskColumns(i, i);
        }
        
        if (! _badSeqs.empty())
        {
          for (unsigned i = 0; i < _badSeqs.size(); i++)
            _alModel->maskRows(_badSeqs.at(i), _badSeqs.at(i));
        }
        
        // TODO check this
        if (traitsuccess)
        {
          _tModel = new TraitModel(_traitVect);
          _tView->setModel(_tModel);

          if (_view == Map)
          {
            _mapView->addHapLocations(_traitVect);
            _mapTraitsSet = true;
          }

          else
            _mapTraitsSet = false;
        }
      }


      else
      {
        _netView->clearModel();
        toggleNetActions(false);

        QAbstractItemModel *tmpModel = _tView->model();
        if (traitsuccess)
        {
          _tModel = new TraitModel(_traitVect);
          // Do this differently: only on switch to map view
          _tView->setModel(_tModel);
          if (_view == Map)
          {
            _mapView->addHapLocations(_traitVect);
            _mapTraitsSet = true;
          }

          else
            _mapTraitsSet = false;
        }
        delete tmpModel;

        tmpModel = _alView->model();
        _alModel = new AlignmentModel(_alignment);
        _alView->setModel(_alModel);
        _alModel->setCharType(_datatype);
        delete tmpModel;
        //_alView->resizeColumnsToContents();
      }
    }
  }

  else 
  {
    _filename = oldname;
    statusBar()->showMessage(tr("No file selected"));
  }
}

bool HapnetWindow::loadAlignmentFromFile(QString filename)
{
  
  //int index = 0;
  if (filename.isEmpty())  filename = _filename;

  const char *cstr = filename.toLatin1().constData();


  // need a std::ifstream to deal with Sequence input
  ifstream seqfile(cstr);
  
 // _networkArea->writeToConsole("Opened seqfile.");

  // Set null Parser so that appropriate parser will be chosen
  Sequence::setParser(0);

  if (!seqfile)  return false;
  
  //_alignment = new vector<Sequence *>;
  Sequence *seqptr;
  int nseq = -1, seqcount = 0;
  _progress->setValue(0);
  _progress->setLabelText("Loading alignment...");
  _progress->show();

  try
  {
    while (seqfile.good())
    {
      seqptr = new Sequence();
      seqfile >> (*seqptr);
      QString qseq = QString::fromStdString(seqptr->seq()).toUpper();
      seqptr->setSeq(qseq.toStdString());
      _alignment.push_back(seqptr);
      //cout << "nseq: " << seqptr->parser()->nSeq() << endl;
      if (nseq < 0)  nseq = seqptr->parser()->nSeq();
      _progress->setValue((int)(50.0 * seqcount++/ nseq + 0.5));
      //cout << "set value to: " << _progress->value() << endl;
    }
    //_progress->hide();
  }
    
  catch (SeqParseError &spe)
  {
    _progress->hide();
    //cerr << spe.what() << endl;
    //_errorMessage.showMessage("Error parsing sequence data.");
    QMessageBox error;
    error.setIcon(QMessageBox::Critical);
    error.setText(tr("<b>Error parsing sequence data</b>"));
    error.setDetailedText(spe.what());
    error.setStandardButtons(QMessageBox::Ok);
    error.setDefaultButton(QMessageBox::Ok);
    error.exec();
   
    return false;
  }

  seqfile.close();
  
  SeqParser *parser = Sequence::parser();
  
  QString msg = QString::fromStdString(parser->getWarning());
  
  while (! msg.isEmpty())
  {
    QMessageBox warnBox;
    warnBox.setIcon(QMessageBox::Warning);
    warnBox.setText(tr("<b>Warning from Nexus Parser</b>"));
    warnBox.setInformativeText(msg);
    warnBox.setStandardButtons(QMessageBox::Ok);
    warnBox.setDefaultButton(QMessageBox::Ok);
    warnBox.exec();
    
    msg = QString::fromStdString(parser->getWarning());
  }
  

  if (parser->charType() == SeqParser::AAType)
    _datatype = Sequence::AAType;
  else if (parser->charType() == SeqParser::DNAType)
    _datatype = Sequence::DNAType;
  else if (parser->charType() == SeqParser::StandardType)
    _datatype = Sequence::BinaryType;
  
  /*if (_datatype == Sequence::BinaryType)
  else if (_datatype == Sequence::DNAType)
  else*/
  _progress->setLabelText("Detecting problem sites...");
  vector<unsigned> ambiguousCounts;
  
  for (unsigned i = 0; i < _alignment.size(); i++)
  {
    _alignment.at(i)->setCharType(_datatype);
    
    ambiguousCounts.push_back(0);
    
    if (i == 0)
      _mask.resize(_alignment.at(i)->length(), true);
    
    else  if (_mask.size() != _alignment.at(i)->length())
    {
      QMessageBox warnBox;
      warnBox.setIcon(QMessageBox::Warning);
      warnBox.setText(tr("<b>Sequence lengths differ</b>"));
      warnBox.setStandardButtons(QMessageBox::Ok);
      warnBox.setDefaultButton(QMessageBox::Ok);
      warnBox.exec();

      if (_mask.size() < _alignment.at(i)->length())
        _mask.resize(_alignment.at(i)->length(), false);
      else
        for (unsigned j = _alignment.at(i)->length(); j < _mask.size(); j++)
          _mask.at(j) = false;
    }
    
    for (unsigned j = 0; j < _mask.size(); j++)
    {
      if (Sequence::isAmbiguousChar(_alignment.at(i)->at(j), _datatype))
      {
        _mask.at(j) = false;
        ambiguousCounts[i]++;
      }
    }
  }
  
  unsigned totalAmbiguous = 0;
  for (unsigned i = 0; i < _mask.size(); i++)
    if (! _mask.at(i))  totalAmbiguous++;
    
  if (totalAmbiguous * 100. / _mask.size() > 5)
  {
    
    //_errorMessage.showMessage("Warning: more than 5% sites contain undefined states and will be masked.");
    //_errorMessage.
    QMessageBox warnBox;
    warnBox.setIcon(QMessageBox::Warning);
    warnBox.setText(tr("<b>More than 5% sites contain undefined states and will be masked</b>"));
    warnBox.setStandardButtons(QMessageBox::Ok);
    warnBox.setDefaultButton(QMessageBox::Ok);
    warnBox.exec();
    
    double mean = 0;
    for (unsigned i = 0; i < ambiguousCounts.size(); i++)
      mean += ambiguousCounts.at(i);
    
    mean /= ambiguousCounts.size();
    
    double stdev = 0;
    
    for (unsigned i = 0; i < ambiguousCounts.size(); i++)
      stdev += qPow(ambiguousCounts.at(i) - mean, 2);
    
    stdev = qSqrt(stdev / ambiguousCounts.size());
    
    double outlierThreshold = mean + 0.5 * stdev;
    
    QStringList problemSeqs;
    
    for (unsigned i = 0; i < ambiguousCounts.size(); i++)
    {
      if (ambiguousCounts.at(i) > outlierThreshold)
      {
        problemSeqs.append(QString::fromStdString(_alignment.at(i)->name()));
        _badSeqs.push_back(i);
      }
    }
    
    _progress->setValue(100);

    if (! problemSeqs.isEmpty())
    {
      
      QMessageBox message;
      message.setIcon(QMessageBox::Question);
      message.setText(tr("<b>Some sequences contain significantly more undefined states than others</b>"));
      message.setInformativeText(tr("Remove these sequences?"));
      message.setDetailedText(problemSeqs.join(tr("\n"))); 
      message.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      message.setDefaultButton(QMessageBox::No);
      int result = message.exec();
      
      if (result == QMessageBox::No)
        _badSeqs.clear();
      
      else
      {
        
        unsigned badSeqIdx = 0;
        for (unsigned i = 0; i < _alignment.size(); i++)
        {          
          if (i == 0)
            _mask.assign(_alignment.at(i)->length(), true);
          
          if (badSeqIdx < _badSeqs.size() && _badSeqs.at(badSeqIdx) == i)
          {
            badSeqIdx++;
            continue;
          }
          
          else
            _goodSeqs.push_back(_alignment.at(i));
            
          
          if (_mask.size() < _alignment.at(i)->length())
            _mask.resize(_alignment.at(i)->length(), false);
          else  if (_mask.size() > _alignment.at(i)->length())
            for (unsigned j = _alignment.at(i)->length(); j < _mask.size(); i++)
              _mask.at(j) = false;
         
          
          for (unsigned j = 0; j < _mask.size(); j++)
          {
            if (Sequence::isAmbiguousChar(_alignment.at(i)->at(j), _datatype))
            {
              _mask.at(j) = false;
            }
          }
        }
      }
    }
  }
  
  if (_goodSeqs.empty())
    _goodSeqs.assign(_alignment.begin(), _alignment.end());

  //doStatsSetup();
  _progress->hide();
  return true;
}

bool HapnetWindow::loadTraitsFromParser()
{
  NexusParser *parser = dynamic_cast<NexusParser *>(Sequence::parser());
  
  if (! parser)  return false;
  
  try
  {
    if (parser->hasTraits() && parser->hasGeoTags())
    {
      QDialog dlg(this);
      
      QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
      
      vlayout->addWidget(new QLabel("Nexus file has both Traits and GeoTags data.<br>Which do you want to use by default to colour networks?", this));
      
      QButtonGroup *buttonGroup = new QButtonGroup(this);
      int idCount = 0;
      QRadioButton *button = new QRadioButton("Use Traits", &dlg);
      button->setChecked(true);
      buttonGroup->addButton(button, idCount++);
      vlayout->addWidget(button);
      
      button = new QRadioButton("Use GeoTags", &dlg);
      buttonGroup->addButton(button, idCount++);
      vlayout->addWidget(button);
      
      QHBoxLayout *hlayout = new QHBoxLayout;
      
      QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
      connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
      hlayout->addWidget(okButton, 0, Qt::AlignRight);
      QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
      connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
      hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
      
      vlayout->addLayout(hlayout);      
      
      int result = dlg.exec();
      
      if (result == QDialog::Rejected)
        return false;
      
      if (buttonGroup->checkedId() == 0)
        _traitVect = parser->traitVector();
      else
      {
        const vector<GeoTrait*> & gtv = parser->geoTraitVector();
        _traitVect.assign(gtv.begin(), gtv.end());
      }
    }
    
    else if (parser->hasTraits())
      _traitVect = parser->traitVector();
    
    else if (parser->hasGeoTags())
    {
      const vector<GeoTrait*> & gtv = parser->geoTraitVector();
      _traitVect.assign(gtv.begin(), gtv.end());
    }
    
    else
      return false;
  }
  
  catch (SeqParseError &spe)
  {
    _traitVect.clear();
    QMessageBox error;
    error.setIcon(QMessageBox::Critical);
    error.setText(tr("<b>Error parsing traits</b>"));
    error.setDetailedText(spe.what());
    error.setStandardButtons(QMessageBox::Ok);
    error.setDefaultButton(QMessageBox::Ok);
    error.exec();

    //_errorMessage.showMessage("Error parsing traits.");
    return false;
  }
  
  return true;
}

bool HapnetWindow::loadTreesFromParser(vector<ParsimonyTree *> & treevect)
{
  
  NexusParser *parser = dynamic_cast<NexusParser *>(Sequence::parser());
  
  if (! parser)  return false;

  if (parser->treeVector().empty())  return false;
  vector<Tree *>::const_iterator treeit = parser->treeVector().begin();
  

  try
  {
    while (treeit != parser->treeVector().end())
    {
      treevect.push_back(new ParsimonyTree(**treeit));
      ++treeit;
    }
  }
  
  catch (TreeError te)
  {
    QMessageBox error;
    error.setIcon(QMessageBox::Critical);
    error.setText(tr("<b>Error in an input tree</b>"));
    error.setDetailedText(te.what());
    error.setStandardButtons(QMessageBox::Ok);
    error.setDefaultButton(QMessageBox::Ok);
    error.exec();

    //_errorMessage.showMessage("Error in an input tree.");
    return false;
  }
  
  catch (SeqParseError spe)
  {
    QMessageBox error;
    error.setIcon(QMessageBox::Critical);
    error.setText(tr("<b>Error parsing trees</b>"));
    error.setDetailedText(spe.what());
    error.setStandardButtons(QMessageBox::Ok);
    error.setDefaultButton(QMessageBox::Ok);
    error.exec();

    //_errorMessage.showMessage("Error parsing trees.");
    return false;
  }
  
  return true;
}

void HapnetWindow::closeAlignment()
{
  if (! _alignment.empty())
  {
    /*_alModel->removeColumns(0, _alModel->columnCount());
    _alModel->removeRows(0, _alModel->rowCount());*/
    setWindowTitle("PopART");
    
    for (unsigned i = 0; i < _alignment.size(); i++)
      delete _alignment.at(i);
    
    _alignment.clear();
    _goodSeqs.clear();
    _mask.clear();
    _badSeqs.clear();
    
    for (unsigned i = 0; i < _treeVect.size(); i++)
      delete _treeVect.at(i);
    _treeVect.clear();
    

    QAbstractItemModel *tmpModel = _alView->model();
    _alView->setModel(0);
    delete tmpModel;
    _alModel = 0;
    
    /*tmpModel = _tView->model();
    //_tModel = new TraitModel(_traitVect);
    _tView->setModel(0);//_tModel);
    delete tmpModel;
    _tModel = 0;*/

    toggleAlignmentActions(false);
    
    _netView->clearModel();
    toggleNetActions(false);
    delete _netModel;
    _netModel = 0;
    
    _history->clear();

  }
  
  if (_stats) 
  {
    delete _stats;
    _stats = 0;
  }
  
}

void HapnetWindow::closeTraits()
{
  if (! _traitVect.empty())
  {
    for (unsigned i = 0; i < _traitVect.size(); i++)
      delete _traitVect.at(i);
    _traitVect.clear();
  }

  QAbstractItemModel *tmpModel = _tView->model();
    //_tModel = new TraitModel(_traitVect);
    _tView->setModel(0);//_tModel);
    delete tmpModel;
    _tModel = 0;
}

void HapnetWindow::askAndCloseAlignment()
{  
  QDialog dlg(this);
  
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  
  vlayout->addWidget(new QLabel("Clear alignment data?", this));

  QHBoxLayout *hlayout = new QHBoxLayout;
  
  hlayout->addStretch(1);
  QPushButton *yesButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogYesButton), "Yes", &dlg);
  connect(yesButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(yesButton, 0, Qt::AlignRight);
  QPushButton *noButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogNoButton), "No", &dlg);
  connect(noButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(noButton, 0, Qt::AlignRight);
  vlayout->addLayout(hlayout);

  
  int result = dlg.exec();
  
  if (result == QDialog::Accepted)
    closeAlignment();
}

void HapnetWindow::askAndCloseTraits()
{  
  QDialog dlg(this);
  
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  
  vlayout->addWidget(new QLabel("Clear traits data?", this));

  QHBoxLayout *hlayout = new QHBoxLayout;
  
  hlayout->addStretch(1);
  QPushButton *yesButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogYesButton), "Yes", &dlg);
  connect(yesButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(yesButton, 0, Qt::AlignRight);
  QPushButton *noButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogNoButton), "No", &dlg);
  connect(noButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(noButton, 0, Qt::AlignRight);
  vlayout->addLayout(hlayout);

  
  int result = dlg.exec();
  
  if (result == QDialog::Accepted)
    closeTraits();
}

void HapnetWindow::importAlignment()
{
  QString phylipname = QFileDialog::getOpenFileName(this, "Open alignment file", tr("."), "Phylip files (*.phy *.seq *.phylip);;All Files(*)");

  if (phylipname != "")
  {
    if (! _alignment.empty())
       closeAlignment();
    
    if (! _traitVect.empty())
      askAndCloseTraits();

    bool success = loadAlignmentFromFile(phylipname);

    if (success)
    {
      _filename = QString("%1.%2").arg(phylipname).arg("nex");
      statusBar()->showMessage(tr("Imported file %1").arg(phylipname));
      //_networkMenu->setEnabled(true);

      QFileInfo fileInfo(phylipname);
      setWindowTitle(tr("PopART: %1").arg(fileInfo.fileName()));
      toggleAlignmentActions(true);

      if (! _alModel)
      {
        _alModel = new AlignmentModel(_alignment);
        _alView->setModel(_alModel);

        _alModel->setCharType(_datatype);

        if (! _mask.empty())
        {
          for (unsigned i = 0; i < _mask.size(); i++)
            if (! _mask.at(i))
              _alModel->maskColumns(i, i);
        }

        if (! _badSeqs.empty())
        {
          for (unsigned i = 0; i < _badSeqs.size(); i++)
            _alModel->maskRows(_badSeqs.at(i), _badSeqs.at(i));
        }


      }

      else
      {
        _netView->clearModel();
        toggleNetActions(false);
        QAbstractItemModel *tmpModel = _alView->model();
        _alModel = new AlignmentModel(_alignment);
        _alView->setModel(_alModel);
        _alModel->setCharType(_datatype);
        delete tmpModel;
        //_alView->resizeColumnsToContents();
      }
    _dataWidget->setCurrentWidget(_alView);
    }
  }

  else
    statusBar()->showMessage(tr("No file selected"));
}

void HapnetWindow::importTraits()
{
  QString filename = QFileDialog::getOpenFileName(this, "Open traits table", tr("."), "Table files (*.csv *.txt);;All Files(*)");
    
  if (filename != "")
  {      
    if (! _alignment.empty())
      askAndCloseAlignment();
    
    if (! _traitVect.empty())
      closeTraits();

    _tp = new TableParser();
    bool success = loadTableFromFile(filename);
    if (success)
    {    
      bool warned = false;
      set<string> seqnames;
      set<string> alseqnames;
      if (! _alignment.empty())
      {
        for (unsigned i = 0; i < _alignment.size(); i++)
          alseqnames.insert(_alignment.at(i)->name());
      }

      for (unsigned i = 0; i < _tp->columns(); i++)
      {
        _traitVect.push_back(new Trait(_tp->headerData().at(i)));
        // set data type to integer for trait data
        _tp->setDataType(i, 'd');
      }

      for (unsigned i = 0; i < _tp->rows(); i++)
      {
        pair<set<string>::iterator,bool> result = seqnames.insert(_tp->vHeaderData(i));
        if (! warned && ! alseqnames.empty())
        {
           set<string>::iterator sit = alseqnames.find(*(result.first));
           
           if (sit == alseqnames.end())
           {
             QString text(tr("<b>Sequence %1 appears in traits file but not alignment</b>").arg(QString::fromStdString(*(result.first))));
             QString infText("If traits file does not correspond to alignment, network inference will produce errors.");
             showWarnDlg(text, QString(), infText);
            
             warned = true;

           }
        }
        
        if (! result.second) // sequence name repeated in file
        {
          QString text(tr("<b>Sequence %1 appears more than once!</b>").arg(QString::fromStdString(*(result.first))));
          QString detText("Duplicate sequence names should be merged into one line");
          showErrorDlg(text, detText);
          
          closeTraits();
          delete _tp;
          
          return;
          
        }
        
        for (unsigned j = 0; j < _tp->columns(); j++)
        {
          // TODO remove whitespace from entries after reading!!!
          try
          {
            unsigned count = (unsigned)(_tp->dataInt(i, j));
            if (count > 0)
              _traitVect.at(j)->addSeq(*(result.first), count);
          } 
          
          catch (SeqParseError &spe)
          {
            QString text("<b>Error importing traits</b>");
            QString detText(spe.what());
            showErrorDlg(text, detText);
            
            closeTraits();
            delete _tp;
            
            return;
          }

        }
      }
      

      QAbstractItemModel *tmpModel = _tView->model();
      _tModel = new TraitModel(_traitVect);
      _tView->setModel(_tModel);
      delete tmpModel;

      if (_view == Map)
      {
        _mapView->addHapLocations(_traitVect);
        _mapTraitsSet = true;
      }

      else
        _mapTraitsSet = false;

    _dataWidget->setCurrentWidget(_tView);
    }

    delete _tp;
    
  }
  
  else 
  {
    statusBar()->showMessage(tr("No file selected"));
  }
}

void HapnetWindow::importGeoTags()
{
  QString filename = QFileDialog::getOpenFileName(this, "Open geotag table", tr("."), "Table files (*.csv *.txt);;All Files(*)");
    
  // TODO ask user for number, location of clusters?
  if (filename != "")
  {      
    if (! _alignment.empty())
      askAndCloseAlignment();
    
    if (! _traitVect.empty())
      closeTraits();

    _tp = new TableParser();
    bool success = loadTableFromFile(filename);
    
    // check that sequence names match alignment
    // note that sequences can appear more than once, at different locations
    // TODO (also: check this when importing alignment)
    // get lat,long pair for each sequence
    // check for header: if longitude latitude or long lat, treat as backwards
    // dump sequence names into a vector, coordinate pair into another
    // or maybe a geoseq? sequence with coordinates?
    // run kmeans, get a set of GeoTraits? This could even be a GeoTrait static method
    // think about extending Trait to GeoTrait (cluster centroid, can be given a name)
    if (success)
    { 
      //cout << "columns: " << _tp->columns() << endl;
      if (_tp->columns() == 3)
        _tp->setDataType(2, 'd');

      else if (_tp->columns() != 2)
      {
        QString text("<b>Wrong number of columns in geotags table</b>");
        QString detText("Each entry in the geotags file should consist of a sequence name, latitude, longitude, and (optionally) count");
        showErrorDlg(text, detText);
        
        delete _tp;
        return;
      }  
      
      vector<string> seqnames;
      vector<pair<float,float> > coordinates;
      vector<unsigned> seqcounts;
     
      set<string> alseqnames;
      bool warned = false;
      if (! _alignment.empty())
      {
        for (unsigned i = 0; i < _alignment.size(); i++)
          alseqnames.insert(_alignment.at(i)->name());
      }
            
      for (unsigned i = 0; i < _tp->rows(); i++)
      {
        string name = _tp->vHeaderData(i);
        if (! warned && ! alseqnames.empty())
        {
           set<string>::iterator sit = alseqnames.find(name);
           
           if (sit == alseqnames.end())
           {
             QString text(tr("<b>Sequence %1 appears in traits file but not alignment</b>").arg(QString::fromStdString(name)));
             QString infText("If traits file does not correspond to alignment, network inference will produce errors.");
             showWarnDlg(text, QString(), infText);
            
             warned = true;
           }
        }
        
        seqnames.push_back(name);
        const string & latStr = _tp->data(i,0);
        const string & lonStr = _tp->data(i,1);
        coordinates.push_back(GeoTrait::getCoordinate(latStr, lonStr));
        
        unsigned count = 1;
        if (_tp->columns() == 3)
          count = (unsigned)(_tp->dataInt(i, 2));
        if (count == 0)
          count = 1;
        
        seqcounts.push_back(count);
          
      }
      
      QDialog dlg(this);
      QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
      QHBoxLayout *hlayout = new QHBoxLayout;
      
      QLabel *label = new QLabel("Number of location clusters (0 to estimate, very slow!):", &dlg);
      hlayout->addWidget(label);
      
      QSpinBox *spinBox = new QSpinBox(&dlg);
      //spinBox->setRange(0, 10);
      spinBox->setMinimum(0);
      spinBox->setValue(5);
      hlayout->addWidget(spinBox);
      
      vlayout->addLayout(hlayout);
      
      hlayout = new QHBoxLayout;
      
      hlayout->addStretch(1);
      QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
      connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
      hlayout->addWidget(okButton, 0, Qt::AlignRight);
      QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
      connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
      hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
      
      vlayout->addLayout(hlayout);
      
      int result = dlg.exec();
      
      if (result == QDialog::Rejected)
      {
        delete _tp;
        return;
      }
      
      unsigned nclusts = spinBox->value();
      
      
      vector<GeoTrait> geoTraits = GeoTrait::clusterSeqs(coordinates, seqnames, seqcounts, nclusts);
      
      for (unsigned i = 0; i < geoTraits.size(); i++)
        _traitVect.push_back(new GeoTrait(geoTraits.at(i)));
      
      //_traitVect.assign(geoTraits.begin(), geoTraits.end());
      
 
      QAbstractItemModel *tmpModel = _tView->model();
      _tModel = new TraitModel(_traitVect);
      _tView->setModel(_tModel);
      delete tmpModel;

      if (_view == Map)
      {
        _mapView->addHapLocations(_traitVect);
        _mapTraitsSet = true;
      }

      else
        _mapTraitsSet = false;

    _dataWidget->setCurrentWidget(_tView);

    delete _tp;
     
      
      // TODO Add progress meter if number of traits estimated
      // add geo option to Traits block in Nexus
      // create GeoTags block
      // work on save network (and parsing)

    }
  }
  
  else 
  {
    statusBar()->showMessage(tr("No file selected"));
  }
}

bool HapnetWindow::loadTableFromFile(const QString &filename)
{
  const char *cstr = filename.toLatin1().constData();

  ifstream tabfile(cstr);
  _tabfile = &tabfile;

  //_tp = new TableParser();

  _tp->setHasHeader(true);
  _tp->setHasVHeader(true);
  _tp->setMergeDelims(true);
  // Do this in another function so that we can connect table interaction checkboxes to parser


  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);

  QHBoxLayout *hlayout = new QHBoxLayout;

  QButtonGroup *radioButtons = new QButtonGroup(this);
  int idCount = 0;

  QVBoxLayout *vlayout2 = new QVBoxLayout;
  hlayout->addWidget(new QLabel("Delimiter:", &dlg));

  QAbstractButton *button = new QRadioButton("Comma", &dlg);
  button->setChecked(true);
  radioButtons->addButton(button, idCount++);
  vlayout2->addWidget(button);

  button = new QRadioButton("Space", &dlg);
  radioButtons->addButton(button, idCount++);
  vlayout2->addWidget(button);

  button = new QRadioButton("Tab", &dlg);
  radioButtons->addButton(button, idCount++);
  vlayout2->addWidget(button);

  connect(radioButtons, SIGNAL(buttonClicked(int)), this, SLOT(changeDelimiter(int)));
  hlayout->addLayout(vlayout2);

  vlayout2 = new QVBoxLayout;

  button = new QCheckBox("Merge consecutive delimiters", &dlg);
  button->setChecked(true);
  connect(button, SIGNAL(toggled(bool)), this, SLOT(setMergeDelims(bool)));
  vlayout2->addWidget(button);

  button = new QCheckBox("Table has header row", &dlg);
  button->setChecked(true);
  connect(button, SIGNAL(toggled(bool)), this, SLOT(setHasHeader(bool)));
  //button->setEnabled(false);
  vlayout2->addWidget(button);

  /*button = new QCheckBox("Table has header column", &dlg);
  button->setChecked(true);
  connect(button, SIGNAL(toggled(bool)), this, SLOT(setHasVHeader(bool)));
  button->setEnabled(false);
  vlayout2->addWidget(button);*/

  hlayout->addLayout(vlayout2);
  vlayout->addLayout(hlayout);

  // TODO check for vertical header
  // Use a delegate to colour columns by data type?
  // Allow user to set data type?
  // Fix column widths (too wide by default)
  // Add checkboxes to change delimiters, etc.
  _table = new QTableWidget(&dlg);
  updateTable();
  //table->setRowCount(tp.rows());
  //table->setColumnCount(tp.columns());

  vlayout->addWidget(_table);


  hlayout = new QHBoxLayout;


  hlayout->addStretch(1);
  QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
  connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(okButton, 0, Qt::AlignRight);
  QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
  connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(cancelButton, 0, Qt::AlignRight);

  vlayout->addLayout(hlayout);

  int result = dlg.exec();


 bool cancelled = false;
 if (result == QDialog::Rejected)
   cancelled = true;

  // return parse result


  /*tp.readTable(tabfile);

  const vector<vector<string> > & data = tp.data();

  for (unsigned i = 0; i < data.size(); i++)
  {
    for (unsigned j = 0; j < data.at(i).size(); j++)
      cout << data.at(i).at(j) << " ";
    cout << "\n";
  }*/

  tabfile.close();

  delete _table;

  return ! cancelled;
}

void HapnetWindow::updateTable()
{
  _tabfile->clear();
  _tabfile->seekg(ios::beg);
  
  _table->clear();
  _tp->readTable(*_tabfile);

  const vector<vector<string> > & data = _tp->data();

  QStringList headerList;

  //qDebug() << "header list:" << headerList;

  _table->setRowCount(_tp->rows());
  _table->setColumnCount(_tp->columns());

  if (_tp->hasHeader())
  {
    const vector<string> & header = _tp->headerData();
    for (unsigned i = 0; i < header.size(); i++)
      headerList << QString::fromStdString(header.at(i));
    _table->setHorizontalHeaderLabels(headerList);
  }

  if (_tp->hasVHeader())
  {
    headerList.clear();
    const vector<string> & header = _tp->vHeaderData();
    for (unsigned i = 0; i < header.size(); i++)
      headerList << QString::fromStdString(header.at(i));
    _table->setVerticalHeaderLabels(headerList);
  }

  for (unsigned i = 0; i < data.size(); i++)
  {
    for (unsigned j = 0; j < data.at(i).size(); j++)
    {
      /*if (hasVheader)
      {
        if (j == 0)
          _table->setVerticalHeaderItem(i, new QTableWidgetItem(QString::fromStdString(data.at(i).at(j))));
        else
        {
          QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(data.at(i).at(j)));
          item->setFlags(Qt::NoItemFlags);
          _table->setItem(i, j - 1, item);
        }
      }

      else
      {*/
        QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(data.at(i).at(j)));
        item->setFlags(Qt::NoItemFlags);
        _table->setItem(i, j, item);
      //}
    }
  }

  //qDebug() << "table dimensions:" << _table->rowCount() << _table->columnCount();


  _table->resizeColumnsToContents();


}

void HapnetWindow::changeDelimiter(int buttonIdx)
{
  switch(buttonIdx)
  {
    case 0:
      _tp->setDelimChar(',');
      break;
    case 1:
      _tp->setDelimChar(' ');
      break;
    case 2:
      _tp->setDelimChar('\t');
      break;
    default:
      qDebug() << "this shouldn't happen";
      break;
  }

  updateTable();
}

void HapnetWindow::setMergeDelims(bool merge)
{
  _tp->setMergeDelims(merge);
  updateTable();
}

void HapnetWindow::setHasHeader(bool hasHeader)
{
  _tp->setHasHeader(hasHeader);
  updateTable();
}

void HapnetWindow::setHasVHeader(bool hasVHeader)
{
  _tp->setHasVHeader(hasVHeader);
  updateTable();
}

void HapnetWindow::saveNexusFile()
{
  writeNexusFile(cout);
}

ostream & HapnetWindow::writeNexusFile(ostream &out)
{
  
  vector<string> vertLabels;
  
  
  // TODO get vertex labels from _netView
  for (unsigned i = 0; i < _g->vertexCount(); i++)
  {
    vector<string> identical = _g->identicalTaxa(i);
    if (identical.size() > 0)
      vertLabels.push_back(identical.at(0));
  }
  

  out << "Begin Network;" << endl;
  out << "Dimensions ntax=" << vertLabels.size() << " nvertices=" << _g-> vertexCount() << " nedges=" << _g->edgeCount() << ' ';

  QRectF plotRect = _netView->sceneRect();
  out << "plotDim=" << plotRect.x() << ',' << plotRect.y() << ',' << plotRect.width() << ',' << plotRect.height();
  out << ';' << endl;
  //out << "DRAW to_scale;" << endl;
  out << "Format ";
  
  out << "Font=" << _netView->labelFont().toString().toStdString() << ' ';
  out << "LabelFont=" << _netView->legendFont().toString().toStdString() << ' ';
  out << "VColour=" << _netView->vertexColour().name().toStdString() << ' ';
  out << "EColour=" << _netView->edgeColour().name().toStdString() << ' ';
  out << "BGColour=" << _netView->backgroundColour().name().toStdString() << ' ' ;
  out << "VSize=" << _netView->vertexSize() << ' ';
  out << "EView=";
  
  switch (_netView->edgeMutationView())
  {
    case EdgeItem::ShowDashes:
      out << "Dashes ";
      break;
    case EdgeItem::ShowEllipses:
      out << "Ellipses ";
      break;
    case EdgeItem::ShowNums:
    default:
      out << "Numbers ";
      break;
  }
  
  QPointF legendPos = _netView->legendPosition();
  out << "LPos=" << legendPos.x() << ',' << legendPos.y() << ' ';
  out << "LCols=";
  bool first = true;
  for (QList<QColor> legendCols = _netView->traitColours(); ! legendCols.isEmpty(); legendCols.pop_front())
  {
    if (first)
      first = false;
    else
      out << ',';
    out << legendCols.front().name().toStdString();
  }
  
  out << ';' << endl;
  
  out << "Translate" << endl;
  
  for (unsigned i = 0; i < vertLabels.size(); i++)
  {
    out << (i + 1) << ' ' << vertLabels.at(i) << ',' << endl;
  }
  
  out << ';' << endl;
  
  out << "Vertices" << endl;
  for (unsigned i = 0; i < _g->vertexCount(); i++)
  {
    QPointF vertPos = _netView->vertexPosition(i);
    out << (i + 1) << ' ' << vertPos.x() << ' ' << vertPos.y() << ',' << endl;
  }
  
  out << ';' << endl;
  
  out << "VLabels" << endl;
  for (unsigned i = 0; i < _g->vertexCount(); i++)
  {
    QPointF labPos = _netView->labelPosition(i);
    out << (i + 1) << ' ' << labPos.x() << ' ' << labPos.y() << ',' << endl;
  }
  
  out << ';' << endl;
    
  out << "Edges" << endl;
  for (unsigned i = 0; i < _g->vertexCount(); i++)
  {
    const Edge *e = _g->edge(i);
    out << (i + 1) << ' ' << e->from()->index() << ' ' << e->to()->index() << ',' << endl;
  } 
  
  out << ';' << endl;
  
  out << "End;" << endl;
           
  return out;
}

void HapnetWindow::exportNetwork()
{
  if (! _g)
  {
    QMessageBox warnBox;
    warnBox.setIcon(QMessageBox::Warning);
    warnBox.setText(tr("<b>No network to export</b>"));
    warnBox.setStandardButtons(QMessageBox::Ok);
    warnBox.setDefaultButton(QMessageBox::Ok);
    warnBox.exec();

    //_errorMessage.showMessage("Error, no network to export.");
  }
  
  QString defaultName(_filename);
  
  if (defaultName.endsWith(".nex", Qt::CaseInsensitive))
    defaultName.replace(defaultName.length() - 3, 3, "txt"); 
  else
    defaultName.append(".txt");
  
  QString filename = QFileDialog::getSaveFileName(this, tr("Save Network"), tr("./%1").arg(defaultName), tr("Tab-delimited text files (*.txt);;Comma-separated value files (*.csv)"));

  if (filename.isEmpty())  return;
  QFile file(filename);
  if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
     return;

  char separator = '\t';
  QTextStream out(&file);
  if (filename.endsWith(".csv", Qt::CaseInsensitive))
    separator = ',';
  
  Edge *e;
  for (unsigned i = 0; i < _g->edgeCount(); i++)
  {
    e = _g->edge(i);
    QString label = QString::fromStdString(e->from()->label());
    if (label.isEmpty())
      out << e->from()->index();
    else
      out << label;
    
    out << separator << e->weight() << separator;
    
    label = QString::fromStdString(e->to()->label());
    if (label.isEmpty())
      out << e->to()->index();
    else
      out << label;
    
    out << "\n";
  }
  
  file.close();
}

void HapnetWindow::saveGraphics()
{
  QString defaultName(_filename);
  
  if (defaultName.endsWith(".nex", Qt::CaseInsensitive))
    defaultName.replace(defaultName.length() - 3, 3, "svg"); 
  else
    defaultName.append(".svg");
  
  QString filename = QFileDialog::getSaveFileName(this, tr("Save Image"), tr("./%1").arg(defaultName), tr("Scalable Vector Graphics files (*.svg);;Portable Network Graphics files (*.png);;Portable Document Format (*.pdf)"));

  if (filename.isEmpty())  return;
  
  if (filename.endsWith(".svg", Qt::CaseInsensitive))
  {
    if (_view == Net)
      _netView->saveSVGFile(filename);
    else if (_view == Map)
      _mapView->saveSVGFile(filename);
  }
    
  else if (filename.endsWith(".png", Qt::CaseInsensitive))
  {
    if (_view == Net)
      _netView->savePNGFile(filename);
    else if (_view == Map)
      _mapView->savePNGFile(filename);
  }
  
  else if (filename.endsWith(".pdf", Qt::CaseInsensitive))
  {
    if (_view == Net)
      _netView->savePDFFile(filename);
    else if (_view == Map)
      _mapView->savePDFFile(filename);
  }
  
  else
  {
     //_errorMessage.showMessage("Unsupported graphics file type.");
    QMessageBox warnBox;
    warnBox.setIcon(QMessageBox::Warning);
    warnBox.setText(tr("<b>Unsupported graphics format</b>"));
    warnBox.setStandardButtons(QMessageBox::Ok);
    warnBox.setDefaultButton(QMessageBox::Ok);
    warnBox.exec();

  }
  
}

void HapnetWindow::quit()
{
  closeAlignment();
  closeTraits();
  qApp->quit();
}

void HapnetWindow::buildMSN()
{  
  
  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  QHBoxLayout *hlayout = new QHBoxLayout;
  
  QLabel *label = new QLabel("Epsilon", &dlg);
  hlayout->addWidget(label);
  
  QSpinBox *spinBox = new QSpinBox(&dlg);
  spinBox->setRange(0, 10);
  hlayout->addWidget(spinBox);
  
  vlayout->addLayout(hlayout);
  
  hlayout = new QHBoxLayout;
  
  hlayout->addStretch(1);
  QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
  connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(okButton, 0, Qt::AlignRight);
  QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
  connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
  
  vlayout->addLayout(hlayout);
  
  int result = dlg.exec();
  
  if (result == QDialog::Rejected)
    return;
  
  unsigned epsilon = spinBox->value();
  
  toggleNetActions(false);
  toggleAlignmentActions(false);
  _closeAct->setEnabled(false);
  _openAct->setEnabled(false);
  
  _netView->clearModel();

  _progress->setLabelText("Inferring Minimum Spanning Network...");

  inferNetwork(Msn, epsilon);
}

void HapnetWindow::buildMJN()
{
  
  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  QHBoxLayout *hlayout = new QHBoxLayout;
  
  QLabel *label = new QLabel("Epsilon", &dlg);
  hlayout->addWidget(label);
  
  QSpinBox *spinBox = new QSpinBox(&dlg);
  spinBox->setRange(0, 10);
  hlayout->addWidget(spinBox);
  
  vlayout->addLayout(hlayout);
  
  hlayout = new QHBoxLayout;
  
  hlayout->addStretch(1);
  QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
  connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(okButton, 0, Qt::AlignRight);
  QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
  connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
  
  vlayout->addLayout(hlayout);
  
  int result = dlg.exec();
  
  if (result == QDialog::Rejected)
    return;

  unsigned epsilon = spinBox->value();
  
  toggleNetActions(false);
  toggleAlignmentActions(false);
  _closeAct->setEnabled(false);
  _openAct->setEnabled(false);
  
  _netView->clearModel();
  
  _progress->setLabelText("Inferring Median Joining Network...");
  inferNetwork(Mjn, epsilon);
}


void HapnetWindow::buildAPN()
{
  
   if (_alignment.size() != _goodSeqs.size())
  {
    QMessageBox message;
    message.setIcon(QMessageBox::Question);
    message.setText(tr("<b>Some input sequences will be ignored</b>"));
    message.setInformativeText(tr("If these sequences appear in the input trees, an error will occur. Continue?"));
    message.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    message.setDefaultButton(QMessageBox::No);
    int result = message.exec();
    
    if (result == QMessageBox::No)
      return;
  }
  
  for (unsigned i = 0; i < _treeVect.size(); i++)
    delete _treeVect.at(i);
  _treeVect.clear();
  bool success = loadTreesFromParser(_treeVect);
    
  if (success)
  {
    QDialog dlg(this);
    QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
    QHBoxLayout *hlayout = new QHBoxLayout;
    
    QLabel *label = new QLabel("Minimum edge frequency:", &dlg);
    hlayout->addWidget(label);
    
    QDoubleSpinBox *spinBox = new QDoubleSpinBox(&dlg);
    spinBox->setRange(0, 1);
    spinBox->setValue(0.05);
    spinBox->setSingleStep(0.1);
    hlayout->addWidget(spinBox);
    
    vlayout->addLayout(hlayout);
    
    hlayout = new QHBoxLayout;
    
    hlayout->addStretch(1);
    QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
    connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
    hlayout->addWidget(okButton, 0, Qt::AlignRight);
    QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
    connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
    hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
    
    vlayout->addLayout(hlayout);
    
    int result = dlg.exec();
    
    if (result == QDialog::Rejected)
      return;
    
    double ancestorFreq = spinBox->value();
    
    toggleNetActions(false);
    toggleAlignmentActions(false);
    _closeAct->setEnabled(false);
    _openAct->setEnabled(false);
    
    _netView->clearModel();

    _progress->setLabelText("Inferring Ancestral Parsimony network...");

    inferNetwork(Apn, ancestorFreq);
   }
  
  else  
  {
    QMessageBox error;
    error.setIcon(QMessageBox::Critical);
    error.setText("<b>Network inference error</b>");
    error.setInformativeText("Ancestral parsimony networks require a trees block in your Nexus file");
    error.setStandardButtons(QMessageBox::Ok);
    error.setDefaultButton(QMessageBox::Ok);
    
    error.exec();
  }
}

void HapnetWindow::buildIntNJ()
{
  
  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  QHBoxLayout *hlayout = new QHBoxLayout;
  
  QLabel *label = new QLabel("Reticulation tolerance", &dlg);
  hlayout->addWidget(label);
  
  QSpinBox *spinBox = new QSpinBox(&dlg);
  spinBox->setRange(0, 10);
  spinBox->setValue(1);
  hlayout->addWidget(spinBox);
  
  vlayout->addLayout(hlayout);
  
  hlayout = new QHBoxLayout;
  
  hlayout->addStretch(1);
  QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
  connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(okButton, 0, Qt::AlignRight);
  QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
  connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
  
  vlayout->addLayout(hlayout);
  
  int result = dlg.exec();
  
  if (result == QDialog::Rejected)
    return;

  unsigned epsilon = spinBox->value();
    
  toggleNetActions(false);
  toggleAlignmentActions(false);
  _closeAct->setEnabled(false);
  _openAct->setEnabled(false);
  
  _netView->clearModel();

  _progress->setLabelText("Inferring Integer Neighbor-Joining network...");
  inferNetwork(Inj, epsilon);

}

void HapnetWindow::buildTSW()
{
  toggleNetActions(false);
  toggleAlignmentActions(false);
  _closeAct->setEnabled(false);
  _openAct->setEnabled(false);
  
  _netView->clearModel();

  _progress->setLabelText("Inferring Tight Span Walker network...");

  inferNetwork(Tsw);
}

void HapnetWindow::buildTCS()
{
  toggleNetActions(false);
  toggleAlignmentActions(false);
  _closeAct->setEnabled(false);
  _openAct->setEnabled(false);
  
  _netView->clearModel();

  _progress->setLabelText("Inferring TCS network...");
  inferNetwork(Tcs);
}

void HapnetWindow::buildUMP()
{
  statusBar()->showMessage("UMP network not yet available.");
  //_networkArea->writeToConsole("UMP network not yet available.");
}

void HapnetWindow::graphicsMove(QList<QPair<QGraphicsItem *, QPointF> > itemList)
{
  QUndoCommand *moveCommand = new MoveCommand(itemList);
  _history->push(moveCommand);
}

void HapnetWindow::inferNetwork(HapnetWindow::HapnetType netType, QVariant argument)
{
  if (_g)  delete _g;

  //_success = false;

  QString errorText;
  
  switch(netType)
  {
    case Msn:
      errorText = "Error inferring Minimum Spanning Network!";
      _g = new MinSpanNet(_goodSeqs, _mask, argument.toUInt());
      break;
    case Mjn:
      errorText = "Error inferring Median Joining Network!";
      _g = new MedJoinNet(_goodSeqs, _mask, argument.toUInt());
      break;
    case Tcs:
      errorText = "Error inferring TCS Network!";
      _g = new TCS(_goodSeqs, _mask);
      break;
    case Apn:
      errorText = "Error inferring Ancestral Parsimony Network!";
      _g = new ParsimonyNet(_goodSeqs, _mask, _treeVect, 0, argument.toDouble());
      break;
    case Inj:
      errorText = "Error inferring Integer Neighbor-Joining Network!";
      _g = new IntNJ(_goodSeqs, _mask, argument.toUInt());
      break;
    case Tsw:
      errorText = "Error inferring Tight Span Walker network!";
      _g = new TightSpanWalker(_goodSeqs, _mask);
      break;
    default:
      break;

  }
  //_g->setupGraph();
  //_g->associateTraits(_traitVect);
  
  /*catch (NetworkError ne)
  {    
    _msgText = tr("<b>%1</b>").arg(errorText);
    _msgDetail = tr(ne.what());
    return;
  }
  
  catch (TreeError te)
  {
    _msgText = tr("<b>%1</b>").arg(errorText);
    _msgDetail = tr(te.what());
    return;
  }*/
  
  _success = true;
  // set error message in case of exception
  _msgText = tr("<b>%1</b>").arg(errorText);
  connect(_g, SIGNAL(progressUpdated(int)), _progress, SLOT(setValue(int)));
  connect(_g, SIGNAL(caughtException(const QString &)), this, SLOT(showNetError(const QString&)));
  connect(_netThread, SIGNAL(started()), _g, SLOT(setupGraph()));
  _progress->show();


  _g->moveToThread(_netThread);
  _netThread->start();


}

void HapnetWindow::printprog(int progress)
{
  cout << "progress: " << progress << endl;
}

void HapnetWindow::showNetError(const QString &msg)
{
  _success = false;
  //cout << "will display message " << msg.toStdString() << endl;
  //_msgDetail = msg;
 
  QMessageBox error;
  error.setIcon(QMessageBox::Critical);
  error.setText(_msgText);
  error.setDetailedText(msg);
  error.setStandardButtons(QMessageBox::Ok);
  error.setDefaultButton(QMessageBox::Ok);
  
  error.exec();

}

void HapnetWindow::displayNetwork()
{
  _progress->hide();

  if (_success)
  {
    //cout <<  *_g << endl;
    try
    {
      _g->associateTraits(_traitVect);
      _netModel = new NetworkModel(_g);
      /*_msgText = tr("<b>Error drawing network.</b>");
      _progress->show();

      _netView->moveToThread(_drawThread);
      _drawThread->start();*/

      _netView->setModel(_netModel);
      
      connect(_netView, SIGNAL(networkDrawn()), this, SLOT(saveNexusFile()));
    }

    catch (NetworkError &e)
    {
      showNetError(tr(e.what()));
    }
  }
  
  //toggleNetActions(true);
  //toggleAlignmentActions(true);
  //_closeAct->setEnabled(true);
  //_openAct->setEnabled(true);
  //cout << "returning from displayNetwork" << endl;
  
}

/*void HapnetWindow::setModel()
{
  _netView->setModel(_netModel);
}*/

void HapnetWindow::finaliseDisplay()
{
  toggleNetActions(true);
  toggleAlignmentActions(true);
  _closeAct->setEnabled(true);
  _openAct->setEnabled(true);
}


void HapnetWindow::changeColourTheme()
{
  bool changeMap, changeNet;

  // TODO maybe change this? Store colour theme elsewhere, but which one (map or net)?
  ColourTheme::Theme theme = ColourDialog::getColour(this, _netView->colourTheme(), 0, &changeNet, &changeMap);
  

  if (changeNet)
    _netView->setColourTheme(theme);
  if (changeMap)
    _mapView->setColourTheme(theme);
}

void HapnetWindow::changeColour(int colourIdx)
{
  QColor oldColour;
  QString dlgStr;

  if (_view == Net)
  {
    oldColour = _netView->colour(colourIdx);
    dlgStr = tr("Choose a new trait colour");
  }
  else
  {
    oldColour = _mapView->colour(colourIdx);
    dlgStr = tr("Choose a new sequence colour");
  }
  
  QColor newColour = QColorDialog::getColor(oldColour, this, dlgStr);
  
  if (newColour.isValid())
  {
    if (_view == Net)
      _netView->setColour(colourIdx, newColour);
    else
      _mapView->setColour(colourIdx, newColour);
  }
}

void HapnetWindow::changeVertexColour()
{
  const QColor & oldColour = _netView->vertexColour();
  
  QColor newColour = QColorDialog::getColor(oldColour, this, tr("Choose a new default vertex colour"));
  
  if (newColour.isValid())
  _netView->setVertexColour(newColour);
}

void HapnetWindow::changeVertexSize()
{
  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  QHBoxLayout *hlayout = new QHBoxLayout;

  QLabel *label = new QLabel("Choose a new vertex size", &dlg);
  hlayout->addWidget(label);

  QDoubleSpinBox *spinBox = new QDoubleSpinBox(&dlg);
  spinBox->setRange(1, 100);
  spinBox->setSingleStep(1);
  spinBox->setDecimals(2);
  spinBox->setValue(_netView->vertexSize());
  hlayout->addWidget(spinBox);

  vlayout->addLayout(hlayout);

  hlayout = new QHBoxLayout;

  hlayout->addStretch(1);
  QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
  connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(okButton, 0, Qt::AlignRight);
  QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
  connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(cancelButton, 0, Qt::AlignRight);

  vlayout->addLayout(hlayout);

  int result = dlg.exec();

  if (result == QDialog::Rejected)
    return;

  _netView->setVertexSize(spinBox->value());
}

void HapnetWindow::changeEdgeColour()
{
  const QColor & oldColour = _netView->edgeColour();
  
  QColor newColour = QColorDialog::getColor(oldColour, this, tr("Choose a new edge colour"));

  if (newColour.isValid())
    _netView->setEdgeColour(newColour);
}

void HapnetWindow::changeEdgeMutationView(QAction *viewAction)
{
  if (viewAction == _dashViewAct)
    _netView->setEdgeMutationView(EdgeItem::ShowDashes);

  else if (viewAction == _nodeViewAct)
    _netView->setEdgeMutationView(EdgeItem::ShowEllipses);

  else if (viewAction == _numViewAct)
    _netView->setEdgeMutationView(EdgeItem::ShowNums);
}


void HapnetWindow::toggleView()
{
  if (_view == Net)
  {
    //_mapView->show();//setVisible(true);
    //setCentralWidget(_mapView);
    //_netView->hide();//setVisible(false);
    _centralContainer->setCurrentIndex(1);

    if (! _mapTraitsSet && _traitVect.size() > 0)
    {
      _mapView->addHapLocations(_traitVect);
      _mapTraitsSet = true;
    }
    _toggleViewAct->setText(tr("Switch to network view"));
    _view = Map;
  }

  else
  {
    //_netView->show();//setVisible(true);
    //setCentralWidget(_netView);
    //_mapView->hide(); //setVisible(false);
    _centralContainer->setCurrentIndex(0);

    _toggleViewAct->setText(tr("Switch to map view"));
    _view = Net;
  }
}

void HapnetWindow::changeBackgroundColour()
{
  const QColor & oldColour = _netView->backgroundColour();
  
  QColor newColour = QColorDialog::getColor(oldColour, this, tr("Choose a new background colour"));
  
  if (newColour.isValid())
    _netView->setBackgroundColour(newColour);
}

void HapnetWindow::changeLabelFont()
{
  const QFont & oldFont = _netView->labelFont();
  
  bool ok;
  QFont newFont = QFontDialog::getFont(&ok, oldFont, this, tr("Choose a new legend font"));
  
  _netView->changeLabelFont(newFont);
}

void HapnetWindow::changeLegendFont()
{
  const QFont & oldFont = _netView->legendFont();
  
  bool ok;
  QFont newFont = QFontDialog::getFont(&ok, oldFont, this, tr("Choose a new legend font"));
  
  _netView->changeLegendFont(newFont);
}

void HapnetWindow::redrawNetwork()
{
  toggleNetActions(false);
  toggleAlignmentActions(false);
  _closeAct->setEnabled(false);
  _openAct->setEnabled(false);


  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  QHBoxLayout *hlayout = new QHBoxLayout;

  QLabel *label = new QLabel("Iterations", &dlg);
  hlayout->addWidget(label);

  QSpinBox *spinBox = new QSpinBox(&dlg);
  spinBox->setRange(100, 10 * _netView->defaultIterations());
  spinBox->setValue(_netView->defaultIterations());
  hlayout->addWidget(spinBox);

  vlayout->addLayout(hlayout);

  hlayout = new QHBoxLayout;

  hlayout->addStretch(1);
  QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
  connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(okButton, 0, Qt::AlignRight);
  QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
  connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(cancelButton, 0, Qt::AlignRight);

  vlayout->addLayout(hlayout);

  int result = dlg.exec();

  if (result == QDialog::Rejected)
    return;

  unsigned iterations = spinBox->value();

  _netView->redraw(iterations);
}

void HapnetWindow::setTraitColour()
{
  
  if (_traitVect.empty()) 
  {
    statusBar()->showMessage(tr("No traits read."));
    return;
  }
  
  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  
  QComboBox *comboBox = new QComboBox(&dlg);
  
  for (unsigned i = 0; i < _traitVect.size(); i++)
    comboBox->addItem(QString::fromStdString(_traitVect.at(i)->name()));
  
  vlayout->addWidget(comboBox);
  
  QHBoxLayout *hlayout = new QHBoxLayout;
  hlayout->addStretch(1);
  QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
  connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(okButton, 0, Qt::AlignRight);
  QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
  connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
  
  vlayout->addLayout(hlayout);
  int result = dlg.exec();
  
  if (result == QDialog::Rejected)
    return;
  
  int traitIdx = comboBox->currentIndex();
  changeColour(traitIdx);
  
}


void HapnetWindow::resizeEvent(QResizeEvent *event)
{
  emit sizeChanged(event->size());
  QMainWindow::resizeEvent(event);
}

void HapnetWindow::showIdenticalSeqs()
{
  if (! _stats)
    doStatsSetup();

  const map<Sequence, list<Sequence> > & identical = _stats->mapIdenticalSeqs();//_goodSeqs, _mask);
    
  map<Sequence, list<Sequence> >::const_iterator identicalIt = identical.begin();
   
  QStringList header("Node label");
  header.push_back("Matching sequences");
    
  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  
  vlayout->addWidget(new QLabel("Identical Sequences", this));
  
  QTreeWidget *tree = new QTreeWidget(&dlg);
  tree->setColumnCount(2);
  
  tree->setHeaderItem(new QTreeWidgetItem(header));
  
  while (identicalIt != identical.end())
  {
    if (!identicalIt->second.empty())
    {
      QStringList parentList(QString::fromStdString(identicalIt->first.name()));
      parentList.push_back(QString::fromStdString(identicalIt->first.name()));
      
      QTreeWidgetItem *parentItem = new QTreeWidgetItem(parentList);

      tree->addTopLevelItem(parentItem);
      
      list<Sequence>::const_iterator othersIt = identicalIt->second.begin();
      
      while (othersIt != identicalIt->second.end())
      {
        QStringList child(tr(""));
        child.push_back(QString::fromStdString(othersIt->name()));
        parentItem->addChild(new QTreeWidgetItem(child));
        
        ++othersIt;
      }
      
    }
    
    ++identicalIt;
  }

  tree->expandAll();
  tree->setSelectionMode(QAbstractItemView::ContiguousSelection);
  vlayout->addWidget(tree);
  vlayout->addWidget(new QLabel("Log to file?", this));

  QHBoxLayout *hlayout = new QHBoxLayout;
  hlayout->addStretch(1);
  QPushButton *yesButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogYesButton), "Yes", &dlg);
  connect(yesButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(yesButton, 0, Qt::AlignRight);
  QPushButton *noButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogNoButton), "No", &dlg);
  connect(noButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(noButton, 0, Qt::AlignRight);
  
  vlayout->addLayout(hlayout);
  
  int result = dlg.exec();
  
  if (result == QDialog::Rejected)
    return;

  QString defaultName(_filename);

  if (defaultName.endsWith(".nex", Qt::CaseInsensitive))
    defaultName.replace(defaultName.length() - 3, 3, "log"); 
  else
    defaultName.append(".log");
  
  QString filename = QFileDialog::getSaveFileName(this, "Log file", defaultName, "Log files (*.log);;All Files(*)");

  if (filename.isEmpty()) 
    return;
  QFile file(filename);
  if (!file.open(QIODevice::Append | QIODevice::Text))
    return;
  QTextStream out(&file);
  out << "Node label\tMatching Sequences\n";
  
  identicalIt = identical.begin();
  while (identicalIt != identical.end())
  {
    if (! identicalIt->second.empty())
    {
      out << QString::fromStdString(identicalIt->first.name()) << "\t" << QString::fromStdString(identicalIt->first.name()) << endl;
      list<Sequence>::const_iterator othersIt = identicalIt->second.begin();
      
      while (othersIt != identicalIt->second.end())
      {
        out << "\t" << QString::fromStdString(othersIt->name()) << "\n";
        
        ++othersIt;
      }     
    }   
    ++identicalIt;
  }
  
  file.close();
  
}

void HapnetWindow::doStatsSetup()
{
  _stats = new Statistics(_goodSeqs, _mask, _datatype);

  _progress->setLabelText("Setting up stats...");
  connect(_statThread, SIGNAL(started()), _stats, SLOT(setupStats()));
  connect(_stats, SIGNAL(progressUpdated(int)), _progress, SLOT(setValue(int)));
  _progress->show();

  _stats->moveToThread(_statThread);
  _statThread->start();

  _statThread->wait();
  if (! _traitVect.empty())  _stats->setFreqsFromTraits(_traitVect);
}

void HapnetWindow::showNucleotideDiversity()
{
  if (! _stats)
    doStatsSetup();
        
  double diversity = _stats->nucleotideDiversity();
  
  QMessageBox message;
  message.setIcon(QMessageBox::Information);
  message.setText(tr("<b>Nucleotide diversity</b>"));
  QString infText(tr("%1 = %2").arg(QChar(0x03C0)).arg(diversity));
  message.setInformativeText(infText);
  message.setStandardButtons(QMessageBox::Ok); 
  message.setDefaultButton(QMessageBox::Ok);

  message.exec();

}

void HapnetWindow::showSegSites()
{
  if (! _stats)
    doStatsSetup();

  unsigned segsites = _stats->nSegSites();
  
  QMessageBox message;
  message.setIcon(QMessageBox::Information);
  message.setText(tr("<b>Segregating sites</b>"));
  message.setInformativeText(tr("Number of sites: %1").arg(segsites));
  message.setStandardButtons(QMessageBox::Ok); 
  message.setDefaultButton(QMessageBox::Ok);

  message.exec(); 
}

void HapnetWindow::showParsimonySites()
{
  if (! _stats)
    doStatsSetup();

  unsigned psites = _stats->nParsimonyInformative();
  
  QMessageBox message;
  message.setIcon(QMessageBox::Information);
  message.setText(tr("<b>Parsimony-informative sites</b>"));
  message.setInformativeText(tr("Number of sites: %1").arg(psites));
  message.setStandardButtons(QMessageBox::Ok); 
  message.setDefaultButton(QMessageBox::Ok);

  message.exec(); 
}

void HapnetWindow::showTajimaD()
{
  if (! _stats)
    doStatsSetup();

  Statistics::stat tajimaStat = _stats->TajimaD();
  
  QMessageBox message;
  message.setIcon(QMessageBox::Information);
  message.setText(tr("<b>Tajima's D statistic</b>"));
  QString infText(tr("D = %1<br>p(D %2 %1) = %3").arg(tajimaStat.value).arg(QChar(0x2265)).arg(tajimaStat.prob));
  message.setInformativeText(infText);
  message.setStandardButtons(QMessageBox::Ok); 
  message.setDefaultButton(QMessageBox::Ok);

  message.exec(); 
}

void HapnetWindow::showAllStats()
{
  if (! _stats)
    doStatsSetup();

  double diversity = _stats->nucleotideDiversity();
  unsigned segsites = _stats->nSegSites();
  unsigned psites = _stats->nParsimonyInformative();
  Statistics::stat tajimaStat = _stats->TajimaD();
  
  QDialog dlg(this);
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);  
 
  QString divText(tr("<b>Nucleotide diversity:</b> %1 = %2").arg(QChar(0x03C0)).arg(diversity));
  vlayout->addWidget(new QLabel(divText, &dlg));
  
  QString ssText(tr("<b>Number of segregating sites:</b> %1").arg(segsites));
  vlayout->addWidget(new QLabel(ssText, &dlg));
  
  QString psText(tr("<b>Number of parsimony-informative sites:</b> %1").arg(psites));
  vlayout->addWidget(new QLabel(psText, &dlg));
  
  QString tajimaText(tr("<b>Tajima's D:</b> %1<br>p(D %2 %1) = %3").arg(tajimaStat.value).arg(QChar(0x2265)).arg(tajimaStat.prob));
  vlayout->addWidget(new QLabel(tajimaText, &dlg));
  
  vlayout->addWidget(new QLabel("Log to file?", this));

  QHBoxLayout *hlayout = new QHBoxLayout;
  hlayout->addStretch(1);
  QPushButton *yesButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogYesButton), "Yes", &dlg);
  connect(yesButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(yesButton, 0, Qt::AlignRight);
  QPushButton *noButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogNoButton), "No", &dlg);
  connect(noButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(noButton, 0, Qt::AlignRight);
  
  vlayout->addLayout(hlayout);
  
  int result = dlg.exec();
  
  if (result == QDialog::Rejected)
    return;  
  

  QString defaultName(_filename);

  if (defaultName.endsWith(".nex", Qt::CaseInsensitive))
    defaultName.replace(defaultName.length() - 3, 3, "log"); 
  else
    defaultName.append(".log");
  
  QString filename = QFileDialog::getSaveFileName(this, "Log file", defaultName, "Log files (*.log);;All Files(*)");

  if (filename.isEmpty()) 
    return;
  
  QFile file(filename);
  if (!file.open(QIODevice::Append | QIODevice::Text))
    return;
  QTextStream out(&file);
  
  out << "Nucleotide diversity:\tpi = " << diversity << endl;
  out << "Number of segregating sites:\t" << segsites << endl;
  out << "Number of parsimony-informative sites:\t" << psites << endl;
  out << "Tajima's D statistic:\tD = " << tajimaStat.value << endl;
  out << "\tp (D >= " << tajimaStat.value << ") = " << tajimaStat.prob << endl;
  
  file.close();

}

void HapnetWindow::search()
{
  QDialog dlg(this);
  
  QVBoxLayout *vlayout = new QVBoxLayout(&dlg);
  QHBoxLayout *hlayout = new QHBoxLayout;
  
  QLabel *label = new QLabel("Node label", &dlg);
  hlayout->addWidget(label);
  
  QLineEdit *edit = new QLineEdit(&dlg);
  hlayout->addWidget(edit);
  vlayout->addLayout(hlayout);
  
  hlayout = new QHBoxLayout;
  
  hlayout->addStretch(1);
  QPushButton *okButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton), "OK", &dlg);
  connect(okButton, SIGNAL(clicked()), &dlg, SLOT(accept()));
  hlayout->addWidget(okButton, 0, Qt::AlignRight);
  QPushButton *cancelButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), "Cancel", &dlg);
  connect(cancelButton, SIGNAL(clicked()), &dlg, SLOT(reject()));
  hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
  
  vlayout->addLayout(hlayout);
  
  int result = dlg.exec();
  
  if (result == QDialog::Rejected)
    return;
  
  _netView->selectNodes(edit->text());
  
}



void HapnetWindow::toggleAlignmentActions(bool enable)
{
  
  QList<QAction *> netActions = _networkMenu->actions();
  QList<QAction *>::iterator netIt = netActions.begin();
  
  while (netIt != netActions.end())
  {
    (*netIt)->setEnabled(enable);
    ++netIt;
  }
  
  _networkMenu->setEnabled(enable);

  QList<QAction *> statsActions = _statsMenu->actions();
  QList<QAction *>::iterator statsIt = statsActions.begin();
  
  while (statsIt != statsActions.end())
  {
    (*statsIt)->setEnabled(enable);
    ++statsIt;
  }
  
  _statsMenu->setEnabled(enable);

  _closeAct->setEnabled(enable);
  
}

void HapnetWindow::toggleNetActions(bool enable)
{
  _exportAct->setEnabled(enable);
  _saveGraphicsAct->setEnabled(enable);
  _exportToolAct->setEnabled(enable);
  _zoomInAct->setEnabled(enable);
  _zoomOutAct->setEnabled(enable);
  _rotateLAct->setEnabled(enable);
  _rotateRAct->setEnabled(enable);
  _searchAct->setEnabled(enable);
  _taxBoxAct->setEnabled(enable);
  _barchartAct->setEnabled(enable);
  _traitColourAct->setEnabled(enable);
  _vertexColourAct->setEnabled(enable);
  _vertexSizeAct->setEnabled(enable);
  _edgeColourAct->setEnabled(enable);
  _backgroundColourAct->setEnabled(enable);
  _labelFontAct->setEnabled(enable);
  _legendFontAct->setEnabled(enable);
  _redrawAct->setEnabled(enable);
  //_viewMenu->setEnabled(enable);
  _dashViewAct->setEnabled(enable);
  _nodeViewAct->setEnabled(enable);
  _numViewAct->setEnabled(enable);
}

void HapnetWindow::fixBarchartButton(bool taxBoxChecked)
{
  if (taxBoxChecked)
    _barchartAct->setChecked(false);
}

void HapnetWindow::fixTaxBoxButton(bool barchartChecked)
{
  if (barchartChecked)
    _taxBoxAct->setChecked(false);
}



