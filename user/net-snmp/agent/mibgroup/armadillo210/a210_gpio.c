/************************************************************************
 file name : a210_gpio.c
 summary   : gpio control
 coded by  :
 copyright : Atmark techno
************************************************************************/
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <config/autoconf.h>
#undef MODE_GET
#include <asm/arch/armadillo210_gpio.h>

#include "util_funcs.h"
#include "a210_common.h"
#include "a210_gpio.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define MODULE_NAME "a210_gpio"
#define GPIO_DEVICE "/dev/gpio"

#define PORT_NUM 8

typedef struct _port_data{
  unsigned long mode;
  unsigned long value;
  unsigned long initvalue;
  unsigned long trapcase;
  unsigned long traptime;
}port_data;

typedef struct _gpio_info{
  in_addr_t manager;
  unsigned long check_cycle;
  port_data port[PORT_NUM];
}gpio_info;

static gpio_info g_info;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ configration file
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static char *gpio_config_file = NULL;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ magic numbers
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define A210_GPIO_MANAGER                1
#define A210_GPIO_CHECKCYCLE             2

#define A210_GPIO0_MODE                  11
#define A210_GPIO0_VALUE                 12
#define A210_GPIO0_INITVALUE             13
#define A210_GPIO0_TRAPCASE              14
#define A210_GPIO0_TRAPTIME              15

#define A210_GPIO1_MODE                  21
#define A210_GPIO1_VALUE                 22
#define A210_GPIO1_INITVALUE             23
#define A210_GPIO1_TRAPCASE              24
#define A210_GPIO1_TRAPTIME              25

#define A210_GPIO2_MODE                  31
#define A210_GPIO2_VALUE                 32
#define A210_GPIO2_INITVALUE             33
#define A210_GPIO2_TRAPCASE              34
#define A210_GPIO2_TRAPTIME              35

#define A210_GPIO3_MODE                  41
#define A210_GPIO3_VALUE                 42
#define A210_GPIO3_INITVALUE             43
#define A210_GPIO3_TRAPCASE              44
#define A210_GPIO3_TRAPTIME              45

#define A210_GPIO4_MODE                  51
#define A210_GPIO4_VALUE                 52
#define A210_GPIO4_INITVALUE             53
#define A210_GPIO4_TRAPCASE              54
#define A210_GPIO4_TRAPTIME              55

#define A210_GPIO5_MODE                  61
#define A210_GPIO5_VALUE                 62
#define A210_GPIO5_INITVALUE             63
#define A210_GPIO5_TRAPCASE              64
#define A210_GPIO5_TRAPTIME              65

#define A210_GPIO6_MODE                  71
#define A210_GPIO6_VALUE                 72
#define A210_GPIO6_INITVALUE             73
#define A210_GPIO6_TRAPCASE              74
#define A210_GPIO6_TRAPTIME              75

#define A210_GPIO7_MODE                  81
#define A210_GPIO7_VALUE                 82
#define A210_GPIO7_INITVALUE             83
#define A210_GPIO7_TRAPCASE              84
#define A210_GPIO7_TRAPTIME              85

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
struct variable4 a210_gpio_variables[] = {
  {A210_GPIO_MANAGER   , ASN_IPADDRESS, RWRITE, var_a210_gpio, 1, {1}},
  {A210_GPIO_CHECKCYCLE, ASN_INTEGER, RWRITE, var_a210_gpio, 1, {2}},

