TEMPLATE = app
TARGET = i-score

QT += core network xml svg printsupport
#DEFINES     += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT

INCLUDEPATH += headers headers/GUI headers/data
RESOURCES += i-score.qrc
DEFINES += __Types__ TT_NO_DSP Q_COMPILER_INITIALIZER_LISTS

ICON = resources/images/i-score.icns
resources/translations = i-score_en.ts i-score_fr.ts

unix {
    INCLUDEPATH += /usr/local/lib/jamoma/include /usr/local/jamoma/include /usr/include/libxml2
    QMAKE_CXXFLAGS += -std=c++11  -Wno-unused-parameter -Wno-deprecated-register
    QMAKE_CXXFLAGS_RELEASE += -O3 -fPIC
    QMAKE_LFLAGS += -Wl,-rpath,/usr/local/lib/jamoma/lib  -Wl,-rpath,/usr/local/jamoma/lib -Wl,-rpath,/usr/local/jamoma/extensions

    LIBS += -L/usr/local/lib/jamoma/lib -L/usr/local/lib/ -L/usr/local/jamoma/lib -lJamomaFoundation -lJamomaDSP -lJamomaScore -lJamomaModular
    LIBS += -lxml2 -lgecodeint -lgecodesearch -lgecodedriver -lgecodeflatzinc -lgecodekernel -lgecodeminimodel -lgecodeset -lgecodesupport

    contains(QMAKE_HOST.arch, 86){
        QMAKE_CXXFLAGS_RELEASE += -msse3
    }

    macx{
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
        QMAKE_CXXFLAGS += -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET -stdlib=libc++
        QMAKE_LFLAGS   += -stdlib=libc++ -lc++
        QMAKE_INFO_PLIST = Info.plist
    }

    linux-clang{
	QMAKE_CXXFLAGS += -stdlib=libc++
	QMAKE_LFLAGS += -stdlib=libc++
    }

    linux-clang:contains(QMAKE_HOST.arch, x86):{
	INCLUDEPATH += /usr/include/i386-linux-gnu/c++/4.8/
    }
}

win32 {
    DEFINES += NOMINMAX
    INCLUDEPATH += "C:/Program Files (x86)/GnuWin32/include" \
                   "C:/Program Files (x86)/libxml2-2.7.8.win32/include" \
                   "C:/Program Files (x86)/JamomaCore 0.6-dev/include" 

                    
    LIBS += "C:/Program Files (x86)/JamomaCore 0.6-dev/lib/JamomaFoundation.lib" \ 
            "C:/Program Files (x86)/JamomaCore 0.6-dev/lib/JamomaScore.lib" \
            "C:/Program Files (x86)/JamomaCore 0.6-dev/lib/JamomaModular.lib"
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
headers/GUI/Comment.hpp \
headers/GUI/CurveWidget.hpp \
headers/GUI/Help.hpp \
headers/GUI/LogarithmicSlider.hpp \
headers/GUI/MainWindow.hpp \
headers/GUI/MaquetteScene.hpp \
headers/GUI/MaquetteView.hpp \
headers/GUI/NetworkTree.hpp \
headers/GUI/ParentBox.hpp \
headers/GUI/PlayingThread.hpp \
headers/GUI/Relation.hpp \
headers/GUI/TriggerPoint.hpp \
headers/GUI/BoxWidget.hpp \
headers/GUI/TimeBarWidget.hpp \
headers/GUI/DeviceEdit.hpp \
headers/GUI/HeaderPanelWidget.hpp \
headers/GUI/ConditionalRelation.hpp \
headers/GUI/TriggerPointEdit.hpp \
headers/IScoreApplication.hpp \
headers/GUI/CurvesComboBox.hpp \
headers/DelayedDelete.h \
headers/GlobalEventFilter.h \
headers/GUI/NetworkUpdater.h

SOURCES += src/main.cpp \
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
src/GUI/Comment.cpp \
src/GUI/CurveWidget.cpp \
src/GUI/Help.cpp \
src/GUI/LogarithmicSlider.cpp \
src/GUI/MainWindow.cpp \
src/GUI/MaquetteScene.cpp \
src/GUI/MaquetteView.cpp \
src/GUI/NetworkTree.cpp \
src/GUI/ParentBox.cpp \
src/GUI/PlayingThread.cpp \
src/GUI/Relation.cpp \
src/GUI/TriggerPoint.cpp \
src/GUI/BoxWidget.cpp \
src/GUI/TimeBarWidget.cpp \
src/GUI/DeviceEdit.cpp \
src/GUI/HeaderPanelWidget.cpp \
src/GUI/ConditionalRelation.cpp \
src/GUI/TriggerPointEdit.cpp \
src/IScoreApplication.cpp \
src/GUI/CurvesComboBox.cpp \
src/GUI/NetworkUpdater.cpp
