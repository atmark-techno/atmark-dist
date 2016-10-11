/************************************************************************
 file name : a210_misc.c
 summary   : misc mibs for armadillo-a210
 coded by  : M.Nakai
 copyright : Atmark Techno
************************************************************************/
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ctype.h>

#include "util_funcs.h"
#include "a210_common.h"
#include "a210_misc.h"

#define MODULE_NAME "a210_misc"

// MAGIC NUMBERS
#define A210_MISCVERSION		1
#define A210_MISCUSEDMEM		2
#define A210_MISCFREEMEM		3
#define A210_MISCBUFFMEM		4
#define A210_MISCCACHEMEM		5

oid a210_misc_variables_oid[] = { 1, 3, 6, 1, 4, 1, 16031, 1, 4, 10 };

struct variable2 a210_misc_variables[] = {
    {A210_MISCVERSION, ASN_OCTET_STR, RONLY, var_a210_misc, 1, {5}},
    {A210_MISCUSEDMEM, ASN_INTEGER, RONLY, var_a210_misc, 1, {10}},
    {A210_MISCFREEMEM, ASN_INTEGER, RONLY, var_a210_misc, 1, {15}},
    {A210_MISCBUFFMEM, ASN_INTEGER, RONLY, var_a210_misc, 1, {20}},
    {A210_MISCCACHEMEM, ASN_INTEGER, RONLY, var_a210_misc, 1, {25}},
};

#define NO_VERSION_STR	"none"
#define VERSION_STR_SIZE 64
static char versionString[VERSION_STR_SIZE];

/******************************************************************
 * open version file to get version info
 *****************************************************************/
void GetVersionInfo()
{
	FILE* fp;
	versionString[VERSION_STR_SIZE - 1] = 0;
	strncpy(versionString, NO_VERSION_STR, VERSION_STR_SIZE - 1);
	fp = fopen("/etc/IMAGEVERSION", "r");
	if(!fp){
		return;
	}

	if(fgets(versionString, VERSION_STR_SIZE - 1, fp)){
		int i;
		for(i = strlen(versionString) - 1 ; i >= 0 ; i--){
			if(isspace(versionString[i])){
				versionString[i] = 0;
			}
			else{
				break;
			}
		}
	}

	fclose(fp);
}

enum MEMORY_ID{
	ID_MEMTOTAL=0,
	ID_MEMFREE,
	ID_BUFFERS,
	ID_CACHED,
};

struct target_memory{
	int id;
	char *label;
};

struct target_memory st_mem[]={
	{ID_MEMTOTAL,"MemTotal"},
	{ID_MEMFREE,"MemFree"},
	{ID_BUFFERS,"Buffers"},
	{ID_CACHED,"Cached"},
};

/******************************************************************
 * open proc/meminfo file to get used memory size
 *****************************************************************/
unsigned int GetCurrentMemory(int id){
	FILE *fp;
	char buf[256];

	fp = fopen("/proc/meminfo", "r");
	if(!fp) return -1;

	while(fgets(buf, sizeof(buf), fp)){
		char label[64];
		unsigned int size;
		sscanf(buf, "%s %d", label, &size);

		if(st_mem[id].id == id){
			if(memcmp(label,
				  st_mem[id].label,
				  strlen(st_mem[id].label)) == 0){
				fclose(fp);
				return size;
			}
		}
	}
	fclose(fp);
	return -1;
}

/******************************************************************
 * misc init
 *****************************************************************/
void init_a210_misc(void)
{
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));
	REGISTER_MIB(
				MODULE_NAME,
				a210_misc_variables,
				variable2,
				a210_misc_variables_oid
				);
	GetVersionInfo();
}

/******************************************************************
 * hander when access by oid
 *****************************************************************/
unsigned char* var_a210_misc(
	struct variable *vp,
	oid * name,
	size_t * length,
	int exact,
	size_t * var_len,
	WriteMethod ** write_method
)
{
	unsigned long total, used, free, buffer, cache;
	static unsigned long long_ret;
	long_ret = 0;

	DEBUGMSGTL((MODULE_NAME, "var_a210_misc()\n"));
	if(header_generic(vp, name, length, exact, var_len, write_method)
		== MATCH_FAILED){
		return NULL;
	}

	DEBUGMSGTL((MODULE_NAME, "magic : %d\n", vp->magic));

	switch(vp->magic){

	case A210_MISCVERSION:
		*var_len = strlen(versionString);
		return (u_char *)versionString;

	case A210_MISCUSEDMEM:
		total = GetCurrentMemory(ID_MEMTOTAL);
		free  = GetCurrentMemory(ID_MEMFREE);
		if(total == -1 || free == -1) return NULL;

		used = total-free;
		if(total < free) used = 0;

		long_ret = used;
		return (u_char *)&long_ret;

	case A210_MISCFREEMEM:
		free = GetCurrentMemory(ID_MEMFREE);
		if(free == -1) return NULL;

		long_ret = free;
		return (u_char *)&long_ret;

	case A210_MISCBUFFMEM:
		buffer = GetCurrentMemory(ID_BUFFERS);
		if(buffer == -1) return NULL;

		long_ret = buffer;
		return (u_char *)&long_ret;

	case A210_MISCCACHEMEM:
		cache = GetCurrentMemory(ID_CACHED);
		if(cache == -1) return NULL;

		long_ret = cache;
		return (u_char *)&long_ret;

	default:
		DEBUGMSGTL((MODULE_NAME, "invlid oid%d\n", vp->magic));
		break;
	}
	return NULL;
}

