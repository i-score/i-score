/*
 * Copyright: LaBRI / SCRIME / L'Arboretum
 *
 * Authors: Pascal Baltazar, Nicolas Hincker, Luc Vercellin and Myriam Desainte-Catherine (as of 16/03/2014)
 *
 *iscore.contact@gmail.com
 *
 * This software is an interactive intermedia sequencer.
 * It allows the precise and flexible scripting of interactive scenarios.
 * In contrast to most sequencers, i-score doesn’t produce any media,
 * but controls other environments’ parameters, by creating snapshots
 * and automations, and organizing them in time in a multi-linear way.
 * More about i-score on http://www.i-score.org
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#include <iostream>
#include <utility>
#include <algorithm>
#include <math.h>

#include <QPainter>
#include <QTabWidget>
#include <QGridLayout>
#include <QMouseEvent>
#include <QBrush>
#include <QToolTip>
#include <QGraphicsOpacityEffect>
#include <MaquetteScene.hpp>
#include <QDebug>
using std::map;
using std::string;
using std::vector;
using std::pair;
using std::make_pair;

#include "BasicBox.hpp"
#include "CurveWidget.hpp"
#include "AbstractCurve.hpp"
#include "Maquette.hpp"
#include "MaquetteScene.hpp"
#include "AttributesEditor.hpp"
#include "NetworkTree.hpp"

#define BORDER_WIDTH 2.

CurveWidget::CurveWidget(QWidget *parent) : QWidget(parent)
{
	init();
}

CurveWidget::CurveWidget(QWidget *parent, AbstractCurve *abCurve) : QWidget(parent)
{  
	_abstract = abCurve;
	init();
	curveRepresentationOutdated();
}

CurveWidget::~CurveWidget() {}

void
CurveWidget::init()
{
	_abstract = new AbstractCurve(NO_ID, "", 0, 10, false, true, true, 1, vector<float>(), map<float, pair<float, float> >());
	
	setCursor(Qt::CrossCursor);
	setMouseTracking(true);
	
	_layout = new QGridLayout;
	_layout->setAlignment(Qt::AlignCenter);
	
	_scaleX = 1;
	_scaleY = 1;
	
	_interspace = width() - BORDER_WIDTH;
	
	_clicked = false;
	_unactive = false;
	_minYModified = false;
	_maxYModified = false;
	
	_movingBreakpointX = -1;
	_movingBreakpointY = -1;
	
	_lastPointSelected = false;
	_lastPowSave = 1.;
	setLayout(_layout);
	_xAxisPos = height() / 2.;
	
	_minYTextRect = new QRectF(0.,_xAxisPos,40.,10.);
	_maxYTextRect = new QRectF(0.,0.,40.,10.);
	
	_minRangeBoundLocked = false;
	_maxRangeBoundLocked = false;
}

AbstractCurve *
CurveWidget::abstractCurve()
{
	return _abstract;
}

void
CurveWidget::curveRepresentationOutdated()
{
	float maxCurveElement = *(std::max_element(_abstract->_curve.begin(), _abstract->_curve.end()));
	float minCurveElement = *(std::min_element(_abstract->_curve.begin(), _abstract->_curve.end()));
	
	
	std::vector<float> range;
	Maquette::getInstance()->getRangeBounds(_abstract->address(), range);
	
	float margin = std::abs(range[1]) * 0.1 + std::abs(range[2]) * 0.1; 
	if(maxCurveElement > _maxY)
	{
		_maxY = _maxRangeBoundLocked? std::min(maxCurveElement, range[1] + margin) : maxCurveElement;
	}
	if(minCurveElement < _minY)
	{
		_minY = _minRangeBoundLocked? std::max(minCurveElement, range[0] - margin) : minCurveElement;
	}
	
	//abscissa at the box middle only if the curve contains negative elements
	if (_minY >= 0.) 
	{
		_xAxisPos = height() - BORDER_WIDTH;
	}
	else 
	{
		_xAxisPos = (height() - BORDER_WIDTH) / 2.;
	}
	
	_interspace = (width() - BORDER_WIDTH) / (float)(std::max((unsigned int)2, (unsigned int)(_abstract->_curve.size())) - 1);
	
	float halfSizeY = std::max(fabs(_maxY), fabs(_minY));
	_scaleY = (_xAxisPos - BORDER_WIDTH) / halfSizeY;
	// qDebug() <<"Scale: " << _scaleY;
	update();
}

void CurveWidget::adaptScale()
{
	_maxY = *(std::max_element(_abstract->_curve.begin(), _abstract->_curve.end()));
	_minY = *(std::min_element(_abstract->_curve.begin(), _abstract->_curve.end()));
	
	const double min_spacing{0.1};
	if(std::abs(_maxY - _minY) < min_spacing)
	{
		_minY = -min_spacing;
		_maxY = min_spacing;
	}
	_xAxisPos = (_minY >= 0.)? 
					(height() - BORDER_WIDTH) : 
					(height() - BORDER_WIDTH) / 2.;
	
	_interspace = (width() - BORDER_WIDTH) / (float)(std::max((unsigned int)2, 
															  (unsigned int)(_abstract->_curve.size())) - 1);
	
	float halfSizeY = std::max(fabs(_maxY), fabs(_minY));
	_scaleY = 2 * (_xAxisPos - BORDER_WIDTH) / (2 * halfSizeY);
	
	update();
}


void
CurveWidget::setAttributes(unsigned int boxID,
						   const std::string &address,
						   unsigned int argPosition,
						   const vector<float> &values,
						   unsigned int sampleRate,
						   bool redundancy,
						   bool show,
						   bool interpolate,
						   const vector<string> &/*argType*/,
						   const vector<float> &xPercents,
						   const vector<float> &yValues,
						   const vector<short> &sectionType,
						   const vector<float> &coeff)
{
	Q_UNUSED(argPosition);
	Q_UNUSED(sectionType);
	
	_abstract->_boxID = boxID;
	_abstract->_curve.clear();
	_abstract->_breakpoints.clear();
	_abstract->_sampleRate = sampleRate;
	
	_abstract->_redundancy = redundancy;
	_abstract->_show = show;
	_abstract->_interpolate = interpolate;
	_abstract->_address = address;
	
	vector<float>::const_iterator it;
	for (it = values.begin(); it != values.end(); ++it) {
		_abstract->_curve.push_back(*it);
	}
	
	for (unsigned int i = 0; i < xPercents.size(); ++i) {
		_abstract->_breakpoints[xPercents[i] / 100.] = pair<float, float>(yValues[i], coeff[i]);
	}
	
	curveRepresentationOutdated();
}

