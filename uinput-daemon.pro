SOURCES += \
		main.cpp \
		uinput.cpp \

HEADERS += \
		uinput.h \

CONFIG += link_pkgconfig
PKGCONFIG += tslib

target.path = /progetti

INSTALLS += target
