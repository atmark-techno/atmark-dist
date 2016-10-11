//==========================================================
//	File name	: seri2eth.c
//	Summary		: serial to ethernet program
//	Coded by	: F.Morishima
//	CopyRignt	: Atmark techno
//	Date		: 2003.12.15 created
//			: 2005.11.16 implement -d,-h option
//==========================================================

/* //////////////////// include files ///////////////////// */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include "serial_conf.h"

/* //////////////////// macro define ///////////////////// */

#ifdef __DEBUG__
#define LOG(args...)	printf(args)
#define ERR(args...)	printf(args)
#define PERROR(args...)	perror(args)
#define RESET_SIGNAL	SIGINT
#else
#define LOG(args...)	{}
#define ERR(args...)	{}
#define PERROR(args...)	{}
#define RESET_SIGNAL	SIGUSR1
#endif

/* //////////////////// define value ///////////////////// */
// about socket
#define DEFAULT_PORT_NO				21347

// timeout
#define CONNECT_RETRY_INTERVAL	1	// 1 sec

// interface definition
#define DEFAULT_SERIAL_DEVICE		"/dev/ttyS0"
#define RECV_BUF_SIZE		1500

#define PID_FILE		"/var/run/seri2eth.pid"

static sigset_t g_sigmask;
static int g_block_sock_fd = -1;
static int g_serial_fd = -1;
struct termios g_old_tio;
static int g_port_no = DEFAULT_PORT_NO;
static char g_lockfile[PATH_MAX];

//*********************************************************
//	summary		: prepare socket
//*********************************************************
int PrepareSocket(int mode)
{
	struct sockaddr_in localSock;
	int status;
	int sock_opt_on = 1;

	int fd = socket(AF_INET, mode, 0);
	if(fd < 0){
		ERR("failed to create socket : %d\n", fd);
		return fd;
	}

	bzero((char *)&localSock, sizeof(localSock));

	// set socket data
	localSock.sin_family		= AF_INET;
	localSock.sin_addr.s_addr	= INADDR_ANY;
	localSock.sin_port			= htons(g_port_no);
	LOG("port no : %d\n", g_port_no);

	// set option
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt_on, sizeof(int)) < 0){
		ERR("setsockopt() failed\n");
		PERROR(__FUNCTION__);
		close(fd);
		return -1;
	}
	if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &sock_opt_on, sizeof(int)) < 0){
		ERR("setsockopt() failed\n");
		PERROR(__FUNCTION__);
		close(fd);
		return -1;
	}

	// bind
	status = bind(fd, (struct sockaddr*)&localSock, sizeof(localSock));
	if(status < 0){
		ERR("bind() failed : %d\n", status);
		PERROR(__FUNCTION__);
		close(fd);
		return status;
	}

	return fd;
}

//*********************************************************
//	summary		: return baudrate setting
//*********************************************************
speed_t BaudrateSetting(
	unsigned long speed
)
{
	LOG("indicated baudrate value is '%d'\n", speed);
	switch(speed){
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 2400:
		return B2400;
	case 1800:
		return B1800;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	default:
		LOG("baudrate %d[bps] is not supported\n", speed);
		return 0;
	}
}

//*********************************************************
//	summary		: return datalen setting
//*********************************************************
void DataLengthSetting(
	datalen_e datalen,
	struct termios* tio
)
{
	LOG("datalen : %d\n", datalen);
	tio->c_cflag &= ~CSIZE;

	switch(datalen){
	case DATALEN5:
		tio->c_cflag |= CS5;
		break;
	case DATALEN6:
		tio->c_cflag |= CS6;
		break;
	case DATALEN7:
		tio->c_cflag |= CS7;
		break;
	case DATALEN8:
		tio->c_cflag |= CS8;
		break;
	default:
		LOG("datalen value '%d' is invalied\n", datalen);
		tio->c_cflag |= CS8;
		break;
	}
	return;
}