void
CurveWidget::setAttributes(unsigned int boxID,
						   const std::string &address,
						   unsigned int /*argPosition*/,
						   const vector<float> &values,
						   unsigned int sampleRate,
						   bool redundancy,
						   bool show,
						   bool interpolate,
						   const vector<string> &argType,
						   const vector<float> &xPercents,
						   const vector<float> &yValues,
						   const vector<short> &sectionType,
						   const vector<float> &coeff,
						   const float minY,
						   const float maxY)
{    
	_minY = minY;
	_maxY = maxY;
	setAttributes(boxID, address, 0, values, sampleRate, redundancy, show, interpolate, argType, xPercents, yValues, sectionType, coeff);
	updateRangeClipMode();
}

void
CurveWidget::setAttributes(AbstractCurve *abCurve)
{    
	_abstract = abCurve;
	curveRepresentationOutdated();
}

QPointF
CurveWidget::relativeCoordinates(const QPointF &point)
{
	float pointX = point.x();
	float scaledX = pointX / (float)_scaleX;
	float translatedX = scaledX;
	float finalX = std::max((float)0., std::min((float)1., translatedX / width()));
	
	float pointY = point.y();
	pointY = pointY < -1 ? -log(-pointY) : pointY;
	pointY = pointY > height() + 1 ? height() + log(pointY) : pointY;
	float translatedY = pointY - _xAxisPos;
	float symetricalY = -translatedY;
	float finalY = symetricalY / (float)(_scaleY);
	
	return QPointF(finalX, finalY);
}

QPointF
CurveWidget::absoluteCoordinates(const QPointF &point)
{
	float pointX = point.x();
	float unpercentX = pointX * width();
	float scaledX = unpercentX * (float)_scaleX;
	float finalX = scaledX;
	
	float pointY = point.y();
	float scaledY = pointY * (float)_scaleY;
	float symetricalY = -scaledY;
	float finalY = symetricalY + _xAxisPos;
	
	return QPointF(finalX, finalY);
}

