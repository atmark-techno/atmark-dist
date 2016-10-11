#ifndef _A210_COMMON_H_
#define _A210_COMMON_H_

#define REBOOT_RESTART	(0x01234567) //LINUX_REBOOT_CMD_RESTART
#define REBOOT_CAD_ON	(0x89abcdef) //LINUX_REBOOT_CMD_CAD_ON

#define FLATFSD_PID_FILE	("/var/run/flatfsd.pid")
#define SERI2ETH_PID_FILE	("/var/run/seri2eth.pid")

int system_check_executable(char *command);
void system_reboot(int flag);
int system_save_config(void);
int system_get_processid(char *pidfile);
int system_reset_seri2eth(void);

#endif
