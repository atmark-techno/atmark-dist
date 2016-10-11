/************************************************************************
 file name : aj_misc.c
 summary   : misc mibs for armadillo-j
 coded by  : F.Morishima
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

#include "util_funcs.h"
#include "aj_misc.h"

// MAGIC NUMBERS
#define AJMISCVERSION		1
#define AJMISCUSEDMEM		2
#define AJMISCFREEMEM		3
#define AJMISCBUFFMEM		4
#define AJMISCCACHEMEM		5

oid aj_misc_variables_oid[] = { 1, 3, 6, 1, 4, 1, 16031, 1, 2, 10 };

struct variable2 aj_misc_variables[] = {
    {AJMISCVERSION, ASN_OCTET_STR, RONLY, var_aj_misc, 1, {5}},
    {AJMISCUSEDMEM, ASN_INTEGER, RONLY, var_aj_misc, 1, {10}},
    {AJMISCFREEMEM, ASN_INTEGER, RONLY, var_aj_misc, 1, {15}},
    {AJMISCBUFFMEM, ASN_INTEGER, RONLY, var_aj_misc, 1, {20}},
    {AJMISCCACHEMEM, ASN_INTEGER, RONLY, var_aj_misc, 1, {25}},
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
	fp = fopen("/etc/aj_version", "r");
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

/******************************************************************
 * open proc/meminfo file to get used memory size
 *****************************************************************/
unsigned int GetMemoryInfo(
	unsigned long* total,
	unsigned long* used,
	unsigned long* free,
	unsigned long* shared,
	unsigned long* buffer,
	unsigned long* cache
)
{
	FILE* fp;
	char buff[128];
	fp = fopen("/proc/meminfo", "r");
	if(!fp){
		return;
	}

	while(fgets(buff, sizeof(buff), fp)){
		char title[32];
		const char* fmt = "%s %u %u %u %u %u %u";
		if(sscanf(buff, fmt, title, total, used,
					free, shared, buffer, cache) != 7){
			continue;
		}

		if(!strcmp(title, "Mem:")){
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	return -1;
}

/******************************************************************
 * misc init
 *****************************************************************/
void init_aj_misc(void)
{
	DEBUGMSGTL(("aj_misc", "%s()\n", __FUNCTION__));
	REGISTER_MIB(
				"aj_misc",
				aj_misc_variables,
				variable2,
				aj_misc_variables_oid
				);
	GetVersionInfo();
}

/******************************************************************
 * hander when access by oid
 *****************************************************************/
unsigned char* var_aj_misc(
	struct variable *vp,
	oid * name,
	size_t * length,
	int exact,
	size_t * var_len,
	WriteMethod ** write_method
)
{
	unsigned long total, used, free, shared, buffer, cache;
	static unsigned long long_ret;
	long_ret = 0;

	DEBUGMSGTL(("aj_misc", "var_aj_misc()\n"));
	if(header_generic(vp, name, length, exact, var_len, write_method)
		== MATCH_FAILED){
		return NULL;
	}

	DEBUGMSGTL(("aj_misc", "magic : %d\n", vp->magic));

	switch(vp->magic){

	case AJMISCVERSION:
		*var_len = strlen(versionString);
		return (u_char *)versionString;

	case AJMISCUSEDMEM:
		if(!GetMemoryInfo(&total, &used, &free, &shared, &buffer, &cache)){
			long_ret = used;
		}
		return (u_char *)&long_ret;

	case AJMISCFREEMEM:
		if(!GetMemoryInfo(&total, &used, &free, &shared, &buffer, &cache)){
			long_ret = free;
		}
		return (u_char *)&long_ret;

	case AJMISCBUFFMEM:
		if(!GetMemoryInfo(&total, &used, &free, &shared, &buffer, &cache)){
			long_ret = buffer;
		}
		return (u_char *)&long_ret;

	case AJMISCCACHEMEM:
		if(!GetMemoryInfo(&total, &used, &free, &shared, &buffer, &cache)){
			long_ret = cache;
		}
		return (u_char *)&long_ret;

	default:
		DEBUGMSGTL(("aj_misc", "invlid oid%d\n", vp->magic));
		break;
	}
	return NULL;
}

