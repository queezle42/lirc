
/****************************************************************************
 ** hw_receive.c ************************************************************
 ****************************************************************************
 *
 * routines for regression test receiving
 *
 * Copyright (C) 1999 Christoph Bartelmus <lirc@bartelmus.de>
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>


#include "lirc_driver.h"

#include "default.h"

extern struct ir_remote *repeat_remote;

static const const struct hardware hw_simreceive = {
	LIRC_DRIVER_DEVICE,	/* default device */
	-1,			/* fd */
	0,			/* features */
	0,			/* send_mode */
	0,			/* rec_mode */
	0,			/* code_length */
	default_init,		/* init_func */
	default_deinit,		/* deinit_func */
	default_send,		/* send_func */
	default_rec,		/* rec_func */
	receive_decode,		/* decode_func */
	default_ioctl,		/* ioctl_func */
	default_readdata,
	"simreceive"
};


const struct hardware* hardwares[] = { &hw_simreceive, (const struct hardware*)NULL };


/**********************************************************************
 *
 * decode stuff
 *
 **********************************************************************/

int default_readdata(lirc_t timeout)
{
	int data, ret;

	if (!waitfordata((long)timeout))
		return 0;
	while (1) {
		__u32 scan;

		ret = fscanf(stdin, "space %u\n", &scan);
		if (ret == 1) {
			data = (int) scan;
			break;
		}

		ret = fscanf(stdin, "pulse %u\n", &scan);
		if (ret == 1) {
			data = (int) scan | PULSE_BIT;
			break;
		}

		ret = fscanf(stdin, "%*s\n");
		if (ret == EOF){
			//logprintf(LOG_NOTICE, "simreceive: EOF");
			//dosigterm(SIGTERM);
			kill(getpid(), SIGUSR1);
		}
	}
	return data ;
}

/*
  interface functions
*/

int default_init()
{
	hw.fd = STDIN_FILENO;
	hw.features = LIRC_CAN_REC_MODE2;
	hw.send_mode = 0;
	hw.rec_mode = LIRC_MODE_MODE2;
	return (1);
}

int default_deinit(void)
{
	return (1);
}

static int write_send_buffer(int lirc)
{
	if (send_buffer.wptr == 0) {
		LOGPRINTF(1, "nothing to send");
		return (0);
	}
	return (write(lirc, send_buffer.data, send_buffer.wptr * sizeof(lirc_t)));
}

int default_send(struct ir_remote *remote, struct ir_ncode *code)
{
	/* things are easy, because we only support one mode */
	if (hw.send_mode != LIRC_MODE_PULSE)
		return (0);

	if (!init_send(remote, code))
		return (0);

	if (write_send_buffer(hw.fd) == -1) {
		logprintf(LOG_ERR, "write failed");
		logperror(LOG_ERR, NULL);
		return (0);
	}
 	return (1);
}

char *default_rec(struct ir_remote *remotes)
{
	if (!clear_rec_buffer()) {
		default_deinit();
		return NULL;
	}
	return (decode_all(remotes));
}

int default_ioctl(unsigned int cmd, void *arg)
{
	return ioctl(hw.fd, cmd, arg);
}