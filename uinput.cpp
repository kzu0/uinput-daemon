#include "uinput.h"
#include <QDebug>
//---------------------------------------------------------------------------------------------------------------------
// https://docs.kernel.org/input/uinput.html
//---------------------------------------------------------------------------------------------------------------------
int UinputInit() {

	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	qDebug() << "OPEN uinput" << fd;

	if (fd < 0) {
		return fd;
	}

	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);

	ioctl(fd, UI_SET_EVBIT, EV_REL) ;
	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);

	struct uinput_user_dev uidev = {0};

	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "ProxyInput");

	int ret = write(fd, &uidev, sizeof(uidev));
	ioctl(fd, UI_DEV_CREATE);

	return fd;

	Q_UNUSED(ret);
}
//---------------------------------------------------------------------------------------------------------------------
void UinputEvent(int fd, int type, int code, int val, __time_t sec, __suseconds_t usec) {

	struct input_event ie;

	ie.type = type;
	ie.code = code;
	ie.value = val;
	ie.time.tv_sec = sec;
	ie.time.tv_usec = usec;

	if (fd != -1) {
		write(fd, &ie, sizeof(ie));
	}
}
//---------------------------------------------------------------------------------------------------------------------
void UinputClose(int fd) {

	if (fd != -1) {
		ioctl(fd, UI_DEV_DESTROY);
		close(fd);
	}
}
