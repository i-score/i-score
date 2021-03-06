/*
 * Copyright © 2013, Nicolas Hincker
 *
 * License: This code is licensed under the terms of the "CeCILL-C"
 * http://www.cecill.info
 */
#include "HeaderPanelWidget.hpp"

HeaderPanelWidget::HeaderPanelWidget(QWidget *parent, MaquetteScene *scene)
    : QWidget(parent){

    _scene = scene;
    _color = QColor(60,60,60);
    _sliderMoved = false;
    _valueEntered = false;
    _nameLabel = new QLabel;
    _toolBar = new QToolBar;
    _accelerationWidget = new QWidget;

    createAccelerationWidget();
    createActions();
    createToolBar();
    createNameLabel();
    createLayout();

    setGeometry(0, 0, width(), HEADER_HEIGHT);
    setFixedHeight(HEADER_HEIGHT);
    setPalette(QPalette(_color));
    setAutoFillBackground(true);
}

HeaderPanelWidget::~HeaderPanelWidget()
{
    _toolBar->deleteLater();
    _nameLabel->deleteLater();
    _playAction->deleteLater();
    _stopAction->deleteLater();
    _rewindAction->deleteLater();
    _accelerationSlider->deleteLater();
    _accelerationDisplay->deleteLater();
    _accelerationWidget->deleteLater();
    _layout->deleteLater();
}

void
HeaderPanelWidget::createAccelerationWidget()
{
  QHBoxLayout *layout = new QHBoxLayout;
  layout->setSpacing(0);

  QSizePolicy *ignoredPolicy = new QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);

  _accelerationSlider = new LogarithmicSlider(Qt::Horizontal, this);
  _accelerationSlider->setStatusTip(tr("Speed"));
  _accelerationSlider->setFixedWidth(200);
  _accelerationSlider->setSliderPosition(50);
  _accelerationSlider->setSizePolicy(*ignoredPolicy);

  _accelerationDisplay = new QDoubleSpinBox(this);
_accelerationDisplay->setFixedHeight(17);
_accelerationDisplay->setAlignment(Qt::AlignVCenter);

QFont font;
font.setPointSize(9);
_accelerationDisplay->setFont(font);

  _accelerationDisplay->setStyleSheet(
              "QDoubleSpinBox{"
              "height: 13px;"
              "color: lightgray;"
              "background-color: gray;"
              "selection-color: black;"
              "selection-background-color: (170,100,100);"
              "}"
              "QDoubleSpinBox::up-button  {"
              "width: 0px;"
              "height: 0px;"
              "}"
              "QDoubleSpinBox::down-button {"
              "width: 0px;"
              "height: 0px;"
              "}"
              );
  _accelerationDisplay->setStatusTip(tr("Speed"));
  _accelerationDisplay->setRange(0., 5.);  
  _accelerationDisplay->setDecimals(2);
  _accelerationDisplay->setValue(_accelerationSlider->accelerationValue(_accelerationSlider->value()));
  _accelerationDisplay->setKeyboardTracking(false);
  _accelerationDisplay->setSizePolicy(*ignoredPolicy);

  layout->addWidget(_accelerationSlider);
  layout->addWidget(_accelerationDisplay);  
  _accelerationWidget->setLayout(layout);

  connect(_accelerationSlider, SIGNAL(valueChanged(int)), this, SLOT(accelerationValueModified(int)));
  connect(_accelerationDisplay, SIGNAL(valueChanged(double)), this, SLOT(accelerationValueEntered(double)));
  connect(_scene, SIGNAL(accelerationValueChanged(double)), this, SLOT(accelerationValueEntered(double)));
}

void
HeaderPanelWidget::createActions()
{
  QPixmap pix = QPixmap(35, 35);
  pix.fill(Qt::transparent);

  QIcon playIcon(":/resources/images/play.png");
  _playAction = new QAction(playIcon, tr("Play"), this);
  _playAction->setStatusTip(tr("Play scenario"));

  QIcon stopIcon(":/resources/images/stop.png");
  _stopAction = new QAction(stopIcon, tr("Stop"), this);
  _stopAction->setStatusTip(tr("Stop scenario"));

  QIcon rewindIcon(":/resources/images/rewind.png");
  _rewindAction = new QAction(rewindIcon, tr("Rewind"), this);
  _rewindAction->setStatusTip(tr("Rewind scenario"));

  QIcon loopOffIcon(":/resources/images/loop.png");
  _setLoopAction = new QAction(loopOffIcon, tr("Loop"), this);
  _setLoopAction->setStatusTip(tr("Loop scenario"));

  QIcon loopOnIcon(":/resources/images/loopOn.png");
  _unsetLoopAction = new QAction(loopOnIcon, tr("stop Loop"), this);
  _unsetLoopAction->setStatusTip(tr("Don't Loop scenario"));


  connect(_playAction, SIGNAL(triggered()), this, SLOT(play()));
  connect(_stopAction, SIGNAL(triggered()), this, SLOT(stop()));
  connect(_rewindAction, SIGNAL(triggered()), this, SLOT(rewind()));
  connect(_setLoopAction, SIGNAL(triggered()), this, SLOT(loop()));
  connect(_unsetLoopAction, SIGNAL(triggered()), this, SLOT(loop()) );
}