void
CurveWidget::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);
	
	_clicked = true;
	_shiftModifierWasEnabled = (event->modifiers() == Qt::ShiftModifier);
	
	switch (event->modifiers()) {
		case Qt::ShiftModifier:
		{
			map<float, pair<float, float> >::iterator it;
			QPointF relativePoint = relativeCoordinates(event->pos());
			
			for (it = _abstract->_breakpoints.begin(); it != _abstract->_breakpoints.end(); ++it) {
				if (fabs(it->first - relativePoint.x()) < 0.01) {
					_movingBreakpointX = it->first;
					_movingBreakpointY = -1;
					break;
				}
			}
			
			break;
		}
			
		case Qt::ControlModifier:
		{
			break;
		}
			
		case Qt::NoModifier:
		{
			map<float, pair<float, float> >::iterator it;
			bool found =  false;
			QPointF relativePoint = relativeCoordinates(event->pos());
			
			for (it = _abstract->_breakpoints.begin(); it != _abstract->_breakpoints.end(); ++it)
			{
				if (fabs(it->first - relativePoint.x()) < 0.01)
				{
					found = true; //existing breakpoint
					_movingBreakpointX = it->first;
					_movingBreakpointY = it->second.first;
					_lastPowSave = it->second.second;
					map<float, pair<float, float> >::iterator it;
					if ((it = _abstract->_breakpoints.find(_movingBreakpointX)) != _abstract->_breakpoints.end())
					{
						_abstract->_breakpoints.erase(it);
					}
					_savedMap = _abstract->_breakpoints;
					curveChanged();
					update();
					break;
				}
			}
			
			if (!found)
			{ //new breakpoint
				_abstract->_breakpoints[relativePoint.x()] = std::make_pair<float, float>(relativePoint.y(), 1.);
				for (it = _abstract->_breakpoints.begin(); it != _abstract->_breakpoints.end(); ++it)
				{
					if (fabs(it->first - relativePoint.x()) < 0.01)
					{
						_movingBreakpointX = it->first;
						_movingBreakpointY = it->second.first;
						_lastPowSave = it->second.second;
						map<float, pair<float, float> >::iterator it;
						if ((it = _abstract->_breakpoints.find(_movingBreakpointX)) != _abstract->_breakpoints.end())
						{
							_abstract->_breakpoints.erase(it);
						}
						_savedMap = _abstract->_breakpoints;
						curveChanged();
						update();
						break;
					}
				}
			}
			mouseMoveEvent(event);
			break;
		}
			
		case Qt::AltModifier: //delete breakpoint
		{
			map<float, pair<float, float> >::iterator it;
			QPointF relativePoint = relativeCoordinates(event->pos());
			for (it = _abstract->_breakpoints.begin(); it != _abstract->_breakpoints.end(); ++it) {
				if (fabs(it->first - relativePoint.x()) < 0.01) {
					_abstract->_breakpoints.erase(it);
					_movingBreakpointX = -1.;
					_movingBreakpointY = -1.;
					curveChanged();
					update();
					break;
				}
			}
			break;
		}
	}
}

