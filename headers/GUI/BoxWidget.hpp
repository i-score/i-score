#ifndef BOXWIDGET_H
#define BOXWIDGET_H

/*!
 * \file BoxWidget.hpp
 *
 * \author Luc Vercellin
 */

#include <QTabWidget>
#include <QComboBox>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QMap>
#include <map>
#include <vector>
#include <string>

class QGridLayout;
class CurveWidget;
class Interpolation;
class QString;

/*!
 * \class BoxWidget
 *
 * \brief Widget handling curves.
 */

class BoxWidget : public QWidget
{
    Q_OBJECT

public :
    BoxWidget(QWidget *parent);
    ~BoxWidget();
    /*!
     * \brief Updates the widget.
     */
    void applyChanges();
    /*!
     * \brief Update curves from messages handled by a box.
     *
     * \param boxID : the box handling messages
     */
    void updateMessages(unsigned int boxID, bool forceUpdate);

    bool contains(const std::string &address);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    void displayCurveWindow(CurveWidget *curve);
    virtual void paintEngine();

    void curveActivationChanged(const QString &address,bool state);
    void curveRedundancyChanged(const QString &address,bool state);
    void curveSampleRateChanged(const QString &address,int value);
    void curveShowChanged(const QString &address,bool state);
    void updateCurve(const std::string &address);
    void removeCurve(const std::string &address);
    void setComboBox(QComboBox *cbox);    
    inline QStackedLayout *stackedLayout(){return _stackedLayout;}
    inline QComboBox *comboBox(){return _comboBox;}
    inline void setStackedLayout(QStackedLayout *slayout){_stackedLayout = slayout;setLayout(_stackedLayout);}

public slots :
    void displayCurve(const QString &address);

signals :
    void currentIndexChanged();

private :
    bool updateCurve(const std::string &address, bool forceUpdate);
    void addCurve(QString address,CurveWidget *curveWidget);
    void addToComboBox(const QString address);
    void clearCurves();
    QComboBox *_comboBox;
    QTabWidget *_tabWidget;
    Interpolation *_interpolation;
    std::map<std::string,unsigned int> _curveIndexes; //!< Map of curves tabs' indexes mapped by their addresses
//    std::map<std::string,CurveWidget *> _curveMap; //!< Map of curves tabs mapped by their addresses
    QMap<std::string,CurveWidget *> *_curveMap; //!< Map of curves tabs mapped by their addresses
    unsigned int _width;
    unsigned int _height;
    unsigned int _boxID;
    QWidget *_parentWidget;
    QList <CurveWidget *> *_curveWidgetList;
    QTabWidget *_curve;
    QGridLayout *_curvePageLayout;
    CurveWidget *_curveWidget;
    QStackedLayout *_stackedLayout;
};

#endif // BOXWIDGET_H
