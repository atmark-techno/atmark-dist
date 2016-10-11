QT += qml quick core-private
qtHaveModule(widgets): QT += widgets

SOURCES += main.cpp

DEFINES += QML_RUNTIME_TESTING QT_QML_DEBUG_NO_WARNING

load(qt_tool)

INSTALLS += target
target.path = /usr/bin
