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

#include "NetworkTree.hpp"
#include "Maquette.hpp"
#include "MainWindow.hpp"
#include <QList>
#include <map>
#include <vector>
#include <string>
#include <exception>
#include <QTreeView>
#include <QByteArray>
#include <QMessageBox>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QTreeView>
#include <QApplication>
#include <DelayedDelete.h>
#include <utility>
#include <QDebug>

int NetworkTree::NAME_COLUMN = 0;
int NetworkTree::VALUE_COLUMN = 1;
int NetworkTree::START_ASSIGNATION_COLUMN = 2;
int NetworkTree::START_COLUMN = 3;
int NetworkTree::INTERPOLATION_COLUMN = 4;
int NetworkTree::END_ASSIGNATION_COLUMN = 5;
int NetworkTree::END_COLUMN = 6;
int NetworkTree::REDUNDANCY_COLUMN = 7;
int NetworkTree::SR_COLUMN = 8;
int NetworkTree::TYPE_COLUMN = 9;
int NetworkTree::MIN_COLUMN = 10;
int NetworkTree::MAX_COLUMN = 11;
unsigned int NetworkTree::PRIORITY_COLUMN = 12;

const QColor NetworkTree::TEXT_COLOR = QColor(200, 200, 200);
const QColor NetworkTree::TEXT_DISABLED_COLOR = QColor(100, 100, 100);

QString NetworkTree::OSC_ADD_NODE_TEXT = QString("Add a node");
QString NetworkTree::ADD_A_DEVICE_TEXT = QString("Add a device");

unsigned int NetworkTree::TEXT_POINT_SIZE = 10;




////////////////////////////////////////////////////////////////////////////////
// Node types
////////////////////////////////////////////////////////////////////////////////

struct ItemProperties { virtual void setup(QTreeWidgetItem*) = 0; };

struct DeviceProperties : public ItemProperties
{
        virtual void setup(QTreeWidgetItem* curItem) override
        {
            curItem->setForeground(0, Qt::white);
        }
};

struct NodeProperties : public ItemProperties
{
        virtual void setup(QTreeWidgetItem* curItem) override
        {
            curItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        }
};
struct LeafProperties : public ItemProperties
{
        virtual void setup(QTreeWidgetItem* curItem) override
        {
            curItem->setCheckState(NetworkTree::INTERPOLATION_COLUMN, Qt::Unchecked);
            curItem->setToolTip(NetworkTree::INTERPOLATION_COLUMN, "check to create an automation - <br> cmd/ctrl click to record <br> a live input");
            curItem->setCheckState(NetworkTree::REDUNDANCY_COLUMN, Qt::Unchecked);
            curItem->setToolTip(NetworkTree::REDUNDANCY_COLUMN, "check to repeat successive similar values");


            curItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        }
};

struct ParameterProperties : public ItemProperties
{
        virtual void setup(QTreeWidgetItem* curItem) override
        {
            curItem->setText(NetworkTree::TYPE_COLUMN,QString("<->"));
            curItem->setToolTip(NetworkTree::TYPE_COLUMN, "bi-directionnal");

            curItem->setForeground(NetworkTree::NAME_COLUMN, Qt::white);
        }
};

struct MessageProperties : public ItemProperties
{
        virtual void setup(QTreeWidgetItem* curItem) override
        {
            curItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);

            QFont curFont = curItem->font(NetworkTree::NAME_COLUMN);
            curFont.setItalic(true);
            curItem->setFont(NetworkTree::NAME_COLUMN,curFont);

            QBrush brush(Qt::lightGray);
            curItem->setForeground(NetworkTree::NAME_COLUMN, brush);
            curItem->setForeground(NetworkTree::VALUE_COLUMN, brush);

            curItem->setText(NetworkTree::TYPE_COLUMN,QString("->"));
            curItem->setToolTip(NetworkTree::TYPE_COLUMN, "receiver");
            curItem->setWhatsThis(NetworkTree::NAME_COLUMN,"Message");
        }
};

struct ReturnProperties : public ItemProperties
{
        virtual void setup(QTreeWidgetItem* curItem) override
        {
            curItem->setFlags(Qt::ItemIsDropEnabled);
            QFont curFont = curItem->font(NetworkTree::NAME_COLUMN);
            curFont.setItalic(true);
            curItem->setFont(NetworkTree::NAME_COLUMN,curFont);

            QBrush brush(Qt::black);
            curItem->setForeground(NetworkTree::NAME_COLUMN, brush);
            curItem->setForeground(NetworkTree::VALUE_COLUMN, brush);

            curItem->setText(NetworkTree::TYPE_COLUMN,QString("<-"));
            curItem->setToolTip(NetworkTree::TYPE_COLUMN, "sender");

        }
};

struct PresetManagerProperties : public ItemProperties
{
        virtual void setup(QTreeWidgetItem * curItem) override
        {
            curItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);

            QFont curFont = curItem->font(NetworkTree::NAME_COLUMN);
            curFont.setItalic(true);
            curItem->setFont(NetworkTree::NAME_COLUMN,curFont);

            QBrush brush(Qt::lightGray);
            curItem->setForeground(NetworkTree::NAME_COLUMN, brush);
            curItem->setForeground(NetworkTree::VALUE_COLUMN, brush);

            curItem->setText(NetworkTree::TYPE_COLUMN, QString("->"));
            curItem->setToolTip(NetworkTree::TYPE_COLUMN, "preset");
            curItem->setWhatsThis(NetworkTree::NAME_COLUMN,"Message");
        }
};

//===//
class NetworkTreeItem : public QTreeWidgetItem
{
    public:
        template<typename... Args>
        NetworkTreeItem(Args&&... a):
            QTreeWidgetItem(std::forward<Args>(a)...)
        {
            // Default parameters for a node
            QFont curFont = font(NetworkTree::NAME_COLUMN);
            curFont.setItalic(false);
            setFont(NetworkTree::NAME_COLUMN,curFont);

            for(int i = 0; i <= 12; i++)
                setForeground(i, Qt::white);

            setCheckState(NetworkTree::START_ASSIGNATION_COLUMN, Qt::Unchecked);
            setToolTip(NetworkTree::START_ASSIGNATION_COLUMN, "check to snapshot the value of this address <br> at the beginning of the selected box");
            setCheckState(NetworkTree::END_ASSIGNATION_COLUMN, Qt::Unchecked);
            setToolTip(NetworkTree::END_ASSIGNATION_COLUMN, "check to snapshot the value of this address <br> at the end of the selected box");

        }

        void setupProperties() {}
        template<typename Prop, typename... Properties>
        void setupProperties(Prop&& p, Properties&&... props)
        {
            p.setup(this);
            setupProperties(std::forward<Properties>(props)...);
        }
};

////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
// Some helper functions
////////////////////////////////////////////////////////////////////////////////
bool checkPredicateInTree(QTreeWidgetItem* parent, std::function<bool(QTreeWidgetItem*)> fun)
{
	bool found = false;
	for( int i = 0; i < parent->childCount(); ++i )
	{
		if(fun(parent->child(i))) return true;
		else found |= checkPredicateInTree(parent->child(i), fun);
	}

	return found;
}

void applyInTree(QTreeWidgetItem* parent, std::function<void(QTreeWidgetItem*)> fun)
{
	for( int i = 0; i < parent->childCount(); ++i )
	{
		fun(parent->child(i));
		applyInTree(parent->child(i), fun);
	}
}


bool isOSC(QTreeWidgetItem* item)
{
	std::string protocol;
    Maquette::getInstance()->getDeviceProtocol(item->text(0).toStdString(),protocol);
    
    return protocol == "OSC";
}




////////////////////////////////////////////////////////////////////////////////
// Tree
////////////////////////////////////////////////////////////////////////////////

#include <QHeaderView>
NetworkTree::NetworkTree(QWidget *parent) : QTreeWidget(parent)
{
  class CustomHeaderView : public QHeaderView {
  public:
    CustomHeaderView(Qt::Orientation o): QHeaderView(o) {}
    QSize sizeHint() const override { return {500, 15}; }
  };

  this->setHeader(new CustomHeaderView(Qt::Horizontal));
  init();

  setColumnCount(12);
  QStringList list;
  list << "Address" << "Value" << "   v" <<"Start" << " ~ " << "   v" <<"End" << " = " << " % "<<" access "<<"min "<<"max ";
  // removed <<"priority " and column
  setColumnWidth(NAME_COLUMN, 135);
  setColumnWidth(VALUE_COLUMN, 63);
  setColumnWidth(START_ASSIGNATION_COLUMN, 30);
  setColumnWidth(START_COLUMN, 60);
  setColumnWidth(END_ASSIGNATION_COLUMN, 30);
  setColumnWidth(END_COLUMN, 60);
  setColumnWidth(INTERPOLATION_COLUMN, 23);
  setColumnWidth(REDUNDANCY_COLUMN, 23);
  setColumnWidth(SR_COLUMN, 31);
  setColumnWidth(TYPE_COLUMN, 42);
  setColumnWidth(MIN_COLUMN, 42);
  setColumnWidth(MAX_COLUMN, 42);
  setColumnWidth(PRIORITY_COLUMN, 30);  

  setIndentation(13);
  setHeaderLabels(list);

  list.clear();
  QFont font;
  font.setPointSize(TEXT_POINT_SIZE);
  setFont(font);

  setTabKeyNavigation(true);

  VALUE_MODIFIED = false;
  SR_MODIFIED = false;
  NAME_MODIFIED = false;
  MIN_MODIFIED = false;
  MAX_MODIFIED = false;
  hideColumn(VALUE_COLUMN);  
  
  connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(clickInNetworkTree(QTreeWidgetItem *, int)));
  connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(valueChanged(QTreeWidgetItem*, int)));
  connect(this, SIGNAL(startValueChanged(QTreeWidgetItem*, QString)), this, SLOT(changeStartValue(QTreeWidgetItem*, QString)));
  connect(this, SIGNAL(endValueChanged(QTreeWidgetItem*, QString)), this, SLOT(changeEndValue(QTreeWidgetItem*, QString)));
  
  connect(_deviceEdit, SIGNAL(deviceChanged(QString)), 
		  this,		   SLOT(refreshCurrentItemNamespace()), Qt::DirectConnection);
  connect(_deviceEdit, SIGNAL(deviceNameChanged(QString,QString)), 
		  this,		   SLOT(updateDeviceName(QString, QString)), Qt::DirectConnection);
  connect(_deviceEdit, SIGNAL(deviceProtocolChanged(QString)), 
		  this,		   SLOT(updateDeviceProtocol(QString)), Qt::DirectConnection);
  connect(_deviceEdit, SIGNAL(newDeviceAdded(QString)), 
		  this,		   SLOT(addNewDevice(QString)), Qt::DirectConnection);
  connect(_deviceEdit, SIGNAL(namespaceLoaded(QString)), 
		  this,		   SLOT(updateDeviceNamespace(QString)), Qt::DirectConnection);
  
  connect(_deviceEdit, &DeviceEdit::disableTree,
		  this,		   &NetworkTree::disable, Qt::DirectConnection);
  connect(_deviceEdit, &DeviceEdit::enableTree,
		  this,		   &NetworkTree::enable, Qt::DirectConnection);

  connect(this,        &NetworkTree::deviceUpdated,
          this,        &NetworkTree::refreshItemNamespace);

  _addADeviceItem = addADeviceNode();
  addTopLevelItem(_addADeviceItem);
}