//*********************************************************
//	summary		: return parity setting
//*********************************************************
void ParitySetting(
	parity_e parity,
	struct termios* tio
)
{
	LOG("parity : %d\n", parity);
	switch(parity){
	case ODD_PARITY:
		tio->c_cflag |= PARENB;
		tio->c_cflag |= PARODD;
		break;
	case EVEN_PARITY:
		tio->c_cflag &= ~PARODD;
		tio->c_cflag |= PARENB;
		break;
	case NO_PARITY:
		tio->c_cflag &= ~PARENB;
		tio->c_cflag &= ~PARODD;
		break;
	default:
		LOG("parity value '%d' is invalid\n", parity);
		break;
	}

	return;
}

//*********************************************************
//	summary		: return stop bit setting
//*********************************************************
void StopbitSetting(
	stop_bit_e stop_bit,
	struct termios* tio
)
{
	LOG("stop bit : %d\n", stop_bit);
	switch(stop_bit){
	case ONE_BIT:
		tio->c_cflag &= ~CSTOPB;
		break;
	case TWO_BIT:
		tio->c_cflag |= CSTOPB;
		break;
	default:
		LOG("stop bit value '%d' is invalid\n", stop_bit);
	}
	return;
}

//*********************************************************
//	summary		: return flow ctrl setting
//*********************************************************
void FlowctrlSetting(
	flow_ctrl_e flow_ctrl,
	struct termios* tio
)
{
	LOG("flow ctrl : %d\n", flow_ctrl);
	switch(flow_ctrl){
	case HARDWARE:
		tio->c_cflag |= CRTSCTS;
		break;
	case NO_FLOW:
		tio->c_cflag &= ~CRTSCTS;
		break;
	default:
		LOG("flow control value '%d' is invalid\n", flow_ctrl);
	}
		
	return;
}

//*********************************************************
//	summary		: set serial port attribute
//*********************************************************
int ConfigSerialPort(int fd)
{
	struct termios tio;
	speed_t speed = B9600;
	char buff[256];
	int num;
	FILE* fp;

        memset(&tio, 0, sizeof(struct termios));
        tcgetattr(fd, &tio);

	// set default (8bit, no parity, 1stop, 9600)
	tio.c_iflag = IGNPAR | IGNBRK; 
	tio.c_oflag = 0; 
	tio.c_cflag = (speed | CS8 | CLOCAL | CREAD); 
	tio.c_lflag &= (~(ECHO|ECHONL|ICANON|ISIG)); 
	
	tio.c_cc[VTIME] = 0; 
	tio.c_cc[VMIN]  = 0; 
	
	cfsetispeed(&tio, speed);
	cfsetospeed(&tio, speed);

	fp = fopen(SERIAL_CONFIG_FILE, "r");
	if(!fp){
		ERR("failed to open config file : %s\n",
			SERIAL_CONFIG_FILE);
#if 0
		cfsetispeed(&tio, speed);
		cfsetospeed(&tio, speed);
#endif
		tcsetattr(fd, TCSAFLUSH, &tio);
		return -1;
	}

	/* last description in config file is enable */
	while(fgets(buff, sizeof(buff), fp)){
		const char* fmt = "%s %u";
		char key[64];
		unsigned long value;

		num = sscanf(buff, fmt, key, &value);
		if(num != 2){
			ERR("sscanf() failed\n");
			continue;
		}

		if(!strcmp(key, BAUDRATE_KEY_STR)){
			speed_t tmp = BaudrateSetting(value);
			if(tmp){
				speed = tmp;
				LOG("baudrate has changed\n", speed);
			}
		}
		else if(!strcmp(key, DATALEN_KEY_STR)){
			DataLengthSetting((datalen_e)value, &tio);
		}
		else if(!strcmp(key, PARITY_KEY_STR)){
			ParitySetting((parity_e)value, &tio);
		}
		else if(!strcmp(key, STOPBIT_KEY_STR)){
			StopbitSetting((stop_bit_e)value, &tio);
		}
		else if(!strcmp(key, FLOWCTRL_KEY_STR)){
			FlowctrlSetting((flow_ctrl_e)value, &tio);
		}
		else{
			LOG("%s is not defined key word\n", key);
			continue;
		}
	}
	fclose(fp);

	LOG("serial, i : %08o, c : %08o\n", tio.c_iflag, tio.c_cflag);

	// set serial parameter
	cfsetispeed(&tio, speed);
	cfsetospeed(&tio, speed);
	tcsetattr(fd, TCSAFLUSH, &tio);

	return 0;
}

