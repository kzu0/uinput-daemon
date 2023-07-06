#include "uinput.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <tslib.h>
#include <QDebug>
#include <QString>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <QDebug>
//---------------------------------------------------------------------------------------------------------------------
// https://github.com/libts/tslib#setup-and-configure-tslib
//---------------------------------------------------------------------------------------------------------------------
#define SLOTS 10
#define SAMPLES 1
//---------------------------------------------------------------------------------------------------------------------
int fd = -1;
bool terminate = false;
struct tsdev *ts = NULL;
//---------------------------------------------------------------------------------------------------------------------
void catch_sigkill(int) {

	qDebug() << "catch_sigkill";
	terminate = true;
}
//---------------------------------------------------------------------------------------------------------------------
void catch_sigterm(int) {

	qDebug() << "catch_sigterm";
	terminate = true;
}
//---------------------------------------------------------------------------------------------------------------------
void catch_sigsegv(int) {

	qDebug() << "catch_sigsegv";
	terminate = true;
}
//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {

	int pressure = 0;

	signal(SIGKILL, catch_sigkill);
	signal(SIGSEGV, catch_sigsegv);
	signal(SIGTERM, catch_sigterm);

	//
	// Inizializzazione Uinput
	//
	fd = UinputInit();
	if (fd < 0) {
		perror("uinput_init error");
		return -1;
	}

	//
	// Inizializzazione TSLib
	//
	char *tsdevice = NULL;
	struct ts_sample_mt **samp_mt = NULL;
	int ret, i, j;

	ts = ts_setup(tsdevice, 0);
	if (!ts) {
		perror("ts_setup");
		qDebug() << "ts_setp_error";
		return -1;
	}

	samp_mt = (ts_sample_mt **)malloc(SAMPLES * sizeof(struct ts_sample_mt *));
	if (!samp_mt) {
		ts_close(ts);
		qDebug() << "malloc error";
		return -1;
	}
	for (i = 0; i < SAMPLES; i++) {
		samp_mt[i] = (ts_sample_mt*)calloc(SLOTS, sizeof(struct ts_sample_mt));
		if (!samp_mt[i]) {
			for (i--; i >= 0; i--) {
				free(samp_mt[i]);
			}
			free(samp_mt);
			ts_close(ts);
			qDebug() << "calloc error";
			return -1;
		}
	}

	//
	// Ascolto gli eventi della TSLib
	//
	while (1) {

		if (terminate) {
			goto exit;
		}

		ret = ts_read_mt(ts, samp_mt, SLOTS, SAMPLES);
		if (ret < 0) {
			perror("ts_read_mt");
			qDebug() << "ts_read_mt_error";
			ts_close(ts);
			exit(1);
		}

		for (j = 0; j < ret; j++) {
			for (i = 0; i < SLOTS; i++) {

				if (terminate) {
					goto exit;
				}

#ifdef TSLIB_MT_VALID
				if (!(samp_mt[j][i].valid & TSLIB_MT_VALID))
					continue;
#else
				if (samp_mt[j][i].valid < 1)
					continue;
#endif

				// Coordinate assolute dell'evento della TSLib
				int abs_x = samp_mt[j][i].x;
				int abs_y = samp_mt[j][i].y;

				__time_t sec = samp_mt[j][i].tv.tv_sec;
				__suseconds_t usec = samp_mt[j][i].tv.tv_usec;

				if ((fd != -1)) {

					// Move
					if (samp_mt[j][i].pressure == 255 and pressure == 255) {

						qDebug() << "Move:" << abs_x << abs_y;
						UinputEvent(fd, EV_ABS, ABS_X, abs_x, sec, usec);
						UinputEvent(fd, EV_ABS, ABS_Y, abs_y, sec, usec);
						UinputEvent(fd, EV_SYN, SYN_REPORT, 0, sec, usec);

					} else {

						// Press
						if (samp_mt[j][i].pressure == 255 and pressure == 0) {

							qDebug() << "Press:" << abs_x << abs_y;
							UinputEvent(fd, EV_ABS, ABS_X, abs_x, sec, usec);
							UinputEvent(fd, EV_ABS, ABS_Y, abs_y, sec, usec);
							UinputEvent(fd, EV_KEY, BTN_TOUCH, 1, sec, usec);
							UinputEvent(fd, EV_SYN, SYN_REPORT, 0, sec, usec);

						} else {

							// Release
							if (samp_mt[j][i].pressure == 0 and pressure == 255) {

								qDebug() << "Release:" << abs_x << abs_y;
								UinputEvent(fd, EV_ABS, ABS_X, abs_x, sec, usec);
								UinputEvent(fd, EV_ABS, ABS_Y, abs_y, sec, usec);
								UinputEvent(fd, EV_KEY, BTN_TOUCH, 0, sec, usec);
								UinputEvent(fd, EV_SYN, SYN_REPORT, 0, sec, usec);
							}
						}
					}
				}

				pressure = samp_mt[j][i].pressure;
			}
		}
	}

	exit:

	qDebug() << "Terminating...";

	//
	// Close TSLib
	//
	if (ts != NULL) {
		ts_close(ts);
	}

	/*
	* Give userspace some time to read the events before we destroy the
	* device with UI_DEV_DESTROY.
	*/
	sleep(1);

	UinputClose(fd);

	return 0;

	(void)(argc);
	(void)(argv);
}