NetworkTree::~NetworkTree()
{
    _startMessages->deleteLater();
    _endMessages->deleteLater();
    _OSCStartMessages->deleteLater();
    _OSCEndMessages->deleteLater();
    //delete _addADeviceItem;
}

/****************************************************************************
*                          General tools
****************************************************************************/
void
NetworkTree::init()
{

    _treeFilterActive = true;
    _deviceEdit = new DeviceEdit(topLevelWidget());

    _startMessages = new NetworkMessages;
    _endMessages = new NetworkMessages;
    _OSCMessageCount = 0;
    _OSCStartMessages = new NetworkMessages;
    _OSCEndMessages = new NetworkMessages;

    setStyleSheet(
                "QTreeView {"
                "show-decoration-selected: 1;"
                "background-color: #5a5a5a;"
                "}"

                "QTreeView::item {"
                "border-right: 1px solid #000000;"
                "border-top-color: transparent;"
                "border-bottom-color: transparent;"
                "}"

                "QTreeView::item:disabled {"
                "background: transparent;"
                "}"
                
                "QTreeView::item:hover {"
                "background: qlineargradient(x1: -5, y1: 0, x2: 0, y2: 1, stop: 0 #e7effd, stop: 1 #cbdaf1);"
                "color: #5a5a5a;"
                "border: 1px solid #bfcde4;"
                "}"

                "QTreeView::item:selected {"
                "border: 1px solid #567dbc;"
                "}"

                "QTreeView::item:selected:active{"
                "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6ea1f1, stop: 1 #567dbc);"
                "}"

                "QTreeView::item:selected:!active {"
                "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6b9be8, stop: 1 #577fbf);"
                "}"

                "  QHeaderView::section {"
                "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                "stop:0 #a1a1a1, stop: 0.5 #909090,"
                "stop: 0.6 #808080, stop:1 #a3a3a3);"
                "color: black;"
                "padding-left: 1px;"
                "padding-top: 2px;"
                "padding-bottom: 1px;"
                "border: 1px solid #606060;"
                "}"
                );
	
	setSelectionMode(QAbstractItemView::SingleSelection);
}

void
NetworkTree::clear()
{
  QList<QTreeWidgetItem*>::iterator it;

  _addressMap.clear();
  _nodesWithSelectedChildren.clear();
  _assignedItems.clear();
  _nodesWithSomeChildrenAssigned.clear();
  _nodesWithAllChildrenAssigned.clear();

  _startMessages->clear();
  _endMessages->clear();
  _OSCStartMessages->clear();
  _OSCEndMessages->clear();
  _recMessages.clear();

  _OSCMessageCount = 0;

  QTreeWidget::clear();
}

void NetworkTree::enable()
{
	setEnabled(true);
}

void NetworkTree::disable()
{
	setEnabled(false);
}

void
NetworkTree::load()
{
  vector<string> deviceNames;
  Maquette::getInstance()->getNetworkDeviceNames(deviceNames);

  vector<string>::iterator nameIt;
  QList<QTreeWidgetItem*> itemsList;
  _addADeviceItem = addADeviceNode();
  string protocol;

  for (nameIt = deviceNames.begin(); nameIt != deviceNames.end(); ++nameIt) {      

      QString deviceName = QString::fromStdString(*nameIt);
      QTreeWidgetItem *curItem = new QTreeWidgetItem(DeviceNode);
      curItem->setText(NAME_COLUMN , deviceName);
      curItem->setCheckState(NAME_COLUMN,Qt::Unchecked);
      treeRecursiveExploration(curItem, true);
      itemsList << curItem;

      Maquette::getInstance()->getDeviceProtocol(deviceName.toStdString(),protocol);
      if(protocol=="OSC")
          createOSCBranch(curItem);
    }

  itemsList<<_addADeviceItem;
  addTopLevelItems(itemsList);  
  
  setSelectionMode(QAbstractItemView::SingleSelection);
}

/*
 * Idée : Chercher l'item dans l'arbre à partir du nom du msg (ex : MinuitDevice1/groupe2/controle2 4294967318)
 *      On découpe le nom jusqu'à "espace"  de telle sorte que : vector<string> hierarchy = { MinuitDevice1 ; groupe2 ; controle2 }
 *      On fait findItems de controle2 (dernier de la liste),  puis on remonte hierarchy en vérifiant à chaque fois le père du itemFound
 *
 */
QList< QPair<QTreeWidgetItem *, Message> >
NetworkTree:: getItemsFromMsg(vector<string> itemsName)
{
  Message msg;
  QList<QTreeWidgetItem *>  itemsFound;
  QString curName;
  QStringList address;
  QStringList splitAddress;
  QList< QPair<QTreeWidgetItem *, Message> > itemsMatchedList;
  vector<string>::iterator it;

  if (!itemsName.empty()) {
      //Boucle sur liste message
      for (it = itemsName.begin(); it != itemsName.end(); ++it) {
          curName = QString::fromStdString(*it);
          address = curName.split(" ");

          splitAddress = address.first().split("/");
          int nbMembers = address.size();
          int nbSection = splitAddress.size();
          if (nbSection >= 2) {
              curName = address.first();
              msg.value = address.at(1);  //second value
              if (nbMembers > 1) { //pour les listes
                  for (int i = 2; i < nbMembers; i++) {
                      msg.value += QString(tr(" ") + address.at(i));
                    }
                }
              msg.device = curName.section('/', 0, 0);
              msg.message = tr("/");
              msg.message += curName.section('/', 1, nbSection);
            }

          itemsFound = this->findItems(splitAddress.last(), Qt::MatchRecursive, 0);
          if (itemsFound.size() > 1) {
              QList<QTreeWidgetItem *>::iterator it3;
              QTreeWidgetItem *curIt;
              QTreeWidgetItem *father;
              bool found = false;
              for (it3 = itemsFound.begin(); it3 != itemsFound.end(); ++it3) {
                  curIt = *it3;
                  int i = splitAddress.size() - 2;
                  while (curIt->parent() != nullptr) {
                      father = curIt->parent();
                      if (father->text(0) != splitAddress.at(i)) {
                          found = false;
                          break;
                        }
                      else {
                          found = true;
                          curIt = father;
                          i--;
                        }
                    }
                  if (found == true) {
                      QPair<QTreeWidgetItem *, Message> newPair = qMakePair(*it3, msg);
                      itemsMatchedList << newPair;
                      break;
                    }
                }
            }
          else {
              if (!itemsFound.isEmpty()) {
                  QPair<QTreeWidgetItem *, Message> newPair = qMakePair(itemsFound.first(), msg);
                  itemsMatchedList << newPair;
                }
              else { //No item in tree
                  std::cout << "NetworkTree::getItemsFromMsg : " << curName.toStdString() << " not found" << std::endl;
                }
            }
        }
    }
  return itemsMatchedList;
}

void NetworkTree::addOSCMessage(QTreeWidgetItem *rootNode)
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Input an OSC Message"),
                                               tr("OSC message: "),
                                               QLineEdit::Normal,
                                               "",
                                               &ok);

    if(ok && !text.isEmpty())
    {
        auto devicename = getAbsoluteAddress(rootNode);
        auto fullname = devicename + "/" + text;

        std::vector<QString> addrsBefore;
        applyInTree(rootNode, [&] (QTreeWidgetItem* item)
        {
            addrsBefore.push_back(getAbsoluteAddress(item));
        });

        // Ajout du device par simulation de learn
        Maquette::getInstance()->setDeviceLearn(devicename.toLatin1().constData(), true);
        Maquette::getInstance()->appendToNetWorkNamespace(fullname.toStdString());
        Maquette::getInstance()->setDeviceLearn(devicename.toLatin1().constData(), false);

        // Expansion des items nouvellement ajoutés
        std::vector<QString> addrsAfter;
        applyInTree(rootNode, [&] (QTreeWidgetItem* item)
        {
            addrsAfter.push_back(getAbsoluteAddress(item));
        });

        QList<QTreeWidgetItem*> toExpand;
        for(auto& addr : addrsAfter)
        {
            if(std::find(std::begin(addrsBefore),
                         std::end(addrsBefore),
                         addr) == std::end(addrsBefore))
            {
                auto itm = getItemFromAddress(addr.toStdString());
                if(itm)
                    toExpand.push_back(itm);
            }
        }

        expandItems(toExpand);

    }
}

void NetworkTree::removeOSCMessage(QTreeWidgetItem* item)
{
    Maquette::getInstance()->removeFromNetWorkNamespace(getAbsoluteAddress(item).toStdString());
}

void
NetworkTree::loadNetworkTree(AbstractBox *abBox)
{ 
  QList< QPair<QTreeWidgetItem *, Message> > startItemsAndMsgs = getItemsFromMsg(Maquette::getInstance()->firstMessagesToSend(abBox->ID()));
  QList< QPair<QTreeWidgetItem *, Message> > endItemsAndMsgs = getItemsFromMsg(Maquette::getInstance()->lastMessagesToSend(abBox->ID()));

  QList< QPair<QTreeWidgetItem *, Message> >::iterator it0;
  QPair<QTreeWidgetItem *, Message> curPair;

  QMap<QTreeWidgetItem *, Data>itemsFromMsg;
  Data currentData;
  for (it0 = startItemsAndMsgs.begin(); it0 != startItemsAndMsgs.end(); it0++) {
      curPair = *it0;
      itemsFromMsg.insert(curPair.first, currentData);
    }

  QList<QTreeWidgetItem *>itemsFromEndMsg;
  for (it0 = endItemsAndMsgs.begin(); it0 != endItemsAndMsgs.end(); it0++) {
      curPair = *it0;
      itemsFromMsg.insert(curPair.first, currentData);
    }

  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;
  for (it = itemsFromEndMsg.begin(); it != itemsFromEndMsg.end(); it++) {
      curItem = *it;
      if (!itemsFromMsg.contains(curItem)) {
          itemsFromMsg.insert(curItem, currentData);
        }
    }
  setAssignedItems(itemsFromMsg);
  NetworkMessages *startMsg = new NetworkMessages();
  NetworkMessages *endMsg = new NetworkMessages();

  startMsg->setMessages(startItemsAndMsgs);
  endMsg->setMessages(endItemsAndMsgs);
  setStartMessages(startMsg);
  setEndMessages(endMsg);
  
  setSelectionMode(QAbstractItemView::SingleSelection);
}