//*********************************************************
//	summary		: receive socket data / serial data
//*********************************************************
static void WaitUDPData(
	int sock_fd,			// socket file desc.
	int serial_fd,			// serial file desc.
	unsigned long toIP		// IP address to send data	
)
{
	fd_set readfds;
	int msg_len;
	static char recv_buf[RECV_BUF_SIZE];
	struct sockaddr_in sentSock;
	struct sockaddr_in destSock;
	int len = sizeof(sentSock);

	bzero((char*)&destSock, sizeof(destSock));
	destSock.sin_family			= AF_INET;
	destSock.sin_port			= htons(g_port_no);
	destSock.sin_addr.s_addr	= toIP;

	while(1){
		// set FD
		int nfds = (serial_fd > sock_fd)? serial_fd : sock_fd;
		FD_ZERO(&readfds);
		FD_SET(sock_fd, &readfds);
		FD_SET(serial_fd, &readfds);

		LOG("call select()\n");
		sigprocmask(SIG_UNBLOCK, &g_sigmask, 0);
		if(select(nfds + 1, &readfds, NULL, NULL, NULL) < 0){
			ERR("select() failed\n");
			break;
		}
		sigprocmask(SIG_BLOCK, &g_sigmask, 0);

		if(FD_ISSET(sock_fd, &readfds)){
			/* receive socket data and write to serial */
			msg_len = recvfrom(sock_fd, recv_buf, RECV_BUF_SIZE, 0,
								(struct sockaddr *)&sentSock, &len);
			LOG("socket data receive, size : %d\n", msg_len);
			if(msg_len <= 0){
				ERR("recvfrom failed : %d\n", msg_len);
				break;
			}
			if(write(serial_fd, recv_buf, msg_len) < 0){
				ERR("write() failed\n");
				break;
			}
		}
		if(FD_ISSET(serial_fd, &readfds)){
			/* read serial data and send to socket */
			msg_len = read(serial_fd, recv_buf, RECV_BUF_SIZE);
			LOG("serial data receive, size : %d\n", msg_len);
			if(msg_len <= 0){
				ERR("failed to receive data : %d\n", msg_len);
				break;
			}
			if(!destSock.sin_addr.s_addr){
				LOG("no need to send received data from serial\n");
				continue;
			}
			if(sendto(sock_fd, recv_buf, msg_len, 0,
				(struct sockaddr *)&destSock, sizeof(destSock)) < 0){
				ERR("sendto() failed\n");
				break;
			}
		}
	}

	return;
}

//*********************************************************
//	summary		: receive socket data / serial data
//*********************************************************
static int WaitTCPData(
	int sock_fd,			// socket file desc.
	int serial_fd			// serial file desc.
)
{
	fd_set readfds;
	fd_set errfds;
	int msg_len;
	static char recv_buf[RECV_BUF_SIZE];

	while(1){
		// set FD
		int nfds = (serial_fd > sock_fd)? serial_fd : sock_fd;
		FD_ZERO(&readfds);
		FD_ZERO(&errfds);
		FD_SET(sock_fd, &readfds);
		FD_SET(serial_fd, &readfds);

		LOG("call select()\n");
		sigprocmask(SIG_UNBLOCK, &g_sigmask, 0);
		if(select(nfds + 1, &readfds, NULL, NULL, NULL) < 0){
			ERR("select() failed\n");
			return -1;
		}
		sigprocmask(SIG_BLOCK, &g_sigmask, 0);

		if(FD_ISSET(sock_fd, &readfds)){
			/* receive socket data and write to serial */
			msg_len = recv(sock_fd, recv_buf, RECV_BUF_SIZE, 0);
			LOG("socket data receive, size : %d\n", msg_len);
			if(msg_len <= 0){
				break;
			}
			if(write(serial_fd, recv_buf, msg_len) < 0){
				ERR("write() failed\n");
				break;
			}
		}
		if(FD_ISSET(serial_fd, &readfds)){
			/* read serial data and send to socket */
			msg_len = read(serial_fd, recv_buf, RECV_BUF_SIZE);
			LOG("serial data receive, size : %d\n", msg_len);
			if(msg_len <= 0){
				break;
			}
			if(send(sock_fd, recv_buf, msg_len, 0) < 0){
				ERR("send() failed\n");
				break;
			}
		}
	}

	return 0;
}

