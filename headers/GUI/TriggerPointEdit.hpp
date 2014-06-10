#ifndef TRIGGERPOINTEDIT_HPP
#define TRIGGERPOINTEDIT_HPP

#include <QPushButton>
#include <QDialog>
#include <QLineEdit>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include "AbstractTriggerPoint.hpp"

class TriggerPointEdit : public QDialog
{
    Q_OBJECT

public :
    TriggerPointEdit(AbstractTriggerPoint *abstract, QWidget *parent = 0);
    ~TriggerPointEdit();

public slots :
    void updateStuff();
    void expressionChanged();
    void autoTriggerChanged();
    void edit();

private :
    AbstractTriggerPoint    *_abstract;

    bool                    _expressionChanged,
                            _autoTriggerChanged;

    QPushButton             *_okButton;
    QPushButton             *_cancelButton;
    QLabel                  *_expressionLabel;
    QLineEdit               *_expressionEdit;
    QCheckBox               *_autoTriggerCheckBox;

    QGridLayout             *_layout;
};

#endif // TRIGGERPOINTEDIT_HPP