void
NetworkTree::createOSCBranch(QTreeWidgetItem *curItem)
{
	// Check if current branch does not have a "Add a node"
	bool already_exists = checkPredicateInTree(curItem, 
											   [] (QTreeWidgetItem* item) 
							{ return item->type() == addOSCNode;});
	if(already_exists) return;
	
	// If so, add it
	QTreeWidgetItem *addANodeItem = new QTreeWidgetItem(QStringList(OSC_ADD_NODE_TEXT), addOSCNode);
	addANodeItem->setFlags(Qt::ItemIsEnabled);
	addANodeItem->setIcon(0, QIcon(":/resources/images/addANode.png"));
    curItem->addChild(addANodeItem);
}

QTreeWidgetItem *
NetworkTree::addADeviceNode()
{
  QTreeWidgetItem *addADeviceNode = new QTreeWidgetItem(QStringList(ADD_A_DEVICE_TEXT), addDeviceNode);
  addADeviceNode->setFlags(Qt::ItemIsEnabled);
  addADeviceNode->setIcon(0, QIcon(":/resources/images/addANode.png"));
  return addADeviceNode;
}

QTreeWidgetItem *
NetworkTree::addDeviceItem(QString name)
{
    auto newItem = new NetworkTreeItem(DeviceNode);
    newItem->setText(NAME_COLUMN,name);
    newItem->setupProperties(DeviceProperties());

    insertTopLevelItem(topLevelItemCount()-1, newItem);

    return newItem;
}

QString
NetworkTree::getAbsoluteAddress(QTreeWidgetItem *item) const
{
  QString address;
  QTreeWidgetItem * curItem = item;
  while (curItem != nullptr) {
      QString node;
      if (curItem->parent() != nullptr && !curItem->text(NAME_COLUMN).startsWith("/")) {
          node.append("/");
        }
      node.append(curItem->text(NAME_COLUMN));
      address.insert(NAME_COLUMN, node);
      curItem = curItem->parent();
    }
  return address;
}

QString
NetworkTree::getDeviceName(QTreeWidgetItem *item) const
{
  QString deviceName;

  QTreeWidgetItem * curItem = item;
  while (curItem->parent() != nullptr) {
      curItem = curItem->parent();
    }

  deviceName = curItem->text(NAME_COLUMN);
  return deviceName;
}

QTreeWidgetItem *
NetworkTree::getItemFromAddress(string address) const
{
  return _addressMap.key(address);
}

QPair< QMap <QTreeWidgetItem *, Data>, QList<QString> >
NetworkTree::treeSnapshot(unsigned int boxID)
{
  Q_UNUSED(boxID);

  QMap<QTreeWidgetItem *, Data> snapshots;
  QList<QString> devicesConcerned;

  QList<QTreeWidgetItem*> selection = assignedItems().keys();
  if (!selection.empty()) {
      QList<QTreeWidgetItem*>::iterator it;
      vector<string>::iterator it2;
      QTreeWidgetItem *curItem;
      for (it = selection.begin(); it != selection.end(); ++it) {
          curItem = *it;
          if (curItem->type() != DeviceNode && curItem->type() != NodeNoNamespaceType){
              QString address = getAbsoluteAddress(*it);

              //get device concerned
              QString deviceName = getDeviceName(*it);
              if (!devicesConcerned.contains(deviceName)) {
                  devicesConcerned.append(deviceName);
                }

              QPair<QTreeWidgetItem *, Data> curPair;

              if (!address.isEmpty()) {                  
                  vector<string> snapshot = Maquette::getInstance()->requestNetworkSnapShot(address.toStdString());

                  Data data;
                  for (it2 = snapshot.begin(); it2 != snapshot.end(); it2++) {
                      data.msg = QString::fromStdString(*it2);
                      data.address = address;
//                        data.sampleRate = Maquette::getInstance()->getCurveSampleRate(boxID,address.toStdString());
                      data.hasCurve = false;
                      snapshots.insert(*it, data);
                    }
                }
            }
        }
    }

  return qMakePair(snapshots, devicesConcerned);
}

QPair< QMap <QTreeWidgetItem *, Data>, QList<QString> >
NetworkTree::treeSnapshot(unsigned int boxID, QList<QTreeWidgetItem *> itemsList)
{
  Q_UNUSED(boxID);

  QMap<QTreeWidgetItem *, Data> snapshots;
  QList<QString> devicesConcerned;

  QList<QTreeWidgetItem*> selection = itemsList;
  if (!selection.empty()) {
      QList<QTreeWidgetItem*>::iterator it;
      vector<string>::iterator it2;
      QTreeWidgetItem *curItem;
      for (it = selection.begin(); it != selection.end(); ++it) {
          curItem = *it;
          if (curItem->type() != DeviceNode && curItem->type() != NodeNoNamespaceType){
              QString address = getAbsoluteAddress(*it);

              //get device concerned
              QString deviceName = getDeviceName(*it);
              if (!devicesConcerned.contains(deviceName)) {
                  devicesConcerned.append(deviceName);
                }

              QPair<QTreeWidgetItem *, Data> curPair;

              if (!address.isEmpty()) {
                  vector<string> snapshot = Maquette::getInstance()->requestNetworkSnapShot(address.toStdString());

                  Data data;
                  for (it2 = snapshot.begin(); it2 != snapshot.end(); it2++) {
                      data.msg = QString::fromStdString(*it2);
                      data.address = address;
//                        data.sampleRate = Maquette::getInstance()->getCurveSampleRate(boxID,address.toStdString());
                      data.hasCurve = false;
                      snapshots.insert(*it, data);
                    }
                }
            }
        }
    }

  return qMakePair(snapshots, devicesConcerned);
}

bool
NetworkTree::hasStartEndMsg(QTreeWidgetItem *item)
{
  return(_startMessages->getMessages().contains(item) || _endMessages->getMessages().contains(item));
}

bool
NetworkTree::hasStartMsg(QTreeWidgetItem *item){
    return _startMessages->getMessages().contains(item);
}

bool
NetworkTree::hasEndMsg(QTreeWidgetItem *item){
    return _endMessages->getMessages().contains(item);
}

/****************************************************************************
*                          General display tools
****************************************************************************/

void
NetworkTree::treeRecursiveExploration(QTreeWidgetItem *curItem, bool conflict)
{
    if (!curItem->isDisabled()) {

         vector<string>            children;
         string                    nodeType,
                                   address = (getAbsoluteAddress(curItem)).toStdString();

         //TOTO : check if necessary (unused for the moment) NH.
         // A tester plus en avant
         auto preexistingkeys = _addressMap.keys(address);
         for(auto& key : preexistingkeys) _addressMap.remove(key);

         _addressMap.insert(curItem, address);

         //Get object's children
         if(Maquette::getInstance()->getObjectChildren(address,children) > 0)
         {
             for(const auto& child : children)
             {
                 QStringList name{QString::fromStdString(child)};
                 NetworkTreeItem *childItem{};
                 string childAbsoluteAddress = address;
                 childAbsoluteAddress.append("/");
                 childAbsoluteAddress.append(child);

                 if(Maquette::getInstance()->getObjectType(childAbsoluteAddress,nodeType) && nodeType == "Data")
                 {
                     childItem = new NetworkTreeItem(curItem, name, LeaveType);
                     childItem->setupProperties(LeafProperties());
                 }
                 else
                 {
                     childItem = new NetworkTreeItem(curItem, name, NodeNoNamespaceType);
                     childItem->setupProperties(NodeProperties());
                 }

                 treeRecursiveExploration(childItem, conflict);
                 setNewItemProperties(childItem);
             }
         }
     }
}

void NetworkTree::setNewItemProperties(NetworkTreeItem* curItem)
{
    // Get the required properties from Maquette
    auto address = (getAbsoluteAddress(curItem)).toStdString();

    std::vector<std::string> tagsValues;
    auto tagsResult = Maquette::getInstance()->requestObjectAttribruteValue(address,"tags",tagsValues);

    std::vector<std::string> servicesValues;
    auto servicesResult = Maquette::getInstance()->requestObjectAttribruteValue(address,"service",servicesValues);

    std::string nodeType;
    Maquette::getInstance()->getObjectType(address,nodeType);



    //Gets priority
    unsigned int priority = 0;
    if(!Maquette::getInstance()->getPriority(address,priority))
    {
        curItem->setText(PRIORITY_COLUMN,QString("%1").arg(priority));
    }

    // Filtering
    if(treeFilterActive())
    {
        bool toDelete =    (nodeType == "Model" ||
                            nodeType == "ModelInfo" ||
                            nodeType == "Input.audio" ||
                            nodeType == "Output.audio" ||
                            nodeType == "Viewer")
                        || (tagsResult > 0 && !tagsValues.empty() && tagsValues[0] == "setup")
                        || (servicesResult > 0 && nodeType == "Container" && !servicesValues.empty() && servicesValues[0] == "view" );

        if(toDelete)
        {
            delete curItem;
            return;
        }

        if(nodeType == "PresetManager")
        {
            curItem->setupProperties(PresetManagerProperties());
            return;
        }
    }

    if(servicesResult > 0 && nodeType != "Container")
    {
        curItem->setCheckState(INTERPOLATION_COLUMN, Qt::Unchecked);
        curItem->setCheckState(REDUNDANCY_COLUMN, Qt::Unchecked);
        curItem->setCheckState(START_ASSIGNATION_COLUMN, Qt::Unchecked);
        curItem->setCheckState(END_ASSIGNATION_COLUMN, Qt::Unchecked);

        if(!servicesValues.empty())
        {
            if(servicesValues[0] == "return")
            {
                curItem->setupProperties(ReturnProperties());
                return;
            }

            if(servicesValues[0] == "message")
            {
                curItem->setupProperties(MessageProperties());
                return;
            }

            if(servicesValues[0] == "parameter")
            {
                curItem->setupProperties(ParameterProperties());
            }
        }

        curItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable);
    }

    //Get range bounds
    vector<float> rangeBounds;
    if(Maquette::getInstance()->getRangeBounds(address,rangeBounds)>0)
    {
        curItem->setText(MIN_COLUMN,QString("%1").arg(rangeBounds[0]));
        curItem->setToolTip(MIN_COLUMN, curItem->text(MIN_COLUMN));
        curItem->setText(MAX_COLUMN,QString("%1").arg(rangeBounds[1]));
        curItem->setToolTip(MAX_COLUMN, curItem->text(MAX_COLUMN));
    }
}