//*********************************************************
//	summary		: run as UDP mode
//*********************************************************
static int UDPMode(int serial_fd, unsigned long toIP)
{
	int sock_fd = PrepareSocket(SOCK_DGRAM);
	if(sock_fd < 0){
		ERR("failed to prepare socket : %d\n", sock_fd);
		return -1;
	}

	ConfigSerialPort(serial_fd);

	////
	// wait data received
	////
	WaitUDPData(sock_fd, serial_fd, toIP);

	shutdown(sock_fd, SHUT_RDWR);
	close(sock_fd);
	return 0;
}

//*********************************************************
//	summary		: run as TCP client mode
//*********************************************************
static int TCPClientMode(int serial_fd, unsigned long destIP)
{
	while(1){
		int ret;
		int new_sock_fd;
		struct sockaddr_in caddr;
		//socklen_t len = sizeof(caddr);

		memset((char*)&caddr, 0, sizeof(caddr));
		caddr.sin_family		= AF_INET;
		caddr.sin_port			= htons(g_port_no);
		caddr.sin_addr.s_addr	= destIP;

		////
		// prepare listen socket
		////
		g_block_sock_fd = PrepareSocket(SOCK_STREAM);
		if(g_block_sock_fd < 0){
			ERR("failed to prepare socket : %d\n", g_block_sock_fd);
			return -1;
		}

		////
		// try to connect to server (infinite)
		////
		while(1){
			sigprocmask(SIG_UNBLOCK, &g_sigmask, 0);
			ret = connect(
					g_block_sock_fd, (struct sockaddr*)&caddr, sizeof(caddr));
			sigprocmask(SIG_BLOCK, &g_sigmask, 0);
			if(ret < 0){
				ERR("connect() is failed : %d\n", ret);
				if(g_block_sock_fd < 0){
					// signal received
					return 0;
				}
				else{
					// connect timed out (retry after sleep short while)
					int ret;
					struct timeval timeout;
					timeout.tv_usec = 0;
					timeout.tv_sec = CONNECT_RETRY_INTERVAL;
					sigprocmask(SIG_UNBLOCK, &g_sigmask, 0);
					ret = select(1, NULL, NULL, NULL, &timeout);
					sigprocmask(SIG_BLOCK, &g_sigmask, 0);
					if(!ret){
						// signal received
						return 0;
					}
					continue;
				}
			}
			// let's begin conversation
			break;
		}

		new_sock_fd = g_block_sock_fd;
		g_block_sock_fd = -1;

		// do serial configuration as soon as possible after connected
		ConfigSerialPort(serial_fd);
		LOG("connected\n");

		////
		// wait data received
		////
		ret = WaitTCPData(new_sock_fd, serial_fd);

		shutdown(new_sock_fd, SHUT_RDWR);
		close(new_sock_fd);
		tcflush(serial_fd, TCIOFLUSH);

		if(ret < 0){
			// need to configure again
			LOG("USR1 signal received\n");
			break;
		}
	}

	return 0;
}