  {A210_GPIO0_MODE     , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,1,1}},
  {A210_GPIO0_VALUE    , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,1,2}},
  {A210_GPIO0_INITVALUE, ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,1,3}},
  {A210_GPIO0_TRAPCASE , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,1,4}},
  {A210_GPIO0_TRAPTIME , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,1,5}},

  {A210_GPIO1_MODE     , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,2,1}},
  {A210_GPIO1_VALUE    , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,2,2}},
  {A210_GPIO1_INITVALUE, ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,2,3}},
  {A210_GPIO1_TRAPCASE , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,2,4}},
  {A210_GPIO1_TRAPTIME , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,2,5}},

  {A210_GPIO2_MODE     , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,3,1}},
  {A210_GPIO2_VALUE    , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,3,2}},
  {A210_GPIO2_INITVALUE, ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,3,3}},
  {A210_GPIO2_TRAPCASE , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,3,4}},
  {A210_GPIO2_TRAPTIME , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,3,5}},

  {A210_GPIO3_MODE     , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,4,1}},
  {A210_GPIO3_VALUE    , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,4,2}},
  {A210_GPIO3_INITVALUE, ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,4,3}},
  {A210_GPIO3_TRAPCASE , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,4,4}},
  {A210_GPIO3_TRAPTIME , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,4,5}},

  {A210_GPIO4_MODE     , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,5,1}},
  {A210_GPIO4_VALUE    , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,5,2}},
  {A210_GPIO4_INITVALUE, ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,5,3}},
  {A210_GPIO4_TRAPCASE , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,5,4}},
  {A210_GPIO4_TRAPTIME , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,5,5}},

  {A210_GPIO5_MODE     , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,6,1}},
  {A210_GPIO5_VALUE    , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,6,2}},
  {A210_GPIO5_INITVALUE, ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,6,3}},
  {A210_GPIO5_TRAPCASE , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,6,4}},
  {A210_GPIO5_TRAPTIME , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,6,5}},

  {A210_GPIO6_MODE     , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,7,1}},
  {A210_GPIO6_VALUE    , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,7,2}},
  {A210_GPIO6_INITVALUE, ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,7,3}},
  {A210_GPIO6_TRAPCASE , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,7,4}},
  {A210_GPIO6_TRAPTIME , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,7,5}},

  {A210_GPIO7_MODE     , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,8,1}},
  {A210_GPIO7_VALUE    , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,8,2}},
  {A210_GPIO7_INITVALUE, ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,8,3}},
  {A210_GPIO7_TRAPCASE , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,8,4}},
  {A210_GPIO7_TRAPTIME , ASN_INTEGER, RWRITE, var_a210_gpio, 3, {10,8,5}},
};

oid a210_gpio_variables_oid[] = { 1, 3, 6, 1, 4, 1, 16031, 1, 4, 3 };

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ trap information
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
const char* objid_gpiochange = ".1.3.6.1.4.1.16031.1.4.3.100.0";
oid objid_gpiostatus[] = {1, 3, 6, 1, 4, 1, 16031, 1, 4, 3, 10, 0, 4, 0};
oid objid_sysuptime[]  = {1, 3, 6, 1, 2, 1, 1, 3, 0};
oid objid_snmptrap[]   = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

#define OID_GPIO_STATUS_LEN    14
#define DEFAULT_COMMUNITY_NAME "public"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ send_gpio_trap
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int send_gpio_trap(unsigned long manager_addr, unsigned long portNo){
  struct snmp_session session, *ss;
  struct snmp_pdu *pdu;
  char *trap = NULL;
  long sysuptime;
  char csysuptime[20];
  char buff[20];
  int err = 0;
  
  netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DEFAULT_PORT, 
		     SNMP_TRAP_PORT);
  
  snmp_sess_init(&session);
  
  session.callback          = NULL;
  session.callback_magic    = NULL;
  session.remote_port       = SNMP_TRAP_PORT;
  session.version           = SNMP_VERSION_2c;
  session.community         = DEFAULT_COMMUNITY_NAME;
  session.community_len     = strlen(DEFAULT_COMMUNITY_NAME);
  session.peername          = inet_ntoa(*(struct in_addr*)&manager_addr);
  
  DEBUGMSGTL((MODULE_NAME, "trap to ip : %s, port : %d\n",
	      session.peername, session.remote_port));
  
  ss = snmp_open(&session);
  if(ss == NULL){
    DEBUGMSGTL((MODULE_NAME, "snmp_open() failed()\n"));
    err = 1;
    goto err_out;
  }
  
  pdu = snmp_pdu_create(SNMP_MSG_TRAP2);

  /* up time */
  sysuptime = get_uptime();
  sprintf(csysuptime, "%ld", sysuptime);
  trap = csysuptime;
  
  snmp_add_var(pdu, objid_sysuptime,
	       sizeof(objid_sysuptime)/sizeof(oid), 't', trap);
  
  /* object ID */
  if(snmp_add_var(pdu, objid_snmptrap,
		  sizeof(objid_snmptrap)/sizeof(oid),
		  'o', objid_gpiochange) != 0) {
    DEBUGMSGTL((MODULE_NAME, "snmp_add_var() failed()\n"));
    err = 1;
    goto err_out;
  }
  
  /* add port status variable */
  objid_gpiostatus[OID_GPIO_STATUS_LEN - 3] = portNo + 1;
  sprintf(buff, "%lu", g_info.port[portNo].trapcase);
  if(snmp_add_var(pdu, objid_gpiostatus,
		  sizeof(objid_gpiostatus)/sizeof(oid),
		  'i', buff) != 0) {
    DEBUGMSGTL((MODULE_NAME, "snmp_add_var() failed()\n"));
    err = 1;
    goto err_out;
  }
  
  /* send trap packet */
  if(!snmp_send(ss, pdu)){
    DEBUGMSGTL((MODULE_NAME, "send trap failed()\n"));
    snmp_free_pdu(pdu);
  }
  
  snmp_close(ss);
  
 err_out:
  netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DEFAULT_PORT, 
		     SNMP_PORT);
  
  if(err) return -1;

  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define gen_set_output_param(value)      value,0,0,0