void
NetworkTree::clearColumn(unsigned int column, bool fullCleaning)
{        
  if (!_assignedItems.isEmpty()) {
      QList<QTreeWidgetItem *>::iterator it;
      QTreeWidgetItem *curIt;
      QList<QTreeWidgetItem *> assignedItems = _assignedItems.keys();
      QString emptyString;
      emptyString.clear();

      for (it = assignedItems.begin(); it != assignedItems.end(); it++) {
          curIt = *it;
          if(curIt->checkState(column))
              curIt->setCheckState(column, Qt::Unchecked);

          if(!fullCleaning){
              if(curIt->whatsThis(NAME_COLUMN)=="Message") { }
          }
          else
              curIt->setText(column, emptyString);
        }      
    }

  //clear record icon
  if(int(column) == INTERPOLATION_COLUMN && !_recMessages.isEmpty()){
      QList<QTreeWidgetItem *>::iterator it;
      for(it = _recMessages.begin(); it != _recMessages.end() ; it++)
          (*it)->setCheckState(column, Qt::Unchecked);
  }
}

void
NetworkTree::clearStartMsgs()
{
  clearColumn(START_COLUMN);
  _startMessages->clear();
}

void
NetworkTree::clearDevicesStartMsgs(QList<QString> devices)
{
  clearColumn(START_COLUMN,false);
  _startMessages->clearDevicesMsgs(devices);
}

void
NetworkTree::clearEndMsgs()
{
  clearColumn(END_COLUMN);
  _endMessages->clear();
}

void
NetworkTree::clearDevicesEndMsgs(QList<QString> devices)
{
  clearColumn(END_COLUMN,false);
  _endMessages->clearDevicesMsgs(devices);
}

void
NetworkTree::displayBoxContent(AbstractBox *abBox)
{
  auto map = abBox->startMessages()->getMessages();

  setStartMessages(abBox->startMessages());
  setEndMessages(abBox->endMessages());
  updateStartMsgsDisplay();
  updateEndMsgsDisplay();
  assignItems(assignedItems());
  setRecMode(abBox->messagesToRecord());

  //Expand items
  QList<QTreeWidgetItem *> expandedItems = abBox->networkTreeExpandedItems();
  QList<QTreeWidgetItem *> selectedItems = abBox->getNetworkTreeSelectedItems();

  if(selectedItems.isEmpty() && expandedItems.isEmpty())
  {
    expandedItems = getExpandedItems();
    abBox->setNetworkTreeExpandedItems(expandedItems);;
  }
  if(abBox->justCreated()){ //Items are not collapsed at each new box. We save the tree current state.
      expandedItems = getExpandedItems();
      abBox->setNetworkTreeExpandedItems(expandedItems);
  }
  else
      expandItems(expandedItems);


  for(int i=0; i<selectedItems.size(); i++)
      setItemSelected(selectedItems.at(i),true);
}

void
NetworkTree::updateStartOSCMsgsDisplay()
{
  QList<QTreeWidgetItem *> items = _OSCStartMessages->getItems();
  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;
  Message currentMsg;
  QFont font;
  font.setBold(true);

  for (it = items.begin(); it != items.end(); it++) {
      curItem = *it;
      currentMsg = _OSCStartMessages->getMessages().value(curItem);
      curItem->setText(START_COLUMN, currentMsg.value);
      curItem->setFont(NAME_COLUMN, font);
    }
}

void
NetworkTree::updateEndOSCMsgsDisplay()
{
  QList<QTreeWidgetItem *> items = _OSCEndMessages->getItems();
  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;
  Message currentMsg;
  QFont font;
  font.setBold(true);

  for (it = items.begin(); it != items.end(); it++) {
      curItem = *it;
      currentMsg = _OSCEndMessages->getMessages().value(curItem);
      curItem->setText(END_COLUMN, currentMsg.value);
      curItem->setFont(NAME_COLUMN, font);
    }
}

void
NetworkTree::updateStartMsgsDisplay()
{
  QList<QTreeWidgetItem *> items = _startMessages->getItems();
  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;
  Message currentMsg;

//    clearColumn(START_COLUMN);
  for (it = items.begin(); it != items.end(); it++) {
      curItem = *it;
      currentMsg = _startMessages->getMessages().value(curItem);            
      curItem->setText(START_COLUMN, currentMsg.value);
      curItem->setCheckState(START_ASSIGNATION_COLUMN, Qt::Checked);
      fatherColumnCheck(curItem, START_ASSIGNATION_COLUMN);
    }
}

void
NetworkTree::fatherColumnCheck(QTreeWidgetItem *item, int column)
{
  if (item->parent() != nullptr) {
      QTreeWidgetItem *father = item->parent();

      if (allBrothersChecked(item, column)) {
          father->setCheckState(column, Qt::Checked); //Check box OK
        }
      else {
          if (brothersPartiallyChecked(item, column)) { //PartialCheck
              father->setCheckState(column, Qt::PartiallyChecked);
            }
          else { //No check
              father->setCheckState(column, Qt::Unchecked);
            }
        }
      fatherColumnCheck(father, column);
  }
}

void NetworkTree::removeCurrentNode()
{
    if(currentItem() != nullptr){
        QString itemName = getAbsoluteAddress(currentItem());
        int ret = QMessageBox::warning(this, QString("Delete %1").arg(itemName),
                                        QString("Do you really want to delete %1 ?").arg(itemName),
                                        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel );
        switch (ret) {
            case QMessageBox::Yes:{
                removeOSCMessage(currentItem());
                currentItem()->parent()->removeChild(currentItem());
                break;
            }
            case QMessageBox::Cancel:{
                break;
            }
        }
    }
}

void
NetworkTree::updateEndMsgsDisplay()
{
  QList<QTreeWidgetItem *> items = _endMessages->getItems();
  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;
  Message currentMsg;

  for (it = items.begin(); it != items.end(); it++) {
      curItem = *it;
      currentMsg = _endMessages->getMessages().value(curItem);
      curItem->setText(END_COLUMN, currentMsg.value);
      curItem->setCheckState(END_ASSIGNATION_COLUMN, Qt::Checked);
      curItem->setToolTip(START_ASSIGNATION_COLUMN, "check to snapshot the value of this address at the end of the selected box");
      fatherColumnCheck(curItem, END_ASSIGNATION_COLUMN);
    }
}

bool
NetworkTree::brothersPartiallyChecked(QTreeWidgetItem *item, int column)
{
    QTreeWidgetItem *father, *child;
    int countCheckedItems = 0;
    int childrenCount = 0;

    if (item->parent() != nullptr) {
        father = item->parent();
        childrenCount = father->childCount();
        for (int i = 0; i < childrenCount; i++) {
            child = father->child(i);
            if (child->checkState(column) == Qt::Checked || child->checkState(column) == Qt::PartiallyChecked) {
                countCheckedItems++;
            }
        }
    }
    if (countCheckedItems > 0 && countCheckedItems < childrenCount) {
        return true;
    }
    else {
        return false;
    }
}

void
NetworkTree::expandItems(QList<QTreeWidgetItem*>& expandedItems)
{
  scrollToTop();
  //collapseAll();

  for(QTreeWidgetItem* item : expandedItems)
  {
	  if(item && checkPredicateInTree(this->invisibleRootItem(), [&item] (QTreeWidgetItem* node)
					{
						return node == item;
					}))
	  {
          if(item->parent() && !item->parent()->isExpanded())
               expandItem(item->parent());
          expandItem(item);
      }
  }
}


void
NetworkTree::clearOSCMessages()
{
  QFont font;
  font.setBold(false);

  QList<QTreeWidgetItem *> items = _OSCStartMessages->getItems();
  QString emptyString;

  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curIt;
  for (it = items.begin(); it != items.end(); it++) {
      curIt = *it;
      curIt->setText(START_COLUMN, emptyString);
      curIt->setFont(NAME_COLUMN, font);
    }
  _OSCStartMessages->clear();

  items = _OSCEndMessages->getItems();
  for (it = items.begin(); it != items.end(); it++) {
      curIt = *it;
      curIt->setText(END_COLUMN, emptyString);
      curIt->setFont(NAME_COLUMN, font);
    }
  _OSCEndMessages->clear();
}

void
NetworkTree::resetNetworkTree()
{
  clearColumn(SR_COLUMN);
  clearColumn(INTERPOLATION_COLUMN);
  clearColumn(REDUNDANCY_COLUMN);
  clearStartMsgs();
  clearEndMsgs();

  resetSelectedItems();
  resetAssignedItems();
  resetAssignedNodes();
  _recMessages.clear();
}


/****************************************************************************
*                          Assignation methods
****************************************************************************
*      - Items (leaves)
*      - Fathers (Nodes)
*          -> Partial assignation (not all children selected)
*          -> Full assignation (all children selected)
*          -> CheckBoxes for start/end values
*
****************************************************************************/

void
NetworkTree::assignItem(QTreeWidgetItem *item, Data data)
{
    setTotallyAssignedStyle(item);

  if (hasStartEndMsg(item)) {
      data.hasCurve = true;
    }

  addAssignedItem(item, data);
  fathersAssignation(item);
}

void
NetworkTree::assignItems(QList<QTreeWidgetItem*> selectedItems)
{
  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;
  Data data;
  data.hasCurve = false;

  for (it = selectedItems.begin(); it != selectedItems.end(); ++it) {
      curItem = *it;
      data.address = getAbsoluteAddress(curItem);
      assignItem(curItem, data);
    }
}

void
NetworkTree::assignItems(QMap<QTreeWidgetItem*, Data> selectedItems)
{
  QList<QTreeWidgetItem *>::iterator it;
  QList<QTreeWidgetItem *> items = selectedItems.keys();
  QTreeWidgetItem *curItem = nullptr;

  resetAssignedItems();
  setAssignedItems(selectedItems);

  for (it = items.begin(); it != items.end(); ++it) {
      curItem = *it;
      assignItem(curItem, selectedItems.value(curItem));
    }
  if (curItem != nullptr) {
      recursiveFatherSelection(curItem, true);
    }
}

void
NetworkTree::unassignItem(QTreeWidgetItem *item, bool recursive)
{
    if(isAssigned(item)){
            item->setCheckState(INTERPOLATION_COLUMN, Qt::Unchecked);
            emit(curveActivationChanged(item, false));

            item->setText(START_COLUMN, "");
            emit(startValueChanged(item, ""));
            item->setText(END_COLUMN, "");
            emit(endValueChanged(item, ""));
            fatherColumnCheck(item, START_ASSIGNATION_COLUMN);
            fatherColumnCheck(item, END_ASSIGNATION_COLUMN);
    }

    int i;
    QTreeWidgetItem *child;

    if(recursive){
        if (!item->isDisabled()) {
            int childrenCount = item->childCount();
            for (i = 0; i < childrenCount; i++) {
                child = item->child(i);
                if (child->type() == NodeNoNamespaceType) {
                    unassignItem(child);
                    recursiveChildrenSelection(child, recursive);
                }
                if (child->type() == LeaveType) {
                    unassignItem(child);
                }
            }
        }
    }
}

void
NetworkTree::setUnassignedStyle(QTreeWidgetItem *item)
{
    QFont font = item->font(NAME_COLUMN);
    font.setBold(false);
    item->setFont(NAME_COLUMN, font);
    removeAssignItem(item);
}