//*********************************************************
//	summary		: run as TCP server mode
//*********************************************************
static int TCPServerMode(int serial_fd, unsigned long destIP)
{
	while(1){
		int ret;
		int new_sock_fd;
		struct sockaddr_in caddr;
		socklen_t len = sizeof(caddr);

		////
		// prepare listen socket
		////
		g_block_sock_fd = PrepareSocket(SOCK_STREAM);
		if(g_block_sock_fd < 0){
			ERR("failed to prepare socket : %d\n", g_block_sock_fd);
			close(g_block_sock_fd);
			return -1;
		}

		ret = listen(g_block_sock_fd, 1);
		if(ret < 0){
			ERR("listen() is failed : %d\n", ret);
			close(g_block_sock_fd);
			g_block_sock_fd = -1;
			return -1;
		}

		////
		// wait for accept and when accept new connection
		////
		LOG("wait for access\n");
		sigprocmask(SIG_UNBLOCK, &g_sigmask, 0);
		new_sock_fd = accept(g_block_sock_fd, (struct sockaddr *)&caddr, &len);
		sigprocmask(SIG_BLOCK, &g_sigmask, 0);

		// close listen socket so that only one client can connect
		close(g_block_sock_fd);
		g_block_sock_fd = -1;

		if(new_sock_fd < 0){
			PERROR(__FUNCTION__);
			ERR("accept() failed : %d\n", new_sock_fd);
			return -1;
		}

		if(destIP && caddr.sin_addr.s_addr != destIP){
			ERR("connected ip is not destination IP: %x\n", destIP);
			close(new_sock_fd);
			continue;
		}

		// do serial configuration as soon as possible after connected
		ConfigSerialPort(serial_fd);
		LOG("connected\n");

		////
		// wait data received
		////
		ret = WaitTCPData(new_sock_fd, serial_fd);

		shutdown(new_sock_fd, SHUT_RDWR);
		close(new_sock_fd);
		tcflush(serial_fd, TCIOFLUSH);

		if(ret < 0){
			// need to configure again
			LOG("USR1 signal received\n");
			break;
		}
	}

	return 0;
}

//*********************************************************
//	summary		: Get MODE and IP from file
//  return      : 1 - UDP mode , 0 - TCP mode
//*********************************************************
static sock_proto_e GetMode(unsigned long* ip)
{
	char buff[256];
	sock_proto_e proto = TCPSERVER;
	FILE* fp;
	*ip = 0;
	if(!(fp = fopen(SERIAL_CONFIG_FILE, "r"))){
		ERR("failed to open config file : %s\n", SERIAL_CONFIG_FILE);
		return proto;
	}

	while(fgets(buff, sizeof(buff), fp)){
		const char* fmt = "%s %u";
		char key[64];
		unsigned long value;

		if(sscanf(buff, fmt, key, &value) != 2){
			ERR("sscanf() failed\n");
			continue;
		}

		if(!strcmp(key, CONNECT_ADDR_KEY_STR)){
			*ip = value;
			continue;
		}

		if(!strcmp(key, SOCK_PROTO_KEY_STR)){
			proto = value;
			continue;
		}

		if(!strcmp(key, PORTNO_KEY_STR)){
			if(PORTNO_RANGE_MIN <= value &&
			   value <= PORTNO_RANGE_MAX){
				g_port_no = value;
			}
		}
	}

	fclose(fp);
	return proto;
}

//*********************************************************
//	summary		: main function (program starts here)
//*********************************************************
static char *sbasename(const char *dev, char *res, int reslen)
{
	char *p;
	const char *q;

	if (strncmp(dev, "/dev/", 5) == 0) {
		/* In /dev */
		strncpy(res, dev + 5, reslen - 1);
		res[reslen-1] = 0;
		for (p = res; *p; p++)
			if (*p == '/')
				*p = '_';
	} else {
		/* Outside of /dev. Do something sensible. */
		if ((q = strrchr(dev, '/')) == NULL)
			q = dev;
		else
			q++;
		strncpy(res, q, reslen - 1);
		res[reslen-1] = 0;
	}

	return res;
}