void
CurveWidget::mouseMoveEvent(QMouseEvent *event)
{    
	QWidget::mouseMoveEvent(event);
	
	std::vector<float> range;
	Maquette::getInstance()->getRangeBounds(_abstract->address(), range);
	
	// Draw cursor coordinates as a tooltip
	QPointF relativePoint = relativeCoordinates(event->pos());

	if(!_minRangeBoundLocked)
	{
		range[0] = std::numeric_limits<int>::min() + 1;
	}
	
	if(!_maxRangeBoundLocked)
	{
		range[1] = std::numeric_limits<int>::max() - 1;
	}
	
	if(relativePoint.y() < range[0])
		relativePoint.setY(range[0]);
	if(relativePoint.y() > range[1])
		relativePoint.setY(range[1]);
	
	QString posStr = QString("%1 ; %2").arg(relativePoint.x(), 0, 'f', 3).arg(relativePoint.y(), 0, 'f', 3);
	Maquette::getInstance()->scene()->displayMessage(posStr.toStdString(), INDICATION_LEVEL);
	
	// Handle interactions
	if (_clicked) 
	{
		switch (event->modifiers())
		{
			case Qt::ShiftModifier: // POW
			{
				if (_movingBreakpointX != -1)
				{
					map<float, pair<float, float> >::iterator it;
					
					if ((it = _abstract->_breakpoints.find(_movingBreakpointX)) != _abstract->_breakpoints.end())
					{
						auto it2 = it;
						float previousYval = (--it2)->second.first;
						bool under = it->second.first <= previousYval;
						float mousePosY = relativePoint.y();
						float pow = 1.;
						float min = 100;
						float div;
						float rigidity = 2;
						float ratio = std::max<double>(0.1, fabs(std::log(it->second.first)));
						float ratioModifier( under ? 0.5f : 1.0f );
						
						if (mousePosY > it->second.first)
						{ // mouse under : pow between 0 and 1
							div = std::min((double)min, (double)std::max(fabs(_maxY), fabs(_minY)));
							pow = std::max(1 - std::min((mousePosY - it->second.first)/(rigidity*ratio*ratioModifier), min) / (double)div, 0.01);
						}
						else if (it->second.first > mousePosY)
						{ // mouse above : pow between 1 and 6
							div = std::min<double>((double)min, std::max(fabs(_maxY), fabs(_minY))) / 10;
							pow = 1 + std::min((it->second.first - mousePosY)/(rigidity*ratio), min) / div;
						}
						
						if (under)
						{
							pow = 1.0 / pow;
						}
						
						it->second = std::make_pair(it->second.first, pow);
						_movingBreakpointY = -1;
						curveChanged();
					}
				}
				break;
			}
				
			case Qt::ControlModifier: // Draw mode
			{
				_abstract->_breakpoints[relativePoint.x()] = std::make_pair<float, float>(relativePoint.y(), 1.);
				curveChanged();
				
				break;
			}
				
			case Qt::NoModifier: //move
			{
				_movingBreakpointX = relativePoint.x();
				_movingBreakpointY = relativePoint.y();
				
				_abstract->_breakpoints = _savedMap;
				
				_abstract->_breakpoints[static_cast<qreal>(relativePoint.x())] = 
						std::make_pair<float, float>(static_cast<qreal>(_movingBreakpointY),
													 static_cast<qreal>(_lastPowSave));
				curveChanged();
				update();
				
				
				break;
			}
				
			case Qt::AltModifier: //delete breakpoints
			{
				map<float, pair<float, float> >::iterator it;
				
				QPointF relativePoint = relativeCoordinates(event->pos());
				for (it = _abstract->_breakpoints.begin(); it != _abstract->_breakpoints.end(); ++it) 
				{
					if (fabs(it->first - relativePoint.x()) < 0.02) 
					{
						_abstract->_breakpoints.erase(it);
						_movingBreakpointX = -1.;
						_movingBreakpointY = -1.;
						curveChanged();
						update();
						break;
					}
				}
				break;
			}
				
			default:
			{
				_movingBreakpointX = -1;
				_movingBreakpointY = -1;
				curveChanged();
				update();
				break;
			}
		}
		
	}
	update();
}

void
CurveWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);
	
	if (_clicked) {
		if (event->modifiers() == Qt::NoModifier && !_shiftModifierWasEnabled) {
			QPointF relativePoint = relativeCoordinates(event->pos());
			
			if(relativePoint.y() > _maxY)
			{
				if(_maxRangeBoundLocked)
				{
					_movingBreakpointX = relativePoint.x();
					_movingBreakpointY = _maxY;
					
					curveChanged();
					update();
				}
				else
				{
					Maquette::getInstance()->scene()->displayMessage(tr("Value clipped (high range clipMode)").toStdString(), 
																	 INDICATION_LEVEL);
				}
				
			}
			else
			{
				if(relativePoint.y() < _minY)
				{
					if(_minRangeBoundLocked)
					{
						_movingBreakpointX = relativePoint.x();
						_movingBreakpointY = _minY;
						curveChanged();
						update();
					}
					else
					{
						Maquette::getInstance()->scene()->displayMessage(tr("Value clipped (low range clipMode)").toStdString(), 
																		 INDICATION_LEVEL);
					}
				}
			}
			map<float, pair<float, float> >::iterator it;
			
			if ((it = _abstract->_breakpoints.find(_movingBreakpointX)) != _abstract->_breakpoints.end()) {
				_abstract->_breakpoints.erase(it);
			}
			_abstract->_breakpoints[static_cast<qreal>(relativePoint.x())] = 
					std::make_pair<float, float>(static_cast<qreal>(_movingBreakpointY),
												 static_cast<qreal>(_lastPowSave));
			curveChanged();
			update();
			
		}
	}
	
	_clicked = false;
	_movingBreakpointX = -1.;
	_movingBreakpointY = -1.;
	_lastPointSelected = false;
	_shiftModifierWasEnabled = false;
	
	update();
}