void
NetworkTree::setTotallyAssignedStyle(QTreeWidgetItem *item)
{
//  std::cout << getAbsoluteAddress(item).toStdString() << " > assignTotally" << std::endl;
  QFont font = item->font(NAME_COLUMN);
//  if (item->type() != OSCNode) {
//      item->setSelected(true);
//    }
  font.setBold(true);
  item->setFont(NAME_COLUMN, font);

//    item->setCheckState(0,Qt::Checked);
  addNodeTotallyAssigned(item);
}

void
NetworkTree::setPartiallyAssign(QTreeWidgetItem *item)
{
//  std::cout << getAbsoluteAddress(item).toStdString() << " > assignPartially" << std::endl;

  QFont font = item->font(NAME_COLUMN);
  item->setSelected(false);
  font.setBold(true);
  item->setFont(0, font);

//    item->setCheckState(0,Qt::PartiallyChecked);
  for (int i = 0; i < columnCount(); i++) {
      item->setBackground(i, QBrush(Qt::cyan));
    }

  addNodePartiallyAssigned(item);
}

void
NetworkTree::unassignPartially(QTreeWidgetItem *item)
{
  QFont font = item->font(NAME_COLUMN);
  font.setBold(false);
  item->setFont(NAME_COLUMN, font);
  for (int i = 0; i < columnCount(); i++) {
      item->setBackground(i, QBrush(Qt::NoBrush));
    }

//    item->setCheckState(0,Qt::Unchecked);
  item->setCheckState(START_ASSIGNATION_COLUMN, Qt::Unchecked);
  item->setCheckState(END_ASSIGNATION_COLUMN, Qt::Unchecked);
  removeNodePartiallyAssigned(item);
}

void
NetworkTree::unassignTotally(QTreeWidgetItem *item)
{
  QFont font = item->font(NAME_COLUMN);
  font.setBold(false);
  item->setFont(0, font);
  for (int i = 0; i < columnCount(); i++) {
      item->setBackground(i, QBrush(Qt::NoBrush));
    }

//    item->setCheckState(0,Qt::Unchecked);
  item->setSelected(false);
  item->setCheckState(START_ASSIGNATION_COLUMN, Qt::Unchecked);
  item->setCheckState(END_ASSIGNATION_COLUMN, Qt::Unchecked);
  removeNodeTotallyAssigned(item);
}

void
NetworkTree::fathersAssignation(QTreeWidgetItem *item)
{
//  std::cout << "------- fatherAssignation(" << getAbsoluteAddress(item).toStdString() << ") -------" << std::endl;
  QTreeWidgetItem *father;

  if (item->parent() != nullptr) {
      father = item->parent();

      if (!allBrothersAssigned(item)) {
          setPartiallyAssign(father);
        }
      else
      if (allBrothersAssigned(item)) {
          QTreeWidgetItem *father;
          father = item->parent();
          setTotallyAssignedStyle(father);
        }
      fathersAssignation(father);
    }
}

bool
NetworkTree::allBrothersAssigned(QTreeWidgetItem *item)
{
//    std::cout<<"<-- NetworkTree::allBrothersAssigned("<<item->text(0).toStdString()<<")-->"<<std::endl;
  QTreeWidgetItem *father, *child;

  if (item->parent() != nullptr) {
      father = item->parent();
      int childrenCount = father->childCount();
      for (int i = 0; i < childrenCount; i++) {
          child = father->child(i);
          if (child->type() == NodeNoNamespaceType) {
//                std::cout<<child->text(0).toStdString()<<" > NodeNameSpace"<<std::endl;
              if (!_nodesWithSomeChildrenAssigned.contains(child)) {
//                    std::cout<<"----------------------> false1 "<<std::endl;
                  return false;
                }
            }
          else
          if (!isAssigned(child)) {
//                    std::cout<<"----------------------> false2 "<<std::endl;
              return false;
            }
        }

//        std::cout<<"----------------------> true1"<<std::endl;
      return true;
    }
  else {
//        std::cout<<"----------------------> true2"<<std::endl;
      return true;
    }
}

bool
NetworkTree::allBrothersChecked(QTreeWidgetItem *item, int column)
{
    /*
   * Tool for columns' values
   */
    QTreeWidgetItem *father, *child;

    if (item->parent() != nullptr) {
        father = item->parent();
        int childrenCount = father->childCount();
        for (int i = 0; i < childrenCount; i++) {
            child = father->child(i);

            if (child->checkState(column) == Qt::Unchecked || child->checkState(column) == Qt::PartiallyChecked) {
                return false;
            }

        }
        return true;
    }
    else {
        return true;
    }
}

void
NetworkTree::resetAssignedItems()
{
  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;

  QList<QTreeWidgetItem *>  assignedLeaves = assignedItems().keys();

  for (it = assignedLeaves.begin(); it != assignedLeaves.end(); it++) {
      curItem = *it;
      setUnassignedStyle(curItem);
    }

  _assignedItems.clear();
}

void
NetworkTree::resetAssignedNodes()
{
  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;

  /*
   * NODES (PARTIAL ASSIGNATION)
   */
  QList<QTreeWidgetItem *>  nodesPartial = nodesPartiallyAssigned();

  for (it = nodesPartial.begin(); it != nodesPartial.end(); it++) {
      curItem = *it;
      unassignPartially(curItem);
    }
  _nodesWithSomeChildrenAssigned.clear();


  /*
   * NODES (FULL ASSIGNATION)
   */
  QList<QTreeWidgetItem *>  nodesFull = nodesTotallyAssigned();

  for (it = nodesFull.begin(); it != nodesFull.end(); it++) {
      curItem = *it;
      unassignTotally(curItem);
    }
  _nodesWithAllChildrenAssigned.clear();
}

QList<QTreeWidgetItem*> NetworkTree::getExpandedItems()
{
    QList<QTreeWidgetItem*> expandedItems;
    applyInTree(invisibleRootItem(), [&] (QTreeWidgetItem* it)
    {
        if(it->isExpanded())
        {
            expandedItems.push_back(it);
        }
    });

    return expandedItems;
}

void
NetworkTree::refreshItemNamespace(QTreeWidgetItem *item, bool updateBoxes)
{
    bool isLearning{isInLearningMode()};
    // Make a copy of the addresses which were expanded
    std::vector<std::string> previouslyExpandedAddresses;
    applyInTree(invisibleRootItem(), [&] (QTreeWidgetItem* it)
    {
        if(it->isExpanded())
        {
            previouslyExpandedAddresses.push_back(getAbsoluteAddress(it).toStdString());
        }
    });

    // Make a copy of all the addresses
    std::vector<std::string> previousAddressMap;
    if(isLearning)
    {
        for(auto& addr : _addressMap)
        {
            previousAddressMap.push_back(addr);
        }
    }

    if(item != nullptr)
    {
        if(item->type() == DeviceNode)
        {
            collapseItem(item);
            string application = getAbsoluteAddress(item).toStdString();
            item->takeChildren();

            /// \todo récupérer la valeur de retour.
            /// Peut être false en cas de OSC (traitement différent dans ce cas là).
            Maquette::getInstance()->rebuildNetworkNamespace(application);
            treeRecursiveExploration(item, true);
            if(updateBoxes)
                Maquette::getInstance()->updateBoxesAttributes();

            if(isOSC(item))
                createOSCBranch(item);
        }
    }

    // Restore the addresses
    QList<QTreeWidgetItem*> itemsToExpand;

    // The ones that were expanded
    for(auto& addr : previouslyExpandedAddresses)
    {
        if(std::find(_addressMap.begin(),
                     _addressMap.end(),
                     addr) != _addressMap.end())
        {
            const auto& tree_items = _addressMap.keys(addr);
            for(auto& item : tree_items)
                itemsToExpand.append(item);
        }
    }

    // The new ones
    if(isLearning)
    {
        for(auto& addr : _addressMap)
        {
            if(std::find(previousAddressMap.begin(),
                         previousAddressMap.end(),
                         addr) == previousAddressMap.end())
            {
                const auto& tree_items = _addressMap.keys(addr);
                for(auto& item : tree_items)
                    itemsToExpand.append(item);
            }
        }
    }

    expandItems(itemsToExpand);
}

void
NetworkTree::refreshCurrentItemNamespace()
{
    if(currentItem() != nullptr)
        refreshItemNamespace(currentItem());
}

void
NetworkTree::deleteCurrentItemNamespace()
{
    if(currentItem() != nullptr){
        QString itemName = getAbsoluteAddress(currentItem());

        int ret = QMessageBox::warning(this, QString("Delete %1").arg(itemName),
                                        QString("Do you really want to delete %1 ?").arg(itemName),
                                        QMessageBox::Yes | QMessageBox::Cancel,
                                        QMessageBox::Cancel);
        switch (ret) {
        case QMessageBox::Yes:{
            delete currentItem();
            Maquette::getInstance()->removeNetworkDevice(itemName.toStdString());
            return;
        }

        case QMessageBox::Cancel:
            break;

        }
    }
}

/*************************************************************************
 *                          Selection methods
 *************************************************************************
 *      - Items (leaves)
 *      - Fathers (Nodes)
 *          -> Partial selection (not all children selected)
 *          -> Full selection (all children selected)
 *
 ************************************************************************/


void
NetworkTree::selectPartially(QTreeWidgetItem *item)
{
  QFont font = item->font(NAME_COLUMN);
  item->setSelected(false);
  font.setBold(true);
  item->setFont(0, font);
  for (int i = 0; i < columnCount(); i++) {
      item->setBackground(i, QBrush(Qt::cyan));
    }
  addNodePartiallySelected(item);
}

void
NetworkTree::unselectPartially(QTreeWidgetItem *item)
{
  QFont font = item->font(NAME_COLUMN);
  font.setBold(false);
  item->setFont(NAME_COLUMN, font);
  for (int i = 0; i < columnCount(); i++) {
      item->setBackground(i, QBrush(Qt::NoBrush));
    }
  removeNodePartiallySelected(item);
}

void NetworkTree::recursiveFatherSelection(QTreeWidgetItem *item, bool select)
{
	QTreeWidgetItem *father = item->parent();
	if (father != nullptr) 
	{
		QFont font = item->font(NAME_COLUMN);
		font.setBold(select);
		father->setFont(NAME_COLUMN, font);
		
		if (select) 
		{
			
			if (!allBrothersSelected(item)) 
			{
				selectPartially(father);
			}
			else 
			{
				setTotallyAssignedStyle(father);
			}
		}
		else 
		{
			if (noBrothersSelected(item)) 
			{
				unselectPartially(father);
			}
			else 
			{
				selectPartially(father);
			}
		}
		
		recursiveFatherSelection(father, select);
	}
}

bool
NetworkTree::allBrothersSelected(QTreeWidgetItem *item, QList<QTreeWidgetItem *> assignedItems)
{
  QTreeWidgetItem *father;

  if (item->parent() != nullptr) {
      father = item->parent();
      int childrenCount = father->childCount();
      for (int i = 0; i < childrenCount; i++) {
          if (!assignedItems.contains(father->child(i))) {
              return false;
            }
        }
      return true;
    }
  else {
      return true;
    }
}