#define gen_set_input_param(enable,type) 0,enable,type,0
#define gen_get_param()                  0,0,0,0

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ add_param_list
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int add_param_list(struct gpio_param **head,
		   unsigned long no,unsigned long mode,
		   unsigned long arg1,unsigned long arg2,
		   unsigned long arg3,unsigned long arg4){
  struct gpio_param *add;
  struct gpio_param *tmp;

  add = (struct gpio_param *)malloc(sizeof(struct gpio_param));
  if(!add) return -1;

  memset(add,0,sizeof(struct gpio_param));

  add->no   = no;
  add->mode = mode;    

  switch(mode){
  case MODE_OUTPUT:
    add->data.o.value = arg1;
    break;
  case MODE_INPUT:
    add->data.i.int_enable = arg2;
    add->data.i.int_type = arg3;
    break;
  case MODE_GET:
    break;
  default:
    return -1;
  }

  if(*head){

    tmp = *head;
    while(tmp->next){
      tmp = tmp->next;
    }
    tmp->next = add;
  }else{
    *head = add;
  }

  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ free_param_list
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int free_param_list(struct gpio_param **head){
  struct gpio_param *tmp;
  struct gpio_param *current;
  int i = 0;

  if(!*head) return 0;

  current = *head;
  do{
    i++;
    tmp = current;
    current = current->next;
    free(tmp);
    
  }while(current);
  *head=0;

  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ set_port_info
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int set_port_info(gpio_info *info){
  struct gpio_param *param_list = NULL;
  int fd;
  int i;
  int ret;

  for(i=0;i<PORT_NUM;i++){
    switch(info->port[i].mode){
    case 0://OUTPUT
      add_param_list(&param_list, BIT(i), MODE_OUTPUT,
		     gen_set_output_param(info->port[i].value));
      break;
    case 1://INPUT
      add_param_list(&param_list, BIT(i), MODE_INPUT,
		     gen_set_input_param(0,0));
      break;
    }
  }

  fd = open(GPIO_DEVICE, O_RDWR);
  if(fd == -1){
    free_param_list(&param_list);
    perror("open");
    return -1;
  }

  ret = ioctl(fd, PARAM_SET, param_list);
  if(ret == -1){
    free_param_list(&param_list);
    perror("ioctl");
    close(fd);
    return -1;
  }

  free_param_list(&param_list);
  close(fd);
  
  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ get_port_info
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int get_port_info(struct gpio_param *param){
  int fd;
  int ret;

  fd = open(GPIO_DEVICE, O_RDWR);
  if(fd == -1){
    perror("open");
    return -1;
  }

  ret = ioctl(fd, PARAM_GET, param);
  if(ret == -1){
    perror("ioctl");
    close(fd);
    return -1;
  }

  close(fd);

  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ load_config
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define IGNORE_LABEL(LABEL) printf("%s: ignore\n",LABEL)

static char *label_member[5] = {
  "MODE","VALUE","INITVALUE","TRAPCASE","TRAPTIME"
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int load_config(void){
  FILE *fp;
  char buf[256];
  char tmpbuf[256];
  char *label,*value,*tmp;
  int i,j;

  fp = fopen(gpio_config_file, "r");
  if(fp == NULL) return -1;

  while(!feof(fp)){
    fgets(buf,512,fp);
    label = buf;

    label += strspn(label, " \t");
    if(label[0] == '#') continue;

    value = strpbrk(label, " \t\r\n");
    if(value == NULL) continue;
    value[0] = 0; value++;
    value += strspn(value, " \t");
    tmp = strpbrk(value, " \t\r\n");
    if(tmp == NULL) continue;
    tmp[0] = 0;

    if(strlen(label)==0) continue;
    if(strlen(value)==0) continue;

    if(strcmp("MANAGER",label) == 0){
      in_addr_t addr;
      addr = inet_addr(value);
      if(addr == INADDR_NONE){ IGNORE_LABEL(label); continue; }
      g_info.manager = addr;
      continue;
    }
    if(strcmp("CHECKCYCLE",label) == 0){
      unsigned long cycle;
      cycle = strtol(value, NULL, 10);

      if(cycle < 10 || 1000 < cycle) cycle = 100;
      g_info.check_cycle = cycle;
      continue;
    }
    for(i=0;i<8;i++){
      for(j=0;j<5;j++){
	sprintf(tmpbuf,"GPIO%d.%s",i,label_member[j]);
	if(strcmp(tmpbuf,label) == 0){
	  unsigned long data;
	  data = strtol(value, NULL, 10);

	  switch(j){
	  case 0://Mode
	    if(1 < data){ IGNORE_LABEL(tmpbuf); continue; }
	    g_info.port[i].mode = data;
	    break;
	  case 1://Value
	    IGNORE_LABEL(tmpbuf); continue;
	    break;
	  case 2://InitValue
	    if(1 < data){ IGNORE_LABEL(tmpbuf); continue; }
	    g_info.port[i].initvalue = data;
	    break;
	  case 3://TrapCase
	    if(1 < data){ IGNORE_LABEL(tmpbuf); continue; }
	    g_info.port[i].trapcase = data;
	    break;
	  case 4://TrapTime
	    if(604800000 < data){ IGNORE_LABEL(tmpbuf); continue; }
	    g_info.port[i].traptime = data;
	    break;
	  }
	  continue;
	}
      }
    }
  }
  fclose(fp);
    
  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ save_config
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int save_config(void){
  int i,j;
  struct in_addr inaddr;
  inaddr.s_addr = g_info.manager;

  FILE *fp;

  fp = fopen(gpio_config_file, "w");
  if(fp == NULL) return -1;

  fprintf(fp, "#\n");
  fprintf(fp, "# ARMADILLO-210-MIB::gpio\n");
  fprintf(fp, "#   Net-SNMP GPIO default value.\n");
  fprintf(fp, "#\n");  

  fprintf(fp, "\n### COMMON ############\n");

  fprintf(fp, "MANAGER %s\n", inet_ntoa(inaddr));
  fprintf(fp, "CHECKCYCLE %ld\n", g_info.check_cycle);

  for(i=0;i<8;i++){
    fprintf(fp, "\n### GPIO%d ############\n",i);
    for(j=0;j<5;j++){
      switch(j){
      case 0://Mode
	fprintf(fp, "GPIO%d.%s %ld\n",
		i,label_member[j],g_info.port[i].mode);
	break;
      case 1://Value
	break;
      case 2://InitValue
	fprintf(fp, "GPIO%d.%s %ld\n",
		i,label_member[j],g_info.port[i].initvalue);
	break;
      case 3://TrapCase
	fprintf(fp, "GPIO%d.%s %ld\n",
		i,label_member[j],g_info.port[i].trapcase);	
	break;
      case 4://TrapTime
	fprintf(fp, "GPIO%d.%s %ld\n",
		i,label_member[j],g_info.port[i].traptime);
	break;
      }
    }
  }
  fclose(fp);
  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
struct trap_timer_info{
  unsigned int id;
  int port_no;
  unsigned long count;
  unsigned long trap_count;
};

static struct trap_timer_info trap_timer[8];

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static void trap_timer_timeout_func(unsigned int id, void *param){
  struct trap_timer_info *info = (struct trap_timer_info *)param;
  struct gpio_param gpio;

  if(info->id == id){
    memset(&gpio, 0, sizeof(struct gpio_param));
    gpio.no = BIT(info->port_no);
    get_port_info(&gpio);

    if(g_info.port[info->port_no].trapcase == gpio.data.i.value){
      info->count++;
      if(info->count >= info->trap_count){
	printf("port[%d]: trap !!\n",info->port_no);

	send_gpio_trap(g_info.manager,info->port_no);
	info->count = 0;
      }
    }else{
      info->count = 0;
    }
  }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int trap_timer_start(int no){
  struct timeval timeout;

  if(no < 0 || 7 < no) return -1;
  if(g_info.check_cycle == 0) return -1;
  if(g_info.port[no].traptime == 0) return -1;

  if(trap_timer[no].id == 0){
    trap_timer[no].port_no = no;
    trap_timer[no].count = 0;
    trap_timer[no].trap_count = g_info.port[no].traptime / g_info.check_cycle;

    timeout.tv_sec  = (g_info.check_cycle / 1000);
    timeout.tv_usec = ((g_info.check_cycle % 1000) * 1000);

    trap_timer[no].id = \
      snmp_alarm_register_hr(timeout,
			     SA_REPEAT,
			     trap_timer_timeout_func,
			     (void *)&trap_timer[no]);
    //printf("%s(): port[%d]: %08x\n",__FUNCTION__,no,(u_int)&trap_timer[no]);
  }
  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int trap_timer_stop(int no){
  if(no < 0 || 7 < no) return -1;

  if(trap_timer[no].id != 0){
    snmp_alarm_unregister(trap_timer[no].id);
    trap_timer[no].id = 0;
    //printf("%s(): port[%d]: %08x\n",__FUNCTION__,no,(u_int)&trap_timer[no]);
  }
  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int trap_timer_restart(int no){
  trap_timer_stop(no);
  trap_timer_start(no);
  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int trap_timer_restart_all(void){
  int i;
  for(i=0;i<8;i++){
    if(trap_timer[i].id != 0){
      trap_timer_stop(i);
      trap_timer_start(i);
    }
  }
  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ write_manager_func
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int write_manager_func(int action, u_char *var_val, u_char var_val_type,
			      size_t var_val_len, u_char *statP, oid *name,
			      size_t name_len){
  unsigned long value = *(unsigned long *)var_val;

  switch(action){
  case RESERVE1:
    if(var_val_type != ASN_IPADDRESS){
      DEBUGMSGTL((MODULE_NAME, "not ipaddress\n"));
      return SNMP_ERR_WRONGTYPE;
    }
    break;

  case RESERVE2:
    break;

  case ACTION:
    g_info.manager = (in_addr_t)value;
    break;

  case COMMIT:
    g_info.manager = (in_addr_t)value;
    save_config();
    break;

  case FREE:
  case UNDO:
    break;
  default:
    break;
  }
  return SNMP_ERR_NOERROR;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ write_cycle_func
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int write_cycle_func(int action, u_char *var_val, u_char var_val_type,
			    size_t var_val_len, u_char *statP, oid *name,
			    size_t name_len){
  unsigned long value = *(unsigned long *)var_val;

  switch(action){
  case RESERVE1:
    if(var_val_type != ASN_INTEGER){
      DEBUGMSGTL((MODULE_NAME, "not integer\n"));
      return SNMP_ERR_WRONGTYPE;
    }
    if(value < 10 || 1000 < value) return SNMP_ERR_WRONGVALUE;
    break;

  case RESERVE2:
    break;

  case ACTION:
    g_info.check_cycle = value;
    trap_timer_restart_all();
    break;

  case COMMIT:
    g_info.check_cycle = value;
    save_config();
    break;

  case FREE:
  case UNDO:
    break;
  default:
    break;
  }
  return SNMP_ERR_NOERROR;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ write_gpio_func
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static int write_gpio_func(int action, u_char *var_val, u_char var_val_type,
			   size_t var_val_len, u_char *statP, oid *name,
			   size_t name_len){
  static int flag = 0;
  static gpio_info new_info;
  int gpio_no;
  int gpio_label;
  unsigned long value = *(unsigned long *)var_val;
  int i;
  int ret;

  gpio_no = name[name_len-3]-1;
  gpio_label = name[name_len-2];

  switch(action){
  case RESERVE1:
    //New data initialize
    if(flag == 0){
      memcpy(&new_info, &g_info, sizeof(gpio_info));
      flag = 1;
    }

    //Type check
    if(var_val_type != ASN_INTEGER){
      DEBUGMSGTL((MODULE_NAME, "not integer\n"));
      return SNMP_ERR_WRONGTYPE;
    }

    //Range check & New data create
    switch(gpio_label){
    case 1://Mode
      if(1 < value) return SNMP_ERR_WRONGVALUE;
      new_info.port[gpio_no].mode = value;
      break;
    case 2://Value
      if(1 < value) return SNMP_ERR_WRONGVALUE;
      new_info.port[gpio_no].value = value;
      break;      
    case 3://InitValue
      if(1 < value) return SNMP_ERR_WRONGVALUE;
      new_info.port[gpio_no].initvalue = value;
      break;
    case 4://TrapCase
      if(1 < value) return SNMP_ERR_WRONGVALUE;
      new_info.port[gpio_no].trapcase = value;      
      break;
    case 5://TrapTime
      if(604800000 < value) return SNMP_ERR_WRONGVALUE;
      new_info.port[gpio_no].traptime = value;
      break;
    default:
      return SNMP_ERR_GENERR;
    }
    break;

  case RESERVE2:
    //
    if(flag == 1){
      for(i = 0; i < PORT_NUM; i++){
	switch(new_info.port[i].mode){
	case 0://OUTPUT
	  new_info.port[i].trapcase = 0;
	  new_info.port[i].traptime = 0;
	  break;
	case 1://INPUT
	  break;
	}
      }
      flag = 2;
    }

    //Dependence solution
    switch(gpio_label){
    case 2://Value
      if(new_info.port[gpio_no].mode == 1/*INPUT*/) return SNMP_ERR_GENERR;
      break;
    case 4://TrapCase
    case 5://TrapTime
      if(new_info.port[gpio_no].mode == 0/*OUTPUT*/) return SNMP_ERR_GENERR;
      break;
    case 1://Mode
    case 3://InitValue
      break;
    default:
      return SNMP_ERR_GENERR;
    }
    break;

  case ACTION:
    if(flag == 2){
      //Update Global-data 
      memcpy(&g_info, &new_info, sizeof(gpio_info));
      flag = 3;
    }

    //Trap-timer restart
    switch(gpio_label){
    case 1://Mode
    case 4://TrapCase
    case 5://TrapTime
      if(new_info.port[gpio_no].mode == 1/*INPUT*/){
	trap_timer_restart(gpio_no);
      }else{
	trap_timer_stop(gpio_no);
      }
      break;
    case 2://Value
    case 3://InitValue
      break;
    default:
      return SNMP_ERR_GENERR;
    }
    break;

  case COMMIT:
    //Update Conf-file & GPIO Configuration
    if(flag == 3){
      flag = 0;

      //GPIO Configuration
      ret = set_port_info(&g_info);
      if(ret == -1) return SNMP_ERR_GENERR;

      //Update Conf-file
      save_config();
    }
    break;

  case FREE:
  case UNDO:
    flag = 0;
    break;

  default:
    break;
  }
  return SNMP_ERR_NOERROR;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static void load_gpio_setting(void){
  struct gpio_param param;
  int ret;
  int i;
  ret = load_config();
  if(ret == -1){
    memset(&g_info, 0, sizeof(gpio_info));
    g_info.check_cycle = 100;
    for(i=0;i<8;i++){
      memset(&param, 0, sizeof(struct gpio_param));
      param.no = BIT(i);

      get_port_info(&param);
      g_info.port[i].mode = param.mode;
      switch(param.mode){
      case MODE_OUTPUT:
	g_info.port[i].value = param.data.o.value;
	break;
      case MODE_INPUT:
	g_info.port[i].value = param.data.i.value;
	break;
      }
    }
  }else{
    for(i=0;i<8;i++){
      switch(g_info.port[i].mode){
      case 0://OUTPUT
	g_info.port[i].value = g_info.port[i].initvalue;
	break;
      case 1://INPUT
	if(g_info.port[i].traptime != 0){
	  trap_timer_start(i);
	}
	break;
      }
    }
    set_port_info(&g_info);
  }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void init_a210_gpio(void){
	int ret;

	REGISTER_MIB(
				MODULE_NAME,
				a210_gpio_variables,
				variable4,
				a210_gpio_variables_oid
				);

	ret = system_check_executable("flatfsd");
	if(ret == 0){
		gpio_config_file = GPIO_CONFIG_FILE_WITH_FLATFSD;
	}else{
		gpio_config_file = GPIO_CONFIG_FILE_DEFAULT;
	}

	load_gpio_setting();
}
/******************************************************************
 * hander when access by oid
 ******************************************************************/
unsigned char* var_a210_gpio(
	struct variable *vp,
	oid * name,
	size_t * length,
	int exact,
	size_t * var_len,
	WriteMethod ** write_method
)
{
	int ret;
	int port_no;
	struct gpio_param param;

	DEBUGMSGTL((MODULE_NAME, "var_a210_gpio()\n"));

        if(header_generic(vp, name, length, exact, var_len, write_method)
                == MATCH_FAILED){
                return NULL;
        }

	DEBUGMSGTL((MODULE_NAME, "magic : %d\n", vp->magic));

	switch(vp->magic){
	case A210_GPIO_MANAGER:
	  *write_method = write_manager_func;
	  return (u_char *)&g_info.manager;

	case A210_GPIO_CHECKCYCLE:
	  *write_method = write_cycle_func;
	  return (u_char *)&g_info.check_cycle;

	case A210_GPIO0_MODE:
	case A210_GPIO1_MODE:
	case A210_GPIO2_MODE:
	case A210_GPIO3_MODE:
	case A210_GPIO4_MODE:
	case A210_GPIO5_MODE:
	case A210_GPIO6_MODE:
	case A210_GPIO7_MODE:
	  *write_method = write_gpio_func;
	  port_no = name[*length-3]-1;
	  memset(&param, 0, sizeof(struct gpio_param));
	  param.no = BIT(port_no);
	  
	  ret = get_port_info(&param);
	  if(ret != 0){
	    //return SNMP_ERR_GENERR;
	    return NULL;
	  }else{
	    g_info.port[port_no].mode = param.mode;
	  }
	  return (u_char *)&g_info.port[port_no].mode;

	case A210_GPIO0_VALUE:
	case A210_GPIO1_VALUE:
	case A210_GPIO2_VALUE:
	case A210_GPIO3_VALUE:
	case A210_GPIO4_VALUE:
	case A210_GPIO5_VALUE:
	case A210_GPIO6_VALUE:
	case A210_GPIO7_VALUE:
	  *write_method = write_gpio_func;
	  port_no = name[*length-3]-1;
	  memset(&param, 0, sizeof(struct gpio_param));
	  param.no = BIT(port_no);
	  
	  ret = get_port_info(&param);
	  if(ret != 0){
	    //return SNMP_ERR_GENERR;
	    return NULL;
	  }else{
	    switch(param.mode){
	    case MODE_OUTPUT:
	      g_info.port[port_no].value = param.data.o.value;
	      break;
	    case MODE_INPUT:
	      g_info.port[port_no].value = param.data.i.value;
	      break;
	    default:
	      //return SNMP_ERR_GENERR;
	      return NULL;
	    }
	  }
	  return (u_char *)&g_info.port[port_no].value;

	case A210_GPIO0_INITVALUE:
	case A210_GPIO1_INITVALUE:
	case A210_GPIO2_INITVALUE:
	case A210_GPIO3_INITVALUE:
	case A210_GPIO4_INITVALUE:
	case A210_GPIO5_INITVALUE:
	case A210_GPIO6_INITVALUE:
	case A210_GPIO7_INITVALUE:
	  *write_method = write_gpio_func;
	  port_no = name[*length-3]-1;
	  return (u_char *)&g_info.port[port_no].initvalue;

	case A210_GPIO0_TRAPCASE:
	case A210_GPIO1_TRAPCASE:
	case A210_GPIO2_TRAPCASE:
	case A210_GPIO3_TRAPCASE:
	case A210_GPIO4_TRAPCASE:
	case A210_GPIO5_TRAPCASE:
	case A210_GPIO6_TRAPCASE:
	case A210_GPIO7_TRAPCASE:
	  *write_method = write_gpio_func;
	  port_no = name[*length-3]-1;
	  return (u_char *)&g_info.port[port_no].trapcase;

	case A210_GPIO0_TRAPTIME:
	case A210_GPIO1_TRAPTIME:
	case A210_GPIO2_TRAPTIME:
	case A210_GPIO3_TRAPTIME:
	case A210_GPIO4_TRAPTIME:
	case A210_GPIO5_TRAPTIME:
	case A210_GPIO6_TRAPTIME:
	case A210_GPIO7_TRAPTIME:
	  *write_method = write_gpio_func;
	  port_no = name[*length-3]-1;
	  return (u_char *)&g_info.port[port_no].traptime;  

	default:
		DEBUGMSGTL((MODULE_NAME, "invlid oid%d\n", vp->magic));
	}

	return NULL;
}

