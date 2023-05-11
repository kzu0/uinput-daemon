#include "uinput.h"
#include <QDebug>
//---------------------------------------------------------------------------------------------------------------------
// https://docs.kernel.org/input/uinput.html
//---------------------------------------------------------------------------------------------------------------------
// https://github.com/libts/tslib/blob/master/src/ts_setup.c
//---------------------------------------------------------------------------------------------------------------------
int UinputInit() {

	int fd, rc, version;

	//
	// Apri il file descriptor
	//
	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	qDebug() << "OPEN uinput" << fd;
	if (fd < 0) {
		return fd;
	}

	//
	// Controllo versione uinput
	//
	rc = ioctl(fd, UI_GET_VERSION, &version);
	qDebug() << "version:" << version;
	if (rc < 0 or version < 5) {
		perror("uinput version error");
		return -1;
	}

	//
	// Setto proprietà UInput
	//
	if(ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT) < 0) {
		perror("error: UI_SET_PROPBIT");
		return -1;
	}
	if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
		perror("error: ioctl EV_KEY");
		return -1;
	}
	if (ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH) < 0) {
		perror("error: ioctl UI_SET_KEYBIT");
		return -1;
	}
	if(ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0) {
		perror("error: ioctl EV_ABS");
		return -1;
	}
	if(ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0) {
		perror("error: ioctl ABS_X");
		return -1;
	}
	if(ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0) {
		perror("error: ioctl ABS_Y");
		return -1;
	}

	//
	// UI_DEV_SETUP
	//
	struct uinput_setup usetup;
	memset(&usetup, 0, sizeof(usetup));

	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234; /* sample vendor */
	usetup.id.product = 0x5678; /* sample product */
	strcpy(usetup.name, "ProxyInput");

	if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0) {
		perror("error: ioctl UI_DEV_SETUP");
		return -1;
	}

	//
	// X Axis
	//
	struct uinput_abs_setup xabs;
	memset(&xabs, 0, sizeof(xabs));

	xabs.code = ABS_X;
	xabs.absinfo.minimum = 0;
	xabs.absinfo.maximum = 800; // X range of ArtZ II 6x8" tablet
	xabs.absinfo.fuzz = 0;
	xabs.absinfo.flat = 0;
	//xabs.absinfo.resolution = xResolution;  // pixels per inch
	xabs.absinfo.value = 0;

	if (ioctl(fd, UI_ABS_SETUP, &xabs) < 0) {
		perror("error: ioctl UI_ABS_SETUP");
		return -1;
	}

	//
	// Y Axis
	//
	struct uinput_abs_setup yabs;
	memset(&yabs, 0, sizeof(yabs));

	yabs.code = ABS_Y;
	yabs.absinfo.minimum = 0; // limit min range to 1920
	yabs.absinfo.maximum = 480;  // Y range of tablet
	yabs.absinfo.fuzz = 0;
	yabs.absinfo.flat = 0;
	//yabs.absinfo.resolution = yResolution;  // pixels per inch
	yabs.absinfo.value = 0;

	if (ioctl(fd, UI_ABS_SETUP, &yabs) < 0) {
		perror("error: ioctl UI_ABS_SETUP");
		return -1;
	}

	ioctl(fd, UI_DEV_CREATE);

	return fd;
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