bool
NetworkTree::allBrothersSelected(QTreeWidgetItem *item)
{
  QTreeWidgetItem *father, *child;

  if (item->parent() != nullptr) {
      father = item->parent();
      int childrenCount = father->childCount();
      for (int i = 0; i < childrenCount; i++) {
          child = father->child(i);
          if (!child->isSelected()) {
              return false;
            }
        }
      return true;
    }
  else {
      return true;
    }
}

bool
NetworkTree::noBrothersSelected(QTreeWidgetItem *item)
{
  QTreeWidgetItem *father, *child;
  int countSelectedItems = 0;
  if (item->parent() != nullptr) {
      father = item->parent();
      int childrenCount = father->childCount();
      for (int i = 0; i < childrenCount; i++) {
          child = father->child(i);
          if (child->isSelected() || _nodesWithSelectedChildren.contains(child)) {
              countSelectedItems++;
            }
        }
    }

  if (countSelectedItems == 0) {
      return true;
    }
  else {
      return false;
    }
}

void
NetworkTree::resetSelectedItems()
{
  QList<QTreeWidgetItem*> selection = selectedItems();
  QList<QTreeWidgetItem *>::iterator it;
  QTreeWidgetItem *curItem;

  for (it = selection.begin(); it != selection.end(); ++it) {
      curItem = *it;
      curItem->setSelected(false);
    }

  for (it = _nodesWithSelectedChildren.begin(); it != _nodesWithSelectedChildren.end(); ++it) {
      curItem = *it;
      QFont font;
      font.setBold(false);
      for (int i = 0; i < columnCount(); i++) {
          curItem->setBackground(i, QBrush(Qt::NoBrush));
        }

      curItem->setSelected(false);      
      curItem->setCheckState(START_ASSIGNATION_COLUMN, Qt::Unchecked);
      curItem->setCheckState(END_ASSIGNATION_COLUMN, Qt::Unchecked);
    }
  _nodesWithSelectedChildren.clear();
}

void
NetworkTree::recursiveChildrenSelection(QTreeWidgetItem *curItem, bool b_select)
{
	if (!curItem->isDisabled()) 
	{
		for (int i = 0; i < curItem->childCount(); i++) 
		{
			auto child = curItem->child(i);
			child->setSelected(b_select);
			recursiveChildrenSelection(child, b_select);
		}
	}
}

/*************************************************************************
 *                         SLOTS - Virtual methods
 ************************************************************************/

void
NetworkTree::mousePressEvent(QMouseEvent *event)
{
    _noItemClicked = true;
    QTreeWidget::mousePressEvent(event);

    if(currentItem()!=nullptr){
        if(event->button()==Qt::RightButton){
            switch(currentItem()->type())
            {
                case DeviceNode:
                {
                    QMenu *contextMenu = new QMenu(this);
                    QAction *refreshAct = new QAction(tr("Refresh"),this);
                    QAction *deleteAct = new QAction(tr("Delete"),this);

                    contextMenu->addAction(refreshAct);
                    contextMenu->addAction(deleteAct);

                    connect(refreshAct, SIGNAL(triggered()), this, SLOT(refreshCurrentItemNamespace()));
                    connect(deleteAct, SIGNAL(triggered()), this, SLOT(deleteCurrentItemNamespace()));

                    contextMenu->exec(event->globalPos());

                    refreshAct->deleteLater();
                    deleteAct->deleteLater();
                    contextMenu->deleteLater();
                    break;
                }

                case NodeNoNamespaceType:
                case OSCNode:
                {
                    QMenu *contextMenu = new QMenu(this);
                    QAction *deleteAct = new QAction(tr("Delete"),this);

                    contextMenu->addAction(deleteAct);

                    connect(deleteAct, &QAction::triggered,
                            this, &NetworkTree::removeCurrentNode);

                    contextMenu->exec(event->globalPos());

                    deleteAct->deleteLater();
                    contextMenu->deleteLater();
                    break;
                }
                case LeaveType:
                {
                    QMenu *contextMenu = new QMenu(this);
                    QAction *deleteAct = new QAction(tr("Delete"),this);

                    contextMenu->addAction(deleteAct);

                    connect(deleteAct, &QAction::triggered,
                            this, &NetworkTree::removeCurrentNode);

                    contextMenu->exec(event->globalPos());

                    deleteAct->deleteLater();
                    contextMenu->deleteLater();
                    break;
                }
                default:
                    break;
            }
        }

        if(event->button()==Qt::LeftButton){
            if(currentItem()->type() == addDeviceNode){

                 disableLearningForEveryDevice();
                 _deviceEdit->edit();
                 setCurrentItem(nullptr);
            }            
            else if(event->modifiers()==Qt::AltModifier){
                unassignItem(currentItem());
            }
        }
    }
}

void
NetworkTree::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(currentItem()!=nullptr){
        
        /// \todo : replace by if(item->whatsThis(NAME_COLUMN)=="Message").
        if (currentItem()->type() == OSCNode || currentItem()->text(TYPE_COLUMN) == "->") 
		{
            editItem(currentItem(), currentColumn());
			
            if (currentColumn() == NAME_COLUMN) {
                NAME_MODIFIED = true;
            }
            if (currentColumn() == START_COLUMN) {
                VALUE_MODIFIED = true;
            }
            if (currentColumn() == END_COLUMN) {
                VALUE_MODIFIED = true;
            }
            if (currentColumn() == SR_COLUMN) {
                SR_MODIFIED = true;
            }
            if (currentColumn() == MIN_COLUMN) {
                MIN_MODIFIED = true;
            }
            if (currentColumn() == MAX_COLUMN) {
                MAX_MODIFIED = true;
            }
        }
        else if (currentItem()->type() == addOSCNode) {
            ;
        }
        else if (currentItem()->type() == DeviceNode) {
            if(currentColumn() == NAME_COLUMN){
                QString deviceName = currentItem()->text(NAME_COLUMN);
                _deviceEdit->edit(deviceName);
            }
        }
        else {
            if (currentColumn() == START_COLUMN || currentColumn() == END_COLUMN || currentColumn() == SR_COLUMN /*|| currentColumn() == MIN_COLUMN || currentColumn() == MAX_COLUMN*/ ) {
                QTreeWidgetItem *item = currentItem();
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
                editItem(item, currentColumn());
                
                if (currentColumn() == START_COLUMN) {
                    VALUE_MODIFIED = true;
                }
                if (currentColumn() == END_COLUMN) {
                    VALUE_MODIFIED = true;
                }
                if (currentColumn() == SR_COLUMN) {
                    SR_MODIFIED = true;
                }
//                if (currentColumn() == MIN_COLUMN) {
//                    MIN_MODIFIED = true;
//                }
//                if (currentColumn() == MAX_COLUMN) {
//                    MAX_MODIFIED = true;
//                }
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable);
                item->setSelected(true);
            }
        }
    }
}

void
NetworkTree::keyReleaseEvent(QKeyEvent *event)
{  
    if (event->key() == Qt::Key_Shift ||event->key() == Qt::Key_Control) {
          setSelectionMode(QAbstractItemView::SingleSelection);
        }
}

void
NetworkTree::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift)
    {
        setSelectionMode(QAbstractItemView::ContiguousSelection);
    }
    else if(event->key() == Qt::Key_Control)
	{
        setSelectionMode(QAbstractItemView::MultiSelection);
	}
    else if (event->key() == Qt::Key_Backtab)
    {
        if (VALUE_MODIFIED)
        {
            if (currentColumn() == START_COLUMN)
            {
                editItem(currentItem(), END_COLUMN);
            }
        }
    }
    else if(event->key() == Qt::Key_Backspace)
    {
        for(auto& item : selectedItems())
        {
            unassignItem(item);
        }
    }
    else if(event->key() == Qt::Key_Escape)
    {
        unselectAll();
    }
}


void
NetworkTree::clickInNetworkTree(QTreeWidgetItem *item, int column)
{
	_noItemClicked = false;
	if (item != nullptr)
	{
		if(item->type()==DeviceNode && column == NAME_COLUMN)
		{
			Maquette::getInstance()->setDeviceLearn(item->text(NAME_COLUMN).toStdString(),item->checkState(NAME_COLUMN));
		}
		
		// Trouver les parents sélectionnés de l'item actuel
		QList<QTreeWidgetItem*> selected_parents;
		QTreeWidgetItem* itm_parent = item->parent();
		while(itm_parent)
		{
			if(itm_parent->isSelected()) selected_parents << itm_parent;
			itm_parent = itm_parent->parent();
		}
		
		bool item_selected = item->isSelected();
		if(column == 0 && (item->text(TYPE_COLUMN) == "<->" || item->childCount() > 0))
		{
			recursiveChildrenSelection(item, item_selected);
			recursiveFatherSelection(item, item_selected);
		}
		
		for(auto& selected_parent : selected_parents)
			selected_parent->setSelected(true);
		
		auto before_selected = selectedItems();
		
		if(column > 0)
		{
			execClickAction(item, selectedItems(), column);
		}
		/*
		if (selectionMode() == QAbstractItemView::ContiguousSelection) 
		{
			for(auto& item : selectedItems())
			{
				recursiveChildrenSelection(item, !item->isSelected());
			}
		}*/
		
		emit(treeSelectionChanged(selectedItems()));
		
		for(auto& item : before_selected)
			item->setSelected(true);
		
		if (item->type() == addOSCNode) {
			addOSCMessage(item->parent());
		}
		
		// Check if there are still nodes in learning mode
	}
}