static int serial_lock(const char *device)
{
	char buf[PATH_MAX];
	struct passwd *pwd;
	int mask;
	int fd;
	int size = 0;
	int pid = -1;
	
	snprintf(g_lockfile, sizeof(g_lockfile), "%s/LCK..%s",
		 UUCPLOCK, sbasename(device, buf, sizeof(buf)));
	
	fd = open(g_lockfile, O_RDONLY);
	if (fd >= 0) {
		size = read(fd, buf, sizeof(buf));
		close(fd);
		if (size > 0) {
			if (size == 4)
				/* Kermit-style lockfile. */
				pid = *(int *)buf;
			else if (size > 0) {
				/* Ascii lockfile. */
				buf[size] = 0;
				sscanf(buf, "%d", &pid);
			}
			if (pid > 0) {
				kill((pid_t)pid, 0);
				if (errno == ESRCH) {
					/* death lockfile - remove it */
					ERR("Lockfile is stale. Overriding it..\n");
					unlink(g_lockfile);
					sleep(1);
				} else
					size = 0;
			}
		}
		if (size == 0) {
			ERR("Device %s is locked.\n", device);
			return -1;
		}
	}

	mask = umask(022);
	fd = open(g_lockfile, O_RDWR | O_EXCL | O_CREAT,
			     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	umask(mask);
	if (fd < 0) {
		ERR("Cannot create lockfile.");
		return -1;
	}

	pwd = getpwuid(getuid());
	if (pwd == (struct passwd *)0) {
		ERR("You don't exist. Go away.\n");
		return -1;
	}

	snprintf(buf, sizeof(buf), "%10ld seri2eth %.20s\n",
		 (long)getpid(), pwd->pw_name);
	write(fd, buf, strlen(buf));

	close(fd);

	return 0;
}

static void serial_unlock(void)
{
	remove(g_lockfile);
}

void abort_signal(int sig)
{
	LOG("signal received\n");
	if(g_block_sock_fd >= 0){
		LOG("close waiting socket : accept()?\n");
		close(g_block_sock_fd);
		g_block_sock_fd = -1;
	}
}

void term_signal(int sig)
{
	LOG("SIGTERM received\n");
	remove(PID_FILE);
	serial_unlock();
	tcsetattr(g_serial_fd, TCSANOW, &g_old_tio);
	abort_signal(0);
	exit(0);
}

int create_pid_file(void)
{
	FILE *fp;
	fp = fopen(PID_FILE, "w");
	if(fp == NULL) return -1;
	fprintf(fp, "%d\n", getpid());
	fclose(fp);
	
	return 0;
}

static void usage(void){
	printf("Usage: seri2eth [OPTION] \n");
	printf("\n");
	printf("  -h          display this message\n");
	printf("  -d DEVICE   use device\n");
	printf("\n");
}

int main(int argc, char* argv[])
{
	int serial_fd;
	struct termios old_tio;
	char *device = NULL;
	int ret;
	struct option optinfo[]={
	  {"help",0,NULL,'h'},
	  {"device",1,0,'d'},
	  {NULL,0,0,0},
	};

	while(1){
	  int ch = getopt_long_only(argc,argv,"hd:",optinfo,NULL);
	  if(ch == -1) break;
	  switch(ch){
	  case 0:
	    break;
	  case 'h':
	    usage();
	    return 0;
	  case 'd':
	    device = optarg;
	    break;
	  }	    
	}

	if(device == NULL){
	  device = DEFAULT_SERIAL_DEVICE;
	}

	ret = serial_lock(device);
	if (ret < 0)
		return -1;

	// open serial device and backup default setting
	//int serial_fd = open(device, O_RDWR | O_NOCTTY);
	serial_fd = open(device, O_RDWR);
	if(serial_fd < 0){
		ERR("failed to open serial dev, %s\n", device);
		serial_unlock();
		return -1;
	}
	tcgetattr(serial_fd, &old_tio);

	g_serial_fd = serial_fd;
	g_old_tio = old_tio;

	LOG("mask setting\n");

	// signal setting
	sigemptyset(&g_sigmask);
	sigaddset(&g_sigmask, RESET_SIGNAL);
	sigaddset(&g_sigmask, SIGTERM);

	sigprocmask(SIG_BLOCK, &g_sigmask, 0);

	signal(RESET_SIGNAL, abort_signal);
	signal(SIGTERM, term_signal);

	create_pid_file();

	while(1){
		unsigned long ip = 0;
		sock_proto_e proto = GetMode(&ip);

		LOG("MODE : %d\n", proto);
		switch(proto){
		case TCPSERVER:
			TCPServerMode(serial_fd, ip);
			break;
		case TCPCLIENT:
			if(ip){
				TCPClientMode(serial_fd, ip);
			}
			else{
				// wait for setting is changed
				sigprocmask(SIG_UNBLOCK, &g_sigmask, 0);
				select(1, NULL, NULL, NULL, NULL);
				sigprocmask(SIG_BLOCK, &g_sigmask, 0);
			}
			break;
		case UDP:
			UDPMode(serial_fd, ip);
			break;
		default:
			break;
		}
	}
	tcsetattr(serial_fd, TCSANOW, &old_tio);
	close(serial_fd);

	serial_unlock();

	return 0;
}
