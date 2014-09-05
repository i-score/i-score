TEMPLATE = app
TARGET = i-score

QT += core network xml svg printsupport

INCLUDEPATH += headers/GUI headers/data
RESOURCES += i-score.qrc
DEFINES += __Types__ TT_NO_DSP

ICON = resources/images/i-score.icns
resources/translations = i-score_en.ts i-score_fr.ts


QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-deprecated-register -O3 -fPIC -msse3 -std=c++11
QMAKE_LFLAGS += -L/usr/local/lib/jamoma/lib -L/usr/local/lib/ -Wl,-rpath,/usr/local/jamoma/lib -Wl,-rpath,/usr/local/jamoma/extensions

contains(QMAKE_HOST.arch, 86) {
        QMAKE_CXXFLAGS += -msse3
}

unix {
INCLUDEPATH +=	/usr/local/jamoma/include

macx {
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
QMAKE_CXXFLAGS += -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET -stdlib=libc++
QMAKE_LFLAGS   += -stdlib=libc++ -lc++
}

linux-clang{
	QMAKE_CXXFLAGS += -stdlib=libc++
	QMAKE_LFLAGS += -stdlib=libc++
}
linux-clang:contains(QMAKE_HOST.arch, x86):{
	INCLUDEPATH += /usr/include/i386-linux-gnu/c++/4.8/
}

LIBS += -L/usr/local/jamoma/lib -lJamomaFoundation -lJamomaDSP -lJamomaScore -lJamomaModular
LIBS += -lxml2 -lgecodeint -lgecodesearch -lgecodedriver -lgecodeflatzinc -lgecodekernel -lgecodeminimodel -lgecodeset -lgecodesupport

}

# Input
HEADERS += \
headers/data/Abstract.hpp \
headers/data/AbstractBox.hpp \
headers/data/AbstractComment.hpp \
headers/data/AbstractCurve.hpp \
headers/data/AbstractRelation.hpp \
headers/data/AbstractParentBox.hpp \
headers/data/AbstractTriggerPoint.hpp \
headers/data/Engine.h \
headers/data/Maquette.hpp \
headers/data/NetworkMessages.hpp \
headers/GUI/AttributesEditor.hpp \
headers/GUI/BasicBox.hpp \
headers/GUI/BoxContextMenu.hpp \
headers/GUI/ChooseTemporalRelation.hpp \
headers/GUI/Comment.hpp \
headers/GUI/CurveWidget.hpp \
headers/GUI/CurvesWidget.hpp \
headers/GUI/Help.hpp \
headers/GUI/Interpolation.hpp \
headers/GUI/LogarithmicSlider.hpp \
headers/GUI/MainWindow.hpp \
headers/GUI/MaquetteScene.hpp \
headers/GUI/MaquetteView.hpp \
headers/GUI/NetworkConfig.hpp \
headers/GUI/NetworkMessagesEditor.hpp \
headers/GUI/NetworkTree.hpp \
headers/GUI/ParentBox.hpp \
headers/GUI/ParentBoxContextMenu.hpp \
headers/GUI/PlayingThread.hpp \
headers/GUI/Relation.hpp \
headers/GUI/RelationEdit.hpp \
headers/GUI/TextEdit.hpp \
headers/GUI/TriggerPoint.hpp \
headers/GUI/TreeMap.hpp \
headers/GUI/TreeMapElement.hpp \
headers/GUI/ViewRelations.hpp \
headers/GUI/BoxWidget.hpp \
headers/GUI/BoxCurveEdit.hpp \
headers/GUI/TimeBarWidget.hpp \
headers/GUI/DeviceEdit.hpp \
headers/GUI/HeaderPanelWidget.hpp \
headers/GUI/ConditionalRelation.hpp \
headers/GUI/TriggerPointEdit.hpp

SOURCES += src/main.cpp \
src/data/Abstract.cpp \
src/data/AbstractBox.cpp \
src/data/AbstractComment.cpp \
src/data/AbstractCurve.cpp \
src/data/AbstractParentBox.cpp \
src/data/AbstractRelation.cpp \
src/data/AbstractTriggerPoint.cpp \
src/data/Engine.cpp \
src/data/Maquette.cpp \
src/data/NetworkMessages.cpp \
src/GUI/AttributesEditor.cpp \
src/GUI/BasicBox.cpp \
src/GUI/BoxContextMenu.cpp \
src/GUI/ChooseTemporalRelation.cpp \
src/GUI/Comment.cpp \
src/GUI/CurveWidget.cpp \
src/GUI/CurvesWidget.cpp \
src/GUI/Help.cpp \
src/GUI/Interpolation.cpp \
src/GUI/LogarithmicSlider.cpp \
src/GUI/MainWindow.cpp \
src/GUI/MaquetteScene.cpp \
src/GUI/MaquetteView.cpp \
src/GUI/NetworkConfig.cpp \
src/GUI/NetworkMessagesEditor.cpp \
src/GUI/NetworkTree.cpp \
src/GUI/ParentBox.cpp \
src/GUI/ParentBoxContextMenu.cpp \
src/GUI/PlayingThread.cpp \
src/GUI/Relation.cpp \
src/GUI/RelationEdit.cpp \
src/GUI/TextEdit.cpp \
src/GUI/TriggerPoint.cpp \
src/GUI/TreeMap.cpp \
src/GUI/TreeMapElement.cpp \
src/GUI/ViewRelations.cpp \
src/GUI/BoxWidget.cpp \
src/GUI/BoxCurveEdit.cpp \
src/GUI/TimeBarWidget.cpp \
src/GUI/DeviceEdit.cpp \
src/GUI/HeaderPanelWidget.cpp \
src/GUI/ConditionalRelation.cpp \
src/GUI/TriggerPointEdit.cpp