void
NetworkTree::valueChanged(QTreeWidgetItem* item, int column)
{                    
    Data data;
    data.hasCurve = false;
    QString qaddress = getAbsoluteAddress(item);
    data.address = qaddress;    

    if (item->type() == LeaveType && column == START_COLUMN && VALUE_MODIFIED) {
        VALUE_MODIFIED = false;
        assignItem(item, data);
        emit(startValueChanged(item, item->text(START_COLUMN)));
    }

    if (item->type() == LeaveType && column == END_COLUMN && VALUE_MODIFIED) {
        VALUE_MODIFIED = false;
        assignItem(item, data);
        emit(endValueChanged(item, item->text(END_COLUMN)));
    }

    if (item->type() == LeaveType && column == SR_COLUMN && SR_MODIFIED) {
        SR_MODIFIED = false;
        emit(curveSampleRateChanged(item, (item->text(SR_COLUMN)).toInt()));
    }

    if (item->type() == LeaveType && column == MIN_COLUMN && MIN_MODIFIED){
        MIN_MODIFIED = false;
        emit(rangeBoundMinChanged(item,item->text(MIN_COLUMN).toFloat()));
    }

    if (item->type() == LeaveType && column == MAX_COLUMN && MAX_MODIFIED){
        MAX_MODIFIED = false;
        emit(rangeBoundMaxChanged(item,item->text(MAX_COLUMN).toFloat()));
    }
    if (item->type() == OSCNode && column == NAME_COLUMN && NAME_MODIFIED) {
        NAME_MODIFIED = false;
        changeNameValue(item, item->text(NAME_COLUMN));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    }
    if (item->type() == OSCNode && column == START_COLUMN && VALUE_MODIFIED) {
        VALUE_MODIFIED = false;
        assignItem(item, data);
        emit(startValueChanged(item, item->text(START_COLUMN)));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    }

    if (item->type() == OSCNode && column == END_COLUMN && VALUE_MODIFIED) {
        VALUE_MODIFIED = false;
        assignItem(item, data);
        emit(endValueChanged(item, item->text(END_COLUMN)));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable| Qt::ItemIsEditable);
    }

    if (item->type() == OSCNode && column == SR_COLUMN && SR_MODIFIED) {
        SR_MODIFIED = false;
        emit(curveSampleRateChanged(item, (item->text(SR_COLUMN)).toInt()));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    }

    //Case message
    if(item->whatsThis(NAME_COLUMN)=="Message"){
        if (column == START_COLUMN && VALUE_MODIFIED) {
            VALUE_MODIFIED = false;
            assignItem(item, data);
            emit(startValueChanged(item, item->text(START_COLUMN)));
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        }
        else if (column == END_COLUMN && VALUE_MODIFIED) {
            VALUE_MODIFIED = false;
            assignItem(item, data);
            emit(endValueChanged(item, item->text(START_COLUMN)));
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        }
    }

    else if (column == END_COLUMN && VALUE_MODIFIED) {
        VALUE_MODIFIED = false;
        assignItem(item, data);
        emit(endValueChanged(item, item->text(END_COLUMN)));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable| Qt::ItemIsEditable);
    }

    else if (column == SR_COLUMN && SR_MODIFIED) {
        SR_MODIFIED = false;
        emit(curveSampleRateChanged(item, (item->text(SR_COLUMN)).toInt()));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    }
}


QString
NetworkTree::getAbsoluteAddressWithValue(QTreeWidgetItem *item, int column) const
{
  QString address = getAbsoluteAddress(item);
  QString value;
  if (column == START_COLUMN || column == END_COLUMN) {
      value = item->text(column);
      address += " ";
      address += value;
    }
  else {
      std::cerr << "NetworkTree::getAbsoluteAddressWithValue : incorrect column" << std::endl;
    }

  return address;
}

QList<string> NetworkTree::getAddressList()
{
    QList<string> addressList;
    QMap<QTreeWidgetItem *, string>::iterator it;
    for (it = _addressMap.begin(); it != _addressMap.end(); it++) {
        if(it.key() != nullptr) {
            QTreeWidgetItem* item = it.key();
            if(checkPredicateInTree(this->invisibleRootItem(), [&item] (QTreeWidgetItem* node)
                    {
                        return node == item;
                    }))
            {
                if (it.key()->toolTip(NetworkTree::TYPE_COLUMN) != nullptr) {
                    if( it.key()->toolTip(NetworkTree::TYPE_COLUMN) == "bi-directionnal" || it.key()->toolTip(NetworkTree::TYPE_COLUMN) == "receiver" || it.key()->toolTip(NetworkTree::TYPE_COLUMN) == "sender" ) {
                        addressList << it.value();
                    }
                }
            }
        }
    }
    return addressList;
}

void
NetworkTree::changeStartValue(QTreeWidgetItem *item, QString newValue)
{
  if (newValue.isEmpty()) {
      item->setCheckState(START_ASSIGNATION_COLUMN,Qt::Unchecked);
      _startMessages->removeMessage(item);      
      if (item->type() == OSCNode) {
          _OSCStartMessages->removeMessage(item);
        }
      if (!endMessages()->getMessages().contains(item)) {
          removeAssignItem(item);
        }
      emit(startMessageValueChanged(item));
    }
  else {
      if (!_startMessages->getMessages().contains(item)) { //New message

          QString Qaddress = getAbsoluteAddressWithValue(item, START_COLUMN);

          //TODO Request la priority et faire un addMessage PAS SIMPLE
          _startMessages->addMessageSimple(item, Qaddress);
          Data data;
          assignItem(item, data);
          if (item->type() == OSCNode) {
              addOSCStartMessage(item, Qaddress);
            }
          emit(startMessageValueChanged(item));
        }
      else { //new value entered
          if (_startMessages->setValue(item, newValue)) {
              if (item->type() == OSCNode) {
                  _OSCStartMessages->setValue(item, newValue);
                }
              emit(startMessageValueChanged(item));
            }
          else {
              std::cerr << "NetworkTree::changeStartValue : Impossible de créer le networkMessage" << std::endl;
            }
        }
    }
}

void
NetworkTree::changeEndValue(QTreeWidgetItem *item, QString newValue)
{
  //Prévoir un assert. Vérifier, le type, range...etc

  if (newValue.isEmpty()) {
      item->setCheckState(END_ASSIGNATION_COLUMN,Qt::Unchecked);
      _endMessages->removeMessage(item);
      if (item->type() == OSCNode) {
          _OSCEndMessages->removeMessage(item);
        }
      if (!startMessages()->getMessages().contains(item)) {
          removeAssignItem(item);
        }
      emit(endMessageValueChanged(item));
    }
  else {
      if (!_endMessages->getMessages().contains(item)) {
          QString Qaddress = getAbsoluteAddressWithValue(item, END_COLUMN);
          _endMessages->addMessageSimple(item, Qaddress);
          if (item->type() == OSCNode) {
              addOSCEndMessage(item, Qaddress);
            }
          emit(endMessageValueChanged(item));
        }
      else {
          if (_endMessages->setValue(item, newValue)) {
              if (item->type() == OSCNode) {
                  _OSCEndMessages->setValue(item, newValue);
                }
              emit(endMessageValueChanged(item));
            }
          else {
              std::cerr << "NetworkTree::changeEndValue : Impossible de créer le networkMessage" << std::endl;
            }
        }
    }
}

void
NetworkTree::changeNameValue(QTreeWidgetItem *item, QString newValue)
{
  if (item->type() == OSCNode) {
      if (newValue.isEmpty()) { //remove message
          _startMessages->removeMessage(item);
          _endMessages->removeMessage(item);
          _OSCEndMessages->removeMessage(item);
          _OSCStartMessages->removeMessage(item);
          item->parent()->removeChild(item);

          removeAssignItem(item);
          Maquette::getInstance()->removeFromNetWorkNamespace(getAbsoluteAddress(item).toStdString());
        }
      else {
          /*setOSCMessageName(item, newValue);
          emit(messageChanged(item, newValue));*/
        }
      QMap<QTreeWidgetItem *, Data>::iterator it = _assignedItems.find(item);
      if (it != _assignedItems.end()) {
          Data data = it.value();
          removeAssignItem(item);
          assignItem(item, data);
        }
    }
}

/***********************************************************************
*                              Curves
***********************************************************************/

unsigned int
NetworkTree::getSampleRate(QTreeWidgetItem *item)
{
  if (isAssigned(item)) {
      if (_assignedItems.value(item).hasCurve) {
          return _assignedItems.value(item).sampleRate;
        }
      else {
          std::cerr << "NetworkTree::getSampleRate : Impossible to get item's sample rate, item has no curve." << std::endl;
          exit(-1);
        }
    }
  else {
      std::cerr << "NetworkTree::getSampleRate : Impossible to get item's sample rate, item is not assigned." << std::endl;
      exit(-1);
    }
}


bool
NetworkTree::updateCurve(QTreeWidgetItem *item, unsigned int boxID, bool forceUpdate)
{
  string address = getAbsoluteAddress(item).toStdString();

  BasicBox *box = Maquette::getInstance()->getBox(boxID);
  if (box != nullptr)
  { // Box Found
    if (box->hasCurve(address) && !_recMessages.contains(item) )
    {
      if (_assignedItems.value(item).hasCurve)
      {
        unsigned int sampleRate = 0;
        bool redundancy = false, interpolate = false;
        vector<float> values, xPercents, yValues, coeff;
        vector<string> argTypes;
        vector<short> sectionType;

        bool getCurveSuccess = Maquette::getInstance()->getCurveAttributes(boxID, address, 0, sampleRate, redundancy, interpolate, values, argTypes, xPercents, yValues, sectionType, coeff);
        bool getCurveValuesSuccess = Maquette::getInstance()->getCurveValues(boxID, address, 0, yValues);

        if(!Maquette::getInstance()->curveIsManuallyActivated(boxID, address))
        {
          if(getCurveSuccess && forceUpdate && interpolate)
          {
            interpolate = !(startMessages()->getMessage(item).value == endMessages()->getMessage(item).value);
          }
          else if (getCurveValuesSuccess)
          {
            interpolate = true;
          }
        }
        else
        {
            interpolate = true;
        }

        Maquette::getInstance()->setCurveMuteState(boxID, address, !interpolate);
        updateLine(item, interpolate, sampleRate, redundancy);
      }
    }
  }
  else
  { // Box Not Found
    return false;
  }

  return false;
}

void
NetworkTree::addOSCStartMessage(QTreeWidgetItem *item, QString msg)
{
  _OSCStartMessages->addMessageSimple(item, msg);
}

void
NetworkTree::addOSCEndMessage(QTreeWidgetItem *item, QString msg)
{
  _OSCEndMessages->addMessageSimple(item, msg);
}

void
NetworkTree::updateCurves(unsigned int boxID, bool forceUpdate)
{
  if (boxID != NO_ID) {
      for (QTreeWidgetItem*& item : _assignedItems.keys()) {
          updateCurve(item, boxID, forceUpdate);
        }
    }
}

bool
NetworkTree::hasCurve(QTreeWidgetItem *item)
{
  return _assignedItems.value(item).hasCurve;
}

void
NetworkTree::setSampleRate(QTreeWidgetItem *item, unsigned int sampleRate)
{
  Data data = _assignedItems.value(item);
  data.sampleRate = sampleRate;
  _assignedItems.insert(item, data);
}

void
NetworkTree::setHasCurve(QTreeWidgetItem *item, bool val)
{
  Data data = _assignedItems.value(item);
  data.hasCurve = val;
  _assignedItems.insert(item, data);
}

void
NetworkTree::setCurveActivated(QTreeWidgetItem *item, bool activated)
{
  Data data = _assignedItems.value(item);
  data.curveActivated = activated;
  _assignedItems.insert(item, data);
}

void
NetworkTree::setRedundancy(QTreeWidgetItem *item, bool activated)
{
  Data data = _assignedItems.value(item);
  data.redundancy = activated;
  _assignedItems.insert(item, data);
}

