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
//---------------------------------------------------------------------------------------------------------------------
// https://github.com/libts/tslib#setup-and-configure-tslib
//---------------------------------------------------------------------------------------------------------------------
#define SLOTS 1
#define SAMPLES 1
//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {

	int fd = -1;
	int abs_x = 0;
	int abs_y = 0;
	int pressure = 0;

	//
	// Inizializzazione Uinput
	//
	fd = UinputInit();
	if (fd < 0) {
		perror("uinput_init error");
		return -1;
	}

	//
	// Scrive l'indice del device in un file per
	// notificare all'applicazione che deve
	// resettare la posizione del cursore
	//
	/*
	FILE* fp = NULL;
	fp = fopen("/tmp/uinput", "wb");
	if (fp != NULL) {
		char str[16];
		sprintf(str, "%d\n", fd);
		fputs(str, fp);
		fclose(fp);
	}
	*/

	//
	// Inizializzazione TSLib
	//
	struct tsdev *ts;
	char *tsdevice = NULL;
	struct ts_sample_mt **samp_mt = NULL;
	int ret, i, j;

	ts = ts_setup(tsdevice, 0);
	if (!ts) {
		perror("ts_setup");
		return -1;
	}

	samp_mt = (ts_sample_mt **)malloc(SAMPLES * sizeof(struct ts_sample_mt *));
	if (!samp_mt) {
		ts_close(ts);
		return -ENOMEM;
	}
	for (i = 0; i < SAMPLES; i++) {
		samp_mt[i] = (ts_sample_mt*)calloc(SLOTS, sizeof(struct ts_sample_mt));
		if (!samp_mt[i]) {
			for (i--; i >= 0; i--) {
				free(samp_mt[i]);
			}
			free(samp_mt);
			ts_close(ts);
			return -ENOMEM;
		}
	}

	//
	// Necessario altrimenti il primo evento
	// non viene raccolto dalle Qt
	//
	sleep(1);
	UinputEvent(fd, EV_REL, REL_X, -800);
	UinputEvent(fd, EV_REL, REL_Y, -480);
	UinputEvent(fd, EV_SYN, SYN_REPORT, 0);


	//
	// Shared memory segment in cui sono salvate
	// le coordinate del cursore delle Qt
	//
	FILE *fp;
	fp = fopen("/tmp/shmid.txt", "r");
	if (fp == NULL) {
		perror("fopen");
		ts_close(ts);
		exit(1);
	}

	int shmid;
	fscanf (fp, "%d", &shmid);
	fclose(fp);




	//
	// Ascolto gli eventi della TSLib
	//
	while (1) {

		ret = ts_read_mt(ts, samp_mt, SLOTS, SAMPLES);
		if (ret < 0) {
			perror("ts_read_mt");
			ts_close(ts);
			exit(1);
		}

		for (j = 0; j < ret; j++) {
			for (i = 0; i < SLOTS; i++) {

#ifdef TSLIB_MT_VALID
				if (!(samp_mt[j][i].valid & TSLIB_MT_VALID))
					continue;
#else
				if (samp_mt[j][i].valid < 1)
					continue;
#endif

				/*
				printf("%ld.%06ld: (slot %d) %6d %6d %6d\n",
							 samp_mt[j][i].tv.tv_sec,
							 samp_mt[j][i].tv.tv_usec,
							 samp_mt[j][i].slot,
							 samp_mt[j][i].x,
							 samp_mt[j][i].y,
							 samp_mt[j][i].pressure);

							 */

				// Press
				if (samp_mt[j][i].pressure == 255 and pressure == 0) {
					if (shmid >= 0) {
						int* shared_mem = (int *)shmat(shmid, NULL, 0);

						char str[16];
						sprintf(str, "%s", shared_mem);

						QString s(str);
						QStringList list = s.split(" ");

						if (list.size() >= 2) {
							abs_x = list.at(0).toInt();
							abs_y = list.at(1).toInt();
						}
						qDebug() << abs_x << abs_y;
					}
				}

				// Coordinate assolute dell'evento della TSLib
				int samp_x = samp_mt[j][i].x;
				int samp_y = samp_mt[j][i].y;

				// Coordinate relative rispetto all'ultimo evento registrato
				int rel_x = samp_x - abs_x;
				int rel_y = samp_y - abs_y;

				__time_t sec = samp_mt[j][i].tv.tv_sec;
				__suseconds_t usec = samp_mt[j][i].tv.tv_usec;

				if ((fd != -1)) {

					// Move
					if (samp_mt[j][i].pressure == 255 and pressure == 255) {

						qDebug() << "Move:" << rel_x << rel_y;
						UinputEvent(fd, EV_REL, REL_X, rel_x, sec, usec);
						UinputEvent(fd, EV_REL, REL_Y, rel_y, sec, usec);
						UinputEvent(fd, EV_SYN, SYN_REPORT, 0, sec, usec);

					} else {

						// Press
						if (samp_mt[j][i].pressure == 255 and pressure == 0) {

							qDebug() << "Press:" << rel_x << rel_y;
							UinputEvent(fd, EV_REL, REL_X, rel_x, sec, usec);
							UinputEvent(fd, EV_REL, REL_Y, rel_y, sec, usec);
							UinputEvent(fd, EV_KEY, BTN_LEFT, 1, sec, usec);
							UinputEvent(fd, EV_SYN, SYN_REPORT, 0, sec, usec);

						} else {

							// Release
							if (samp_mt[j][i].pressure == 0 and pressure == 255) {

								qDebug() << "Release:" << rel_x << rel_y;
								UinputEvent(fd, EV_REL, REL_X, rel_x, sec, usec);
								UinputEvent(fd, EV_REL, REL_Y, rel_y, sec, usec);
								UinputEvent(fd, EV_KEY, BTN_LEFT, 0, sec, usec);
								UinputEvent(fd, EV_SYN, SYN_REPORT, 0, sec, usec);
							}
						}
					}
				}

				abs_x = samp_x;
				abs_y = samp_y;
				pressure = samp_mt[j][i].pressure;
			}
		}
	}

	ts_close(ts);

	return 0;

	(void)(argc);
	(void)(argv);
}