bool
CurveWidget::curveChanged()
{
	vector<float> xPercents;
	vector<float> yValues;
	vector<short> sectionType;
	vector<float> coeff;
	map<float, pair<float, float> >::iterator it;
	
	for (it = _abstract->_breakpoints.begin(); it != _abstract->_breakpoints.end(); ++it) {
		xPercents.push_back(it->first * 100);
		yValues.push_back(it->second.first);
		
		coeff.push_back(it->second.second);
		sectionType.push_back(CURVE_POW);
	}
	
	if (Maquette::getInstance()->setCurveSections(_abstract->_boxID, 
												  _abstract->_address, 
												  0, 
												  xPercents, 
												  yValues, 
												  sectionType, 
												  coeff)) {
		unsigned int sampleRate;
		bool redundancy, interpolate;
		vector<string> argTypes;
		vector<float> values;
		xPercents.clear();
		yValues.clear();
		sectionType.clear();
		coeff.clear();
		
		if (Maquette::getInstance()->getCurveAttributes(_abstract->_boxID, 
														_abstract->_address, 
														0, 
														sampleRate, 
														redundancy, 
														interpolate, 
														values, 
														argTypes, 
														xPercents, 
														yValues, 
														sectionType, 
														coeff)) {
			setAttributes(_abstract->_boxID, 
						  _abstract->_address, 
						  0, 
						  values, 
						  sampleRate, 
						  redundancy, 
						  interpolate, 
						  _abstract->_show, 
						  argTypes, 
						  xPercents, 
						  yValues, 
						  sectionType, 
						  coeff);
			
			update();
			return true;
		}
		else {
			std::cerr << "CurveWidget::curveChanged() : getting attributes failed" << std::endl;
		}
	}
	else {
		std::cerr << "CurveWidget::curveChanged() : setting curve sections failed" << std::endl;
	}
	
	return false;
}

void
CurveWidget::applyChanges()
{
	update();
}

void
CurveWidget::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	curveRepresentationOutdated();
}