void
HeaderPanelWidget::createToolBar()
{
  _toolBar->setStyleSheet("QToolBar{border :none; }"
                          "QToolBar::handle {"
                          "border: none;"
                          "}"
                          "QToolBar:top, QToolBar:bottom {"
                          "border :none; background :grey;"
                          "}"
                          "QToolBar:left, QToolBar:right {"
                          "border :none;background :grey;"
                          "}"
                          );

  _toolBar->addAction(_rewindAction);
  _toolBar->addAction(_playAction);
  _toolBar->addAction(_stopAction);
  _toolBar->addAction(_setLoopAction);
  _toolBar->addAction(_unsetLoopAction);

  _unsetLoopAction->setVisible(false);
  _stopAction->setVisible(false);
  _toolBar->raise();
}

void
HeaderPanelWidget::createNameLabel()
{
  QFont font;
  font.setPointSize(NAME_POINT_SIZE);
  _nameLabel->setFont(font);
//  _nameLabel->setText("<font color='Blue'>Some text</font>");
  _nameLabel->setText("Scenario");
}

void
HeaderPanelWidget::createLayout(){
    setGeometry(0, 0, width(), HEADER_HEIGHT);
    setFixedHeight(HEADER_HEIGHT);
    setPalette(QPalette(_color));
    setAutoFillBackground(true);

    _layout = new QBoxLayout(QBoxLayout::LeftToRight);
    _layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    _layout->addWidget(_toolBar);
    _layout->addWidget(_nameLabel);
    _layout->addWidget(_accelerationWidget);

    _layout->insertSpacing(_layout->count(), 20);

    _layout->insertStretch(_layout->indexOf(_accelerationWidget), _scene->width() / 2);
    _layout->setAlignment(_accelerationSlider, Qt::AlignRight);
    _layout->setContentsMargins(0, 0, 0, 0);

    setLayout(_layout);
}

void
HeaderPanelWidget::play(){
    
    _scene->playOrResume();
    
    updatePlayMode();
}

void
HeaderPanelWidget::stop(){
    
    _scene->stopOrPause();
    
    updatePlayMode();
}

void
HeaderPanelWidget::rewind(){
    
    _scene->stopAndGoToStart();
    
    updatePlayMode();
}

void HeaderPanelWidget::loop()
{
    Maquette::getInstance()->setBoxLoopState(ROOT_BOX_ID, !Maquette::getInstance()->getBoxLoopState(ROOT_BOX_ID));
    _setLoopAction->setVisible(!_setLoopAction->isVisible());
    _unsetLoopAction->setVisible(!_unsetLoopAction->isVisible());
}

void
HeaderPanelWidget::accelerationValueModified(int value){
    if (!_valueEntered) {
        emit(accelerationValueChanged(value));

        double newValue = _accelerationSlider->accelerationValue(value);

        if (_accelerationDisplay->value() != newValue) {
            _sliderMoved = true;
            _accelerationDisplay->setValue(newValue);
          }
      }
    _valueEntered = false;
}

void
HeaderPanelWidget::accelerationValueEntered(double value){
    if (!_sliderMoved) {
        int newValue = _accelerationSlider->valueForAcceleration(value);
        _accelerationDisplay->setValue(value);
        if (newValue < LogarithmicSlider::MAXIMUM_VALUE) {
            _accelerationSlider->setValue(newValue);
          }
        else {
            _valueEntered = true;
            _accelerationSlider->setValue(_accelerationSlider->valueForAcceleration(LogarithmicSlider::MAXIMUM_VALUE));
          }
      }
    _scene->setAccelerationFactor(value);
    _sliderMoved = false;
}

void
HeaderPanelWidget::updatePlayMode(){

    bool playing = _scene->playing();

    _stopAction->setVisible(playing);
    _playAction->setVisible(!playing);
}

void
HeaderPanelWidget::setName(QString name)
{
  _nameLabel->setStyleSheet("QLabel { color : gray; } ");
  _nameLabel->setText(name);
}

void
HeaderPanelWidget::mouseReleaseEvent(QMouseEvent *event){
    QWidget::mouseReleaseEvent(event);

    //Set attributes of the main scenario
    _scene->setAttributes(static_cast<AbstractBox *>(Maquette::getInstance()->getBox(1)->abstract()));
    _scene->unselectAll();
}