void
NetworkTree::updateLine(QTreeWidgetItem *item, bool interpolationState, int sampleRate, bool redundancy)
{
  //INTERPOLATION STATE
  setCurveActivated(item, interpolationState);
  if (interpolationState) {
      item->setCheckState(INTERPOLATION_COLUMN, Qt::Checked);
      //SAMPLE RATE
      setSampleRate(item, sampleRate);
      item->setText(SR_COLUMN, QString::number(getSampleRate(item)));
    }
  else {
      item->setCheckState(INTERPOLATION_COLUMN, Qt::Unchecked);
      item->setText(SR_COLUMN, "");
    }


  //REDUNDANCY
  setRedundancy(item, redundancy);
  if (redundancy) {
      item->setCheckState(REDUNDANCY_COLUMN, Qt::Checked);
    }
  else {
      item->setCheckState(REDUNDANCY_COLUMN, Qt::Unchecked);
    }
}

void
NetworkTree::updateDeviceName(QString oldName, QString newName)
{
    if(currentItem()!=nullptr){
        if(currentItem()->text(NAME_COLUMN) == oldName){
            currentItem()->setText(NAME_COLUMN, newName);
            return;
        }        
    }

    else{
        //have to find in networkTree the device item
        QList<QTreeWidgetItem *> items = findItems(oldName,Qt::MatchExactly,NAME_COLUMN);

        if(items.isEmpty()){ //no item found, create a new device item
            addNewDevice(newName);
            return;
        }
        else{
            for(int i=0 ; i<items.size() ; i++){
                if(items[i]->type() == DeviceNode){ //first deviceType found is set
                    items[i]->setText(NAME_COLUMN, newName);
                    return;
                }
            }
        }
    }
}

void NetworkTree::addNewDevice(QString deviceName)
{
    QTreeWidgetItem *newItem = addDeviceItem(deviceName);
    newItem->setCheckState(NAME_COLUMN,Qt::Unchecked);
    refreshItemNamespace(newItem);
    string protocol;
    Maquette::getInstance()->getDeviceProtocol(deviceName.toStdString(),protocol);    
    if(protocol=="OSC")
        createOSCBranch(newItem);
}

void
NetworkTree::updateDeviceProtocol(QString newName)
{           
  QString deviceName = currentItem()->text(NAME_COLUMN);
  QTreeWidgetItem *item = currentItem();
  item->takeChildren();

  if (newName == "OSC")
      createOSCBranch(item);

  emit(pluginChanged(deviceName));

  //Va supprimer les message de cette device
}

void
NetworkTree::updateDeviceNamespace(QString deviceName)
{
    QTreeWidgetItem *deviceItem;

    if(currentItem()!=nullptr && currentItem()->text(NAME_COLUMN) == deviceName)
        deviceItem = currentItem();

    else{
        //have to find in networkTree the device item
        QList<QTreeWidgetItem *> items = findItems(deviceName,Qt::MatchExactly,NAME_COLUMN);

        if(items.isEmpty()){ //no item found, create a new device item
            std::cerr<<"NetworkTree::updateDeviceNamespace() : cannot find item assiocated to the device's name"<<std::endl;
            return;
        }
        else{
            for(int i=0 ; i<items.size() ; i++){
                if(items.at(i)->type() == DeviceNode){ //first deviceType found is set
                    deviceItem = items.at(i);
                }
            }
        }
    }

    if (deviceItem != nullptr)
        treeRecursiveExploration(deviceItem,false);

}

void
NetworkTree::setRecMode(std::string address){
    QTreeWidgetItem *item = getItemFromAddress(address);
    if(!item) return;

    if(!_recMessages.contains(item)){
        _recMessages<<item;

        item->setData(INTERPOLATION_COLUMN, Qt::CheckStateRole, QVariant());
        item->setIcon(INTERPOLATION_COLUMN,QIcon(":/resources/images/record.svg"));
    }
    else{        
        _recMessages.removeAll(item);
        item->setCheckState(INTERPOLATION_COLUMN,Qt::Unchecked);
    }
}

void
NetworkTree::setRecMode(QList<std::string> address){
    QList<std::string>::iterator it;
    for(it=address.begin() ; it!=address.end() ; it++)
        setRecMode(*it);
}

void
NetworkTree::getChildren(QTreeWidgetItem *item, QList<QTreeWidgetItem *> & items)
{
    QTreeWidgetItem *child;

    if (!item->isDisabled()) {
        int childrenCount = item->childCount();
        for (int i = 0; i < childrenCount; i++) {
            child = item->child(i);
            //if (child->type() == NodeNoNamespaceType) {
                items<<child;
                getChildren(child, items);
            //}
            //else
            //    items<<child;
        }
    }
}
void
NetworkTree::execClickAction(QTreeWidgetItem *curItem, QList<QTreeWidgetItem *> selectedItems, int column)
{
    QList<QTreeWidgetItem *>::iterator it;    

    //si le curItem n'est pas sélectionné, on n'applique l'action seulement sur celui-ci (pas sur les items sélectionnés)
    if(!selectedItems.contains(curItem)){
        selectedItems.clear();
        selectedItems<<curItem;
    }

    //adding children of each node
    QList<QTreeWidgetItem *> children, allChildren;

	for(auto& item : selectedItems)
	{
        children.clear();
        getChildren(item, children);
        allChildren<<children;
    }
    selectedItems<<allChildren;

	if(column == START_ASSIGNATION_COLUMN)
	{
		if(hasStartMsg(curItem))
		{
			//cmd+click update values
			if(static_cast<QApplication *>(QApplication::instance())->keyboardModifiers() == Qt::ControlModifier)
			{
				emit(requestSnapshotStart(selectedItems));
			}
			else
			{
				//remove all start messages
				for(auto& item : selectedItems)
				{
					item->setText(START_COLUMN, "");
					emit(startValueChanged(item, ""));
				}
			}
		}
		else if(curItem->checkState(START_ASSIGNATION_COLUMN) != Qt::Unchecked && curItem->childCount() > 0)
		{
			for(auto& item : selectedItems)
			{
				item->setText(START_COLUMN, "");
				emit(startValueChanged(item, ""));
			}
		}
		else
		{
			emit(requestSnapshotStart(selectedItems));
		}
	}
	else if(column == END_ASSIGNATION_COLUMN)
	{
		if(hasEndMsg(curItem))
		{
			//cmd+click update values
			if(static_cast<QApplication *>(QApplication::instance())->keyboardModifiers() == Qt::ControlModifier)
			{
				emit(requestSnapshotEnd(selectedItems));
			}
			else
			{
				//remove all start messages
				for(auto& item : selectedItems)
				{
					item->setText(END_COLUMN, "");
					emit(endValueChanged(item, ""));
				}
			}
		}
		else if(curItem->checkState(END_ASSIGNATION_COLUMN) != Qt::Unchecked  && curItem->childCount() > 0)
		{
			for(auto& item : selectedItems)
			{
				item->setText(END_COLUMN, "");
				emit(endValueChanged(item, ""));
			}
		}
		else
		{
			emit(requestSnapshotEnd(selectedItems));
		}
	}

    else if ((curItem->type() == LeaveType || curItem->type() == OSCNode) && column == INTERPOLATION_COLUMN) 
	{
        if(static_cast<QApplication *>(QApplication::instance())->keyboardModifiers() == Qt::ControlModifier)
		{
            for(int i = 0; i<selectedItems.size(); i++)
                emit recModeChanged(selectedItems.at(i));
        }
        else{
            for(int i = 0; i<selectedItems.size(); i++){
                auto item = selectedItems.at(i);

                //INTERPOLATION
                if (isAssigned(item) && hasStartMsg(item) && hasEndMsg(item)) {
                    //we set the curItem's state to item.
                    bool activated = curItem->checkState(INTERPOLATION_COLUMN) == Qt::Checked;
                    emit(curveActivationChanged(item, activated));
                    item->setCheckState(INTERPOLATION_COLUMN, curItem->checkState(INTERPOLATION_COLUMN));
                }
                else {
                    float start = 0., end = 1.;
                    vector<float> rangeBounds;
                    std::string address = getAbsoluteAddress(item).toStdString();

                    if(Maquette::getInstance()->getRangeBounds(address,rangeBounds)>0)
                    {
                        if(!hasStartEndMsg(item)){ //Creates curve with start=minBound and end=maxBound (default 0 1)
                            start = rangeBounds[0];
                            end = rangeBounds[1];
                        }
                        else if (hasStartMsg(item)){
                            start = _startMessages->getMessage(item).value.toFloat();
                            end = rangeBounds[1];
                        }
                        else if(hasEndMsg(item)){
                            start = rangeBounds[0];
                            end = _endMessages->getMessage(item).value.toFloat();
                        }
                    }
                    VALUE_MODIFIED = true;
                    item->setText(START_COLUMN,QString("%1").arg(start));
                    VALUE_MODIFIED = true;
                    item->setText(END_COLUMN,QString("%1").arg(end));
                }
            }
        }
    }

    //REDUNDANCY
    else if ((curItem->type() == LeaveType || curItem->type() == OSCNode) && column == REDUNDANCY_COLUMN) 
	{
        for(auto& item : selectedItems)
		{
            if (isAssigned(item) && hasCurve(item)) {
                bool activated = curItem->checkState(column) == Qt::Checked;
                emit(curveRedundancyChanged(item, activated));
                item->setCheckState(REDUNDANCY_COLUMN, curItem->checkState(REDUNDANCY_COLUMN));
            }
            else {
                item->setCheckState(column, Qt::Unchecked);
            }
        }
    }
	
	/////////// Try to fix bad things that may happen
	if(curItem->childCount() > 0)
	{
		bool deselect_start = true;
		bool deselect_end = true;
		
		for(int cnt = 0; cnt < curItem->childCount(); ++cnt)
		{
			auto child = curItem->child(cnt);
			if(child->checkState(START_ASSIGNATION_COLUMN) != Qt::Unchecked)
			{
				deselect_start = false; 
			}
			if(child->checkState(END_ASSIGNATION_COLUMN) != Qt::Unchecked)
			{
				deselect_end = false; 
			}
		}
		
		if(deselect_start) curItem->setCheckState(START_ASSIGNATION_COLUMN, Qt::Unchecked);
		if(deselect_end) curItem->setCheckState(END_ASSIGNATION_COLUMN, Qt::Unchecked);
	}
}

void
NetworkTree::unselectAll()
{
    for(auto& item : selectedItems())
        item->setSelected(false);

    emit(treeSelectionChanged(selectedItems()));
}

bool NetworkTree::isInLearningMode()
{
    for(int i = 0; i < topLevelItemCount(); ++i)
    {
        if(topLevelItem(i)->checkState(NAME_COLUMN) == Qt::Checked)
            return true;
    }

    return false;
}

void NetworkTree::disableLearningForEveryDevice()
{
    for(int i = 0; i < topLevelItemCount(); ++i)
        if(topLevelItem(i)->type() != addDeviceNode)
            topLevelItem(i)->setCheckState(NAME_COLUMN, Qt::Unchecked);
}
