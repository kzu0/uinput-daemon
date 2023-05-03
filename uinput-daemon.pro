SOURCES += \
		main.cpp \
		uinput.cpp \

HEADERS += \
		uinput.h \

CONFIG += link_pkgconfig
PKGCONFIG += tslib

target.path = /usr/bin

INSTALLS += target