void CurveWidget::paintEvent(QPaintEvent * /* event */)
{        
	QPainter *painter = new QPainter(this);
	painter->setRenderHint(QPainter::Antialiasing, true);
	static const QColor BASE_COLOR(Qt::black);
	static const QColor AXE_COLOR(Qt::black);
	
	QColor EXTREMITY_COLOR(isEnabled()? Qt::red : Qt::gray);
	QColor CURVE_COLOR(isEnabled()? Qt::darkRed : Qt::gray);
	QColor BREAKPOINT_COLOR(isEnabled()? Qt::blue : Qt::gray);
	
	static const QColor MOVING_BREAKPOINT_COLOR(Qt::darkBlue);
	static const QColor UNACTIVE_COLOR(Qt::darkGray);
	
	// Abcisses line
	QPen penXAxis((_unactive) ? UNACTIVE_COLOR : AXE_COLOR);
	
	painter->setPen(penXAxis);
	painter->drawLine(0, _xAxisPos, width(), _xAxisPos);
	
	painter->setPen(BASE_COLOR);
	
	vector<float>::iterator it;
	map<float, pair<float, float> >::iterator it2;
	float pointSizeX = 6;
	float pointSizeY = 6;
	QPointF curPoint(0, 0);
	QPointF precPoint(-1, -1);
	
	unsigned int i = 0;
	
	for (it = _abstract->_curve.begin(); it != _abstract->_curve.end(); ++it) 
	{
		curPoint = absoluteCoordinates(QPointF(1, *it));
		curPoint.setX(i * _interspace * _scaleX);
		
		if (it == _abstract->_curve.begin()) 
		{   // First point is represented by a specific color
			painter->fillRect(QRectF(curPoint - QPointF(pointSizeX / 2., pointSizeY / 2.), QSizeF(pointSizeX, pointSizeY)), EXTREMITY_COLOR);
		}
		
		if (precPoint != QPointF(-1, -1)) 
		{
			QPen pen(_unactive ? UNACTIVE_COLOR : CURVE_COLOR);
			pen.setWidth(_unactive ? 1 : 2);
			
			painter->setPen(pen);
			painter->drawLine(precPoint, curPoint);  // Draw lines between values
			
			painter->setPen(BASE_COLOR);
		}
		
		precPoint = curPoint;
		i++;
	}
	
	// Last point is represented by a specific color
	if (!_unactive) 
	{
		painter->fillRect(QRectF(curPoint - QPointF(pointSizeX / 2., pointSizeY / 2.), 
								 QSizeF(pointSizeX, pointSizeY)), 
						  EXTREMITY_COLOR);
		
		precPoint = QPointF(-1, -1);
		for (it2 = _abstract->_breakpoints.begin(); it2 != _abstract->_breakpoints.end(); ++it2) 
		{
			curPoint = absoluteCoordinates(QPointF(it2->first, it2->second.first));
			
			// Breakpoints are drawn with rectangles
			painter->fillRect(QRectF(curPoint - QPointF(pointSizeX / 2., pointSizeY / 2.), 
									 QSizeF(pointSizeX, pointSizeY)), 
							  _unactive ? UNACTIVE_COLOR : BREAKPOINT_COLOR);
			precPoint = curPoint;
		}
		
		if (_movingBreakpointX != -1 && _movingBreakpointY != -1) 
		{
			QPointF cursor = absoluteCoordinates(QPointF(_movingBreakpointX, _movingBreakpointY));
			
			// If a breakpoint is currently being moved, it is represented by a rectangle
			painter->fillRect(QRectF(cursor - QPointF(pointSizeX / 2., pointSizeY / 2.), 
									 QSizeF(pointSizeX, pointSizeY)), 
							  _abstract->_interpolate ? MOVING_BREAKPOINT_COLOR : UNACTIVE_COLOR);
		}
	}
	
	//text : minY, maxY
	if(_minYModified || _maxYModified)
	{
		painter->save();
		QFont textFont;
		textFont.setPointSize(9.);
		painter->setFont(textFont);
		painter->setPen(QPen(Qt::black));
		if(_minYModified)
		{
			painter->drawText(*_minYTextRect,QString("%1").arg(_minY));
			_minYModified = false;
		}
		else if(_maxYModified)
		{
			painter->drawText(*_maxYTextRect,QString("%1").arg(_maxY));
			_maxYModified = false;
		}
		painter->restore();
	}
	
	delete painter;
}

void
CurveWidget::setLowerStyle(bool state)
{    
	_unactive = state;
	repaint();
}

void
CurveWidget::setMinY(float value){
	_minY = value;
	curveRepresentationOutdated();
}

void
CurveWidget::setMaxY(float value){
	_maxY = value;
	curveRepresentationOutdated();
}

void
CurveWidget::updateRangeClipMode(){
	vector<string> attributesValues;
	
	if(Maquette::getInstance()->requestObjectAttribruteValue(_abstract->_address,"rangeClipmode",attributesValues) > 0){
		string rangeClipMode = attributesValues[0];
		
		if(rangeClipMode == "none"){
			_maxRangeBoundLocked = false;
			_minRangeBoundLocked = false;
			return;
		}
		if(rangeClipMode == "low"){
			_maxRangeBoundLocked = false;
			_minRangeBoundLocked = true;
			return;
		}
		if(rangeClipMode == "high"){
			_maxRangeBoundLocked = true;
			_minRangeBoundLocked = false;
			return;
		}
		if(rangeClipMode == "both"){
			_maxRangeBoundLocked = true;
			_minRangeBoundLocked = true;
			return;
		}
	}
}

