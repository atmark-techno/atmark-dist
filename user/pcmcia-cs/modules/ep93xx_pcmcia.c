/*======================================================================
	
	ep93xx_pcmcia.c
	
	Device driver for PCMCIA on the Cirrus Logic EP93XX processor.

	The contents of this file are subject to the Mozilla Public
	License Version 1.1 (the "License"); you may not use this file
	except in compliance with the License. You may obtain a copy of
	the License at http://www.mozilla.org/MPL/

	Software distributed under the License is distributed on an "AS
	IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
	implied. See the License for the specific language governing
	rights and limitations under the License.

	Alternatively, the contents of this file may be used under the
	terms of the GNU Public License version 2 (the "GPL"), in which
	case the provisions of the GPL are applicable instead of the
	above.  If you wish to allow the use of your version of this file
	only under the terms of the GPL and not to allow others to use
	your version of this file under the MPL, indicate your decision
	by deleting the provisions above and replace them with the notice
	and other provisions required by the GPL.  If you do not delete
	the provisions above, a recipient may use your version of this
	file under either the MPL or the GPL.
	
======================================================================*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/string.h>

#include <asm/io.h>
#include <asm/system.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/ss.h>
#include <pcmcia/bus_ops.h>

#include <asm/hardware.h>
#include <asm/irq.h>

#include "ep93xx_pcmcia.h"

#undef PCMCIA_DEBUG
#undef DEBUG

#if 0
#define DEBUG(n, args...) printk(args);
#else
#define DEBUG(n, args...)

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) printk(KERN_DEBUG "ep93xx_pcmcia: " args);
#else
#define DEBUG(n, args...)
#endif
#endif

#define PCMCIA_INFO(args...) printk(KERN_INFO "ep93xx_pcmcia: "args)
#define PCMCIA_ERROR(args...) printk(KERN_ERR "ep93xx_pcmcia: "args)

MODULE_AUTHOR("David Hinds <dahinds@users.sourceforge.net>");
MODULE_DESCRIPTION("Cirrus Logic EP93xx pcmcia socket driver");
MODULE_LICENSE("Dual MPL/GPL");

/* Maximum number of IO windows per socket */
#define EP93XX_MAX_IO_WIN 2

/* Maximum number of memory windows per socket */
#define EP93XX_MAX_MEM_WIN 5

#define EP93XX_UNCONFIGURED_VOLTAGE 0xFF

struct pcmcia_irq_info {
	unsigned int sock;
	unsigned int irq;
};

static struct irqs {
	int irq;
	unsigned int gpio;
	const char *str;
} ep93xx_pcmcia_irqs[] =
{
	// Interrupt number       GPIO port F pin mask      String
	{ EP93XX_PCMCIA_INT_RDY,   EP93XX_PCMCIA_RDY,  "EP93XX PCMCIA RDY"  },
	{ EP93XX_PCMCIA_INT_CD1,   EP93XX_PCMCIA_CD1,  "EP93XX PCMCIA CD1"  },
	{ EP93XX_PCMCIA_INT_CD2,   EP93XX_PCMCIA_CD2,  "EP93XX PCMCIA CD2"  },
	{ EP93XX_PCMCIA_INT_BVD2,  EP93XX_PCMCIA_BVD2, "EP93XX PCMCIA BVD2" },
	{ EP93XX_PCMCIA_INT_BVD1,  EP93XX_PCMCIA_BVD1, "EP93XX PCMCIA BVD1" },
};

#define EP93XX_GPIOS        \
	( EP93XX_PCMCIA_RDY |   \
	  EP93XX_PCMCIA_CD1 |   \
	  EP93XX_PCMCIA_CD2 |   \
	  EP93XX_PCMCIA_BVD2 |  \
	  EP93XX_PCMCIA_BVD1 )


/* This structure maintains housekeeping state for each socket, such
 * as the last known values of the card detect pins, or the Card Services
 * callback value associated with the socket:
 */
struct socket_info_t {
	socket_state_t      state;
	struct pccard_mem_map mem_map[EP93XX_MAX_MEM_WIN];
	struct pccard_io_map  io_map[EP93XX_MAX_IO_WIN];

	// The CS event handler:
	void                (*cs_event_handler)(void *info, u_int events);
	void                *cs_handler_info;

	// IRQ handler for card driver
	void                (*card_irq_handler)(int, void *, struct pt_regs *);
	void                *dev_id;

	void                *virt_io;
	unsigned short      speed_io, speed_attr, speed_mem;
	unsigned int        uiStatus;
	unsigned long       IrqsToEnable;
	u_char              Vcc;
	struct bus_operations bus_ops;
};

static struct socket_info_t socket_info[EP93XX_MAX_SOCK];

static void ep93xx_pcmcia_poll_event(unsigned long dummy);
static int ep93xx_set_voltage( u_short sock, u_char NewVcc );
static unsigned long ep93xx_calculate_timing( unsigned long ulNsSpeed );
static int ep93xx_shutdown(void);
static int __init ep93xx_init(void);
static void __exit ep93xx_exit(void);
static void ep93xx_interrupt(int irq, void *dev, struct pt_regs *regs);
static int ep93xx_register_callback(u_short sock, ss_callback_t *call);
static int ep93xx_get_status( u_short sock, u_int *uiStatus );
static int ep93xx_inquire_socket(u_short sock, socket_cap_t *cap);
static int ep93xx_get_socket(u_short sock, socket_state_t *state);
static int ep93xx_set_socket( u_short sock, socket_state_t *state );
static int ep93xx_get_io_map(u_short sock, struct pccard_io_map *io_map);
static int ep93xx_set_io_map(u_short sock, struct pccard_io_map *map);
static int ep93xx_get_mem_map(u_short sock, struct pccard_mem_map *mem_map);
static int ep93xx_set_mem_map(u_short sock, struct pccard_mem_map *map);
static int ep93xx_service(u_int sock, u_int cmd, void *arg);


static u32 ep93xx_bus_in(void *bus, u32 port, s32 sz){
  DEBUG(5, "%s(%08x, %d)\n", __FUNCTION__, port, sz);

  switch(sz){
  case  0: return inb(port);
  case  1: return le16_to_cpu(inw(port));
  case -1: return inw(port);
  case  2: return le32_to_cpu(inl(port));
  case -2: return inl(port);
  }

  printk(KERN_ERR "%s(): invalid size %d\n", __FUNCTION__, sz);
  return 0;
}

static void ep93xx_bus_out(void *bus, u32 val, u32 port, s32 sz){
  DEBUG(5, "%s(%08x, %08x, %d)\n", __FUNCTION__, val, port, sz);

  switch(sz){
  case  0: outb(val, port);               break;
  case  1: outw(cpu_to_le16(val), port);  break;
  case -1: outw(val, port);               break;
  case  2: outl(cpu_to_le32(val), port);  break;
  case -2: outl(val, port);               break;
  default: printk(KERN_ERR "%s(): invalid size %d\n", __FUNCTION__, sz);
  }
}

static void ep93xx_bus_ins(void *bus, u32 port, void *buf, 
			   u32 count, s32 sz){
  DEBUG(5, "%s(%08x, %p, %u, %d)\n", __FUNCTION__, port, buf, count, sz);

  switch(sz){
  case 0:
	while((sz--)>0)
	  *(((unsigned char *)buf)++)=inb(port);
	break;

  case 1:
	while((sz--)>0)
	  *(((unsigned short *)buf)++)=le16_to_cpu(inw(port));
	break;

  case -1:
	while((sz--)>0)
	  *(((unsigned short *)buf)++)=inw(port);
	break;

  case 2:
	while((sz--)>0)
	  *(((unsigned int *)buf)++)=le32_to_cpu(inl(port));
	break;

  case -2:
	while((sz--)>0)
	  *(((unsigned int *)buf)++)=inl(port);
	break;

  default: printk(KERN_ERR "%s(): invalid size %d\n", __FUNCTION__, sz);
  }
}

static void ep93xx_bus_outs(void *bus, u32 port, void *buf,
				u32 count, s32 sz){
  DEBUG(5, "%s(%08x, %p, %u, %d)\n", __FUNCTION__, port, buf, count, sz);

  switch(sz){
  case 0:
	while((sz--)>0)
	  outb(*(((unsigned char *)buf)++), port);
	break;
	
  case 1:
	while((sz--)>0)
	  outw(cpu_to_le16(*(((unsigned short *)buf)++)), port);
	break;

  case -1:
	while((sz--)>0)
	  outw(*(((unsigned short *)buf)++), port);
	break;

  case 2:
	while((sz--)>0)
	  outl(cpu_to_le32(*(((unsigned int *)buf)++)), port);
	break;

  case -2:
	while((sz--)>0)
	  outl(*(((unsigned int *)buf)++), port);
	break;

  default: printk(KERN_ERR "%s(): invalid size %d\n", __FUNCTION__, sz);
  }
}

static void *ep93xx_bus_ioremap(void *bus, u_long ofs, u_long sz){
  DEBUG(5, "%s(%08lx, %lu)\n", __FUNCTION__, ofs, sz);
//  return (void *)ofs;
  return ioremap( ofs, sz );
}

static void ep93xx_bus_iounmap(void *bus, void *addr){
  DEBUG(5, "%s(%p)\n", __FUNCTION__, addr);
  iounmap( addr );
}

static u32 ep93xx_bus_read(void *bus, void *addr, s32 sz){
  DEBUG(5, "%s(%p, %d)\n", __FUNCTION__, addr, sz);
  
  switch(sz){
//  case  0: return *((unsigned char *)addr);
  case  0: 
  { 
	unsigned char ucTemp = *((unsigned char *)addr); 
	DEBUG(5, "%s(%p, %d): 0x%02x\n", __FUNCTION__, addr, sz, ucTemp);
	return ucTemp;
  }
  case  1: return le16_to_cpu(*((unsigned short *)addr));
  case -1: return *((unsigned short *)addr);
  case  2: return le32_to_cpu(*((unsigned int *)addr));
  case -2: return *((unsigned int *)addr);
  }

  printk(KERN_ERR "%s(): invalid size %d\n", __FUNCTION__, sz);
  return 0;
}

static void ep93xx_bus_write(void *bus, u32 val, void *addr, s32 sz){
	DEBUG(5, "%s(%x, %p, %d)\n", __FUNCTION__, val, addr, sz);

	switch(sz)
	{
		case  0: *((unsigned char *)addr)=(unsigned char)val;                break;
		case  1: *((unsigned short *)addr)=cpu_to_le16((unsigned short)val); break;
		case -1: *((unsigned short *)addr)=(unsigned short)val;              break;
		case  2: *((unsigned int *)addr)=cpu_to_le32((unsigned int)val);     break;
		case -2: *((unsigned int *)addr)=(unsigned int)val;                  break;
		default: printk(KERN_ERR "%s(): invalid size %d\n", __FUNCTION__, sz);
	}
}

static void ep93xx_bus_copy_from(void *bus, void *d, void *s, u32 count){
	DEBUG(5, "%s(%p, %p, %u)\n", __FUNCTION__, d, s, count);
	_memcpy_fromio(d, (unsigned long)s, count);
}

static void ep93xx_bus_copy_to(void *bus, void *d, void *s, u32 count){
	DEBUG(5, "%s(%p, %p, %u)\n", __FUNCTION__, d, s, count);
	_memcpy_toio((unsigned long)d, s, count);
}

static int 
ep93xx_bus_request_irq
( 
	void *bus, 
	u_int irq, 
	void (*handler)(int, void *, struct pt_regs *), 
	u_long flags, 
	const char *device, 
	void *dev_id 
)
{
	if (irq!=EP93XX_PCMCIA_INT_RDY)
		return -1;

	DEBUG(5, "%s(%u, %p, %08lx, \"%s\", %p)\n", __FUNCTION__, irq, handler,
	flags, device, dev_id);

	socket_info[0].dev_id = dev_id;
	socket_info[0].card_irq_handler = handler;
	
	return 0;
}

static void 
ep93xx_bus_free_irq(void *bus, u_int irq, void *dev_id)
{
	DEBUG(5, "%s(%u, %p)\n", __FUNCTION__, irq, dev_id);
	
	if (irq!=EP93XX_PCMCIA_INT_RDY)
		return;
		
	socket_info[0].card_irq_handler = NULL;
}


static void ep93xx_timer(void *data)
{
	ep93xx_interrupt( 0, NULL, NULL );
}

static struct timer_list poll_timer;
static struct tq_struct ep93xx_timer_task = {
	routine: ep93xx_timer
};

#define EP9315_PCMCIA_POLL_PERIOD    (2*HZ)

static void ep93xx_pcmcia_poll_event(unsigned long dummy)
{
  //DEBUG(4, "%s(): polling for events\n", __FUNCTION__);
  poll_timer.function = ep93xx_pcmcia_poll_event;
  poll_timer.expires = jiffies + EP9315_PCMCIA_POLL_PERIOD;
  add_timer(&poll_timer);
  schedule_task(&ep93xx_timer_task);
}

#ifndef CONFIG_ARCH_ARMADILLO9
/*
 * We bit-bang the pcmcia power controller using this function.
 */
static void ep93xx_bitbang
( 
	unsigned long ulNewEEValue 
)
{
	unsigned long ulGdata;

	ulGdata = inl( GPIO_PGDR );

	ulGdata &= ~(GPIOA_EECLK | GPIOA_EEDAT | GPIOA_SLA0);

	ulNewEEValue &= (GPIOA_EECLK | GPIOA_EEDAT | GPIOA_SLA0);

	ulGdata |= ulNewEEValue;

	outl( ulGdata, GPIO_PGDR );
	ulGdata = inl( GPIO_PGDR ); // read to push write out wrapper
	
	// Voltage controller's data sheet says minimum pulse width is 
	// one microsecond.
	udelay(5);
}
#endif

static int
ep93xx_set_voltage( u_short sock, u_char NewVcc )
{
	struct socket_info_t * skt = &socket_info[sock];
	unsigned long ulSwitchSettings;
#ifndef CONFIG_ARCH_ARMADILLO9
	unsigned long ulDataBit, ulGdirection;
	int       i;
#endif
	
	if (sock >= EP93XX_MAX_SOCK)
		return -EINVAL;

	if( skt->Vcc == NewVcc ){
		DEBUG(3, "Power already set to %d\n", NewVcc );
		return 0;
	}
	
	ulSwitchSettings = EE_ADDRESS | ENABLE;
	switch( NewVcc ) 
	{
		case 0:
			DEBUG(3, "Configure the socket for 0 Volts\n");
			ulSwitchSettings |= AVCC_0V;
			break;

		case 50:
#ifdef CONFIG_ARCH_ARMADILLO9
			printk(KERN_ERR "%s(): unsupported Vcc 5 Volts\n",
			       __FUNCTION__);
			return -1;
#else
			ulSwitchSettings |= AVCC_5V;
			DEBUG(3, "Configure the socket for 5 Volts\n");
			break;
#endif

		case 33:
			DEBUG(3, "Configure the socket for 3.3 Volts\n");
			ulSwitchSettings |= AVCC_33V;
			break;

		default:
			printk(KERN_ERR "%s(): unrecognized Vcc %u\n",
			       __FUNCTION__, NewVcc);
			return -1;
	}

#ifdef CONFIG_ARCH_ARMADILLO9
	outl( (inl(GPIO_PADR) & ~0x8) |
	      ((ulSwitchSettings & AVCC_33V) ? 0x0 : 0x8), GPIO_PADR );	
	mdelay(300);
#else
	//
	// Configure the proper GPIO pins as outputs.
	//
	ep93xx_bitbang( GPIOA_EECLK | GPIOA_EEDAT );
	
	//
	// Read modify write the data direction register, set the
	// proper lines to be outputs.
	//
	ulGdirection = inl( GPIO_PGDDR );
	ulGdirection |= GPIOA_EECLK | GPIOA_EEDAT | GPIOA_SLA0;
	outl( ulGdirection, GPIO_PGDDR );
	ulGdirection = inl( GPIO_PGDDR ); // read to push write out wrapper
	
	//
	// Clear all except EECLK
	// Lower the clock.
	// 
	ep93xx_bitbang( GPIOA_EECLK );
	ep93xx_bitbang( 0 );

	//
	// Serial shift the command word out to the voltage controller.
	//
	for( i=18 ; i>=0 ; --i )
	{
		if( (ulSwitchSettings >> i) & 0x1 )
			ulDataBit = GPIOA_EEDAT;
		else
			ulDataBit = 0;
		
		//
		// Put the data on the bus and lower the clock.
		// Raise the clock to latch the data in.
		// Lower the clock again.
		//
		ep93xx_bitbang( ulDataBit );
		ep93xx_bitbang( ulDataBit | GPIOA_EECLK );
		ep93xx_bitbang( ulDataBit );
	}
		
	//
	// Raise and lower the Latch.
	// Raise EECLK, delay, raise EEDAT, leave them that way.
	//
	ep93xx_bitbang( GPIOA_SLA0 );
	ep93xx_bitbang( 0 );
	ep93xx_bitbang( GPIOA_EECLK );
	ep93xx_bitbang( GPIOA_EECLK | GPIOA_EEDAT );
#endif

	skt->Vcc = NewVcc;
	
	DEBUG(3, "ep93xx_set_voltage - exit\n");

	return 0;
}


#define PCMCIA_BOARD_DELAY          40

//****************************************************************************
// CalculatePcmciaTimings
//****************************************************************************
// Calculate the pcmcia timings based on the register settings. 
// For example here is for Attribute/Memory Read.
//
//
// Address:   _______XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX___________
//
// Data:      __________________________XXXXXXXXXXXXXX________________________
//
// CE#:       -------______________________________________________-----------
//
// REG#:      -------______________________________________________-----------
//
// OE#:       --------------------___________________-------------------------
//
//
//                   |<---------------Cycle time ----------------->|
//
//                   |< Address >|<-- Access Time -->|< Hold Time >|
//                      Time           ta(CE)             th(CE)
//                      tsu(A)
//
//  See PCMCIA Electrical Specification Section 4.7 for the timing numbers.
//
//
static unsigned long ep93xx_calculate_timing( unsigned long ulNsSpeed )
{
	unsigned long ulAddressTime, ulHoldTime, ulAccessTime, ulHPeriod, ulSMC;
	unsigned long ulHAccessTime, ulHAddressTime, ulHHoldTime, ulHCLK = 50000000;
	
	switch( ulNsSpeed )
	{
		case 600:
		default:
			ulAccessTime = 600; ulAddressTime = 100; ulHoldTime = 35;
			break;
			
		case 300:
			ulAccessTime = 300; ulAddressTime = 30;	 ulHoldTime = 20;
			break;
			
		case 250:
			ulAccessTime = 250; ulAddressTime = 30;	 ulHoldTime = 20;
			break;
			
		case 200:
			ulAccessTime = 200; ulAddressTime = 20; ulHoldTime = 20;
			break;
			
		case 150:
			ulAccessTime = 150; ulAddressTime = 20;	ulHoldTime = 20;
			break;
			
		case 100:
			ulAccessTime = 100;	ulAddressTime = 10;	ulHoldTime = 15;
			break;
			
		// Special case for I/O all access.
		case 0:
			ulAccessTime = 165; ulAddressTime = 70; ulHoldTime = 20;
			break;
	}

	//
	// Add in a board delay.
	//
	ulAccessTime    += PCMCIA_BOARD_DELAY;
	ulAddressTime   += PCMCIA_BOARD_DELAY;
	ulHoldTime      += PCMCIA_BOARD_DELAY;
	
	//
	// This gives us the period in nanosecods.
	//
	// = 1000000000 (ns/s) / HCLK (cycle/s)
	//
	// = (ns/cycle)
	//
	ulHPeriod       = (1000000000/ ulHCLK);
	
	//
	// Find the number of hclk cycles for cycle time, address time and
	// hold time.
	//
	// = ulAccessTime  (ns) / ulHPeriod (ns/Cycles)
	// = ulAddressTime (ns) / ulHPeriod (ns/Cycles)
	// = ulHoldTime    (ns) / ulHPeriod (ns/Cycles)
	//
	ulHAccessTime    = ulAccessTime / ulHPeriod;
	if(ulHAccessTime > 0xFF)
		ulHAccessTime  = 0xFF;
	
	ulHAddressTime  = ulAddressTime / ulHPeriod;
	if(ulHAddressTime > 0xFF)
		ulHAddressTime = 0xFF;
			
	ulHHoldTime     = (ulHoldTime /ulHPeriod) + 1;
	if(ulHHoldTime >0xF)
		ulHHoldTime     = 0xF;

	ulSMC = (PCCONFIG_ADDRESSTIME_MASK & (ulHAddressTime << PCCONFIG_ADDRESSTIME_SHIFT)) |
			(PCCONFIG_HOLDTIME_MASK & (ulHHoldTime << PCCONFIG_HOLDTIME_SHIFT)) |
			(PCCONFIG_ACCESSTIME_MASK & (ulHAccessTime << PCCONFIG_ACCESSTIME_SHIFT)) ;

	//DEBUG(3, "pcmciatiming: %d uSec. SMC reg value 0x%08x\n",
	//		(unsigned int)ulNsSpeed, (unsigned int)ulSMC );
	
	return ulSMC;    
}

/* ------------------------------------------------------------------------- */

static int ep93xx_shutdown(void)
{
	unsigned long ulTemp;
	int i;

	DEBUG(3, "ep93xx_shutdown - enter\n");

	del_timer_sync(&poll_timer);

	//
	// Disable, clear pcmcia irqs in hw.
	//
	ulTemp = inl( GPIO_FINTEN );
	outl( ulTemp & ~EP93XX_GPIOS, GPIO_FINTEN );
	ulTemp = inl( GPIO_FINTEN );

	outl( EP93XX_GPIOS, GPIO_FEOI );
	ulTemp = inl( GPIO_FEOI );

	//
	// Set reset.
	//
	outl( (PCCONT_WEN | PCCONT_PC1RST | PCCONT_PC1EN), SMC_PCMCIACtrl );
	ulTemp = inl( SMC_PCMCIACtrl );

	//
	// Release memory
	// Free the pcmcia interrupts.
	// Set socket voltage to zero
	//
	iounmap(socket_info[0].virt_io);
	socket_info[0].Vcc = EP93XX_UNCONFIGURED_VOLTAGE;
	ep93xx_set_voltage( 0, 0 );

	for( i = 0; i < ARRAY_SIZE(ep93xx_pcmcia_irqs) ; i++ ) 
		free_irq( ep93xx_pcmcia_irqs[i].irq, socket_info );

	return 0;
}

/* ------------------------------------------------------------------------- */

static int __init ep93xx_init(void)
{
	servinfo_t info;
	unsigned long ulRegValue;
	int ret, i;
	unsigned long ulTemp, flags;

	DEBUG(3, "ep93xx_init - enter\n");

	printk(KERN_INFO "EP93XX PCMCIA (CS release %s)\n", CS_RELEASE);
	
	/*
	 * Initialize our socket info structure.
	 */
	struct socket_info_t * skt = &socket_info[0];
	
	for( i = 0 ; i < EP93XX_MAX_IO_WIN ; i++ )
	{
		skt->io_map[i].flags = 0;
		skt->io_map[i].speed = EP93XX_PCMCIA_IO_ACCESS;
	}

	for( i = 0 ; i < EP93XX_MAX_MEM_WIN ; i++ )
	{
		skt->mem_map[i].flags = 0;
		skt->mem_map[i].speed = EP93XX_PCMCIA_3V_MEM_ACCESS;
	}

	skt->card_irq_handler = NULL;
	skt->cs_event_handler = NULL;
	skt->cs_handler_info = NULL;
	skt->dev_id = NULL;
	
	skt->speed_io   = EP93XX_PCMCIA_IO_ACCESS;
	skt->speed_attr = EP93XX_PCMCIA_5V_MEM_ACCESS;
	skt->speed_mem  = EP93XX_PCMCIA_5V_MEM_ACCESS;

	skt->uiStatus = 0;
	skt->IrqsToEnable = 0;
	
	skt->Vcc = EP93XX_UNCONFIGURED_VOLTAGE;
	ep93xx_set_voltage(0,0);

	skt->virt_io    = ioremap( EP93XX_PHYS_ADDR_PCMCIAIO, 0x10000 );
	DEBUG(3, "virt_io is 0x%08x\n", (unsigned int)skt->virt_io );
	if (skt->virt_io == NULL) {
		ep93xx_shutdown();
		return -ENOMEM;
	}

	skt->bus_ops.priv = 0; /* socket number = 0 */
	skt->bus_ops.b_in          = ep93xx_bus_in;
	skt->bus_ops.b_ins         = ep93xx_bus_ins;
	skt->bus_ops.b_out         = ep93xx_bus_out;
	skt->bus_ops.b_outs        = ep93xx_bus_outs;
	skt->bus_ops.b_ioremap     = ep93xx_bus_ioremap;
	skt->bus_ops.b_iounmap     = ep93xx_bus_iounmap;
	skt->bus_ops.b_read        = ep93xx_bus_read;
	skt->bus_ops.b_write       = ep93xx_bus_write;
	skt->bus_ops.b_copy_from   = ep93xx_bus_copy_from;
	skt->bus_ops.b_copy_to     = ep93xx_bus_copy_to;
	skt->bus_ops.b_request_irq = ep93xx_bus_request_irq;
	skt->bus_ops.b_free_irq    = ep93xx_bus_free_irq;


	CardServices(GetCardServicesInfo, &info);
	if (info.Revision != CS_RELEASE_CODE){
		printk(KERN_ERR "Card Services release codes do not match\n");
		return -EINVAL;
	}

	//
	// Disable interrupts in hw for pcmcia.
	//
	ulTemp = inl( GPIO_FINTEN );
	outl( (ulTemp & ~EP93XX_GPIOS), GPIO_FINTEN );
	ulTemp = inl( GPIO_FINTEN );
	
	//
	// Set data direction to input for pcmcia socket lines.
	//
	ulTemp = inl( GPIO_PFDDR );
	outl( (ulTemp & ~EP93XX_GPIOS), GPIO_PFDDR );
	ulTemp = inl( GPIO_PFDDR );

	//
	// Enable debounce for the card detect lines
	// Set interrupts to be edge sensitive, falling edge triggered.
	//
	ulTemp = inl( GPIO_FDB ) & ~EP93XX_GPIOS;
	outl( (EP93XX_PCMCIA_CD1 | EP93XX_PCMCIA_CD2) | ulTemp, GPIO_FDB );

	ulTemp = inl( GPIO_FINTTYPE1 ) & ~EP93XX_GPIOS;
	outl( (EP93XX_PCMCIA_CD1 | EP93XX_PCMCIA_CD2 |
	       EP93XX_PCMCIA_BVD2 | EP93XX_PCMCIA_BVD1) | ulTemp, GPIO_FINTTYPE1 );

	ulTemp = inl( GPIO_FINTTYPE2 ) & ~EP93XX_GPIOS;
	outl( ulTemp, GPIO_FINTTYPE2 );
	ulTemp = inl( GPIO_FINTTYPE2 );

	//
	// Clear all interrupts for GPIO port F.
	//
	outl( EP93XX_GPIOS, GPIO_FEOI );
	ulTemp = inl( GPIO_FEOI );

	//
	// Register ISR.  EP93XX_PCMCIA_INT_RDY is a shared interrupt as
	// the kernel IDE stack has its own interrupt handler that it
	// will register for it.
	//
	for( i = 0; i < ARRAY_SIZE(ep93xx_pcmcia_irqs) ; i++ )
	{
		if ( ep93xx_pcmcia_irqs[i].irq == EP93XX_PCMCIA_INT_RDY )
			flags = SA_INTERRUPT | SA_SHIRQ;
		else
			flags = SA_INTERRUPT;
		
		if( request_irq( ep93xx_pcmcia_irqs[i].irq, ep93xx_interrupt, 
				flags, ep93xx_pcmcia_irqs[i].str, socket_info ) ) 
		{
			printk( KERN_ERR "%s: request for IRQ%d failed\n",
				   __FUNCTION__, ep93xx_pcmcia_irqs[i].irq );

			while (i--)
				free_irq( ep93xx_pcmcia_irqs[i].irq, socket_info );
			
			return -EINVAL;
		}
	}

	DEBUG(3, "ep93xx_init GPIO_F: INTEN=0x%02x  DDR=0x%02x DB=0x%02x INTTYP1=0x%02x INTTYP2=0x%02x\n",
		 inl( GPIO_FINTEN ), inl( GPIO_PFDDR ),	inl( GPIO_FDB ),
		 inl( GPIO_FINTTYPE1 ), inl( GPIO_FINTTYPE2 ) );

	//
	// Set speed to the defaults
	//
	ulRegValue = ep93xx_calculate_timing( skt->speed_io );
	outl( ulRegValue, SMC_PCIO );
	ulRegValue = ep93xx_calculate_timing( skt->speed_attr );
	outl( ulRegValue, SMC_PCAttribute ); 
	ulRegValue = ep93xx_calculate_timing( skt->speed_mem );
	outl( ulRegValue, SMC_PCCommon ); 
	ulRegValue = inl( SMC_PCCommon ); // Push the out thru the wrapper

	DEBUG(3, "INITIALIZING SMC: Attr:%08x  Comm:%08x  IO:%08x  Ctrl:%08x\n",
		inl( SMC_PCAttribute ), inl( SMC_PCCommon), 
		inl( SMC_PCIO ), inl( SMC_PCMCIACtrl ) );

	/* Only advertise as many sockets as we can detect */
	if ((ret = register_ss_entry( EP93XX_MAX_SOCK, &ep93xx_service)) < 0){
		printk(KERN_ERR "Unable to register sockets\n");
		ep93xx_shutdown();
		return ret;
	}

	/*
	 * Start the event poll timer.  It will reschedule by itself afterwards.
	 */
	ep93xx_pcmcia_poll_event(0);

	return 0;
}

/* ------------------------------------------------------------------------- */

static void __exit ep93xx_exit(void)
{
	unregister_ss_entry(&ep93xx_service);
	ep93xx_shutdown();
}

/* ------------------------------------------------------------------------- */

static void ep93xx_interrupt(int irq, void *dev, struct pt_regs *regs)
{
	unsigned long ulTemp;
	unsigned int uiStatus, uiNewEvents;
	
	// Assuming we have only one socket.
	struct socket_info_t * skt = &socket_info[0];
  
//	if( irq != 0 )
//    	printk( "ep93xx_interrupt enter intstatus=0x%02x\n", inl(GPIO_INTSTATUSF) );
//	else
//		printk( "." );
	
	ep93xx_get_status( 0, &uiStatus );
	
	//
	// We're going to report only the events that have changed and that
	// are not masked off.
	//
	uiNewEvents = (uiStatus ^ skt->uiStatus) & skt->state.csc_mask;
	skt->uiStatus = uiStatus;

	if( (skt->cs_event_handler != 0) && (uiNewEvents != 0) )
		skt->cs_event_handler( skt->cs_handler_info, uiNewEvents );
	
	// Clear whatever interrupt we're servicing.
	switch( irq )
	{
		case EP93XX_PCMCIA_INT_CD1:
			outl( EP93XX_PCMCIA_CD1, GPIO_FEOI );
			break;

		case EP93XX_PCMCIA_INT_CD2:
			outl( EP93XX_PCMCIA_CD2, GPIO_FEOI );
			break;

		case EP93XX_PCMCIA_INT_BVD1:
			outl( EP93XX_PCMCIA_BVD1, GPIO_FEOI );
			break;

		case EP93XX_PCMCIA_INT_BVD2:
			outl( EP93XX_PCMCIA_BVD2, GPIO_FEOI );
			break;

		case EP93XX_PCMCIA_INT_RDY:
			if( skt->card_irq_handler )
				skt->card_irq_handler( irq, skt->dev_id, regs );

			outl( EP93XX_PCMCIA_RDY, GPIO_FEOI );
			break;

		default:
			break;
	}
	ulTemp = inl( GPIO_FEOI ); // read to push write out wrapper

	//DEBUG(3, "exit_irq_");
}

/* ------------------------------------------------------------------------- */

static int 
ep93xx_register_callback(u_short sock, ss_callback_t *call)
{
	struct socket_info_t * skt = &socket_info[sock];

	if (sock >= EP93XX_MAX_SOCK)
		return -EINVAL;

	if (call == NULL) {
	skt->cs_event_handler = NULL;
	MOD_DEC_USE_COUNT;
	} else {
	MOD_INC_USE_COUNT;
	skt->cs_event_handler = call->handler;
	skt->cs_handler_info = call->info;
	}
	return 0;
}

/* ------------------------------------------------------------------------- */

static int ep93xx_get_status( u_short sock, u_int *uiStatus )
{
	struct socket_info_t * skt = &socket_info[sock];
	unsigned long ulPFDR;

	if (sock >= EP93XX_MAX_SOCK)
		return -EINVAL;

	//
	// Read the GPIOs that are connected to the PCMCIA slot.
	//
	ulPFDR  = inl(GPIO_PFDR);
	
	*uiStatus = 0;
	
	//
	// If both CD1 and CD2 are low, set SS_DETECT bit.
	//
	*uiStatus = ((ulPFDR & (EP93XX_PCMCIA_CD1|EP93XX_PCMCIA_CD2)) == 0) ?
				SS_DETECT : 0;

	*uiStatus |= (ulPFDR & EP93XX_PCMCIA_WP) ? SS_WRPROT : 0;

	if (skt->state.flags & SS_IOCARD)
	{
		*uiStatus |= (ulPFDR & EP93XX_PCMCIA_BVD1) ? SS_STSCHG : 0;
	}
	else 
	{
		*uiStatus |= (ulPFDR & EP93XX_PCMCIA_RDY) ? SS_READY : 0;
		*uiStatus |= (ulPFDR & EP93XX_PCMCIA_BVD1) ? 0 : SS_BATDEAD;
		*uiStatus |= (ulPFDR & EP93XX_PCMCIA_BVD2) ? 0 : SS_BATWARN;
	}
	
	*uiStatus |= skt->state.Vcc ? SS_POWERON : 0;
	
	*uiStatus |= (ulPFDR & EP93XX_PCMCIA_VS1) ? 0 : SS_3VCARD;
	
	// We don't support X v.
	//*uiStatus |= state[sock].vs_Xv ? SS_XVCARD : 0;

//	DEBUG(3, "ep93xx_get_status: %08x %s%s%s%s%s%s%s%s\n", 
//		(unsigned int)ulPFDR,
//		*uiStatus & SS_DETECT  ? "DETECT "  : "",
//		*uiStatus & SS_READY   ? "READY "   : "",
//		*uiStatus & SS_BATDEAD ? "BATDEAD " : "",
//		*uiStatus & SS_BATWARN ? "BATWARN " : "",
//		*uiStatus & SS_POWERON ? "POWERON " : "",
//		*uiStatus & SS_STSCHG  ? "STSCHG "  : "",
//		*uiStatus & SS_3VCARD  ? "3VCARD "  : "",
//		*uiStatus & SS_XVCARD  ? "XVCARD "  : "");

	return 0;
}

/* ------------------------------------------------------------------------- */

static int ep93xx_inquire_socket(u_short sock, socket_cap_t *cap)
{
	DEBUG(3,"ep93xx_inquire_socket - sock = %d\n", sock );

	if (sock >= EP93XX_MAX_SOCK)
		return -EINVAL;

	cap->features=(SS_CAP_PAGE_REGS  | SS_CAP_VIRTUAL_BUS | SS_CAP_MEM_ALIGN |
		 SS_CAP_STATIC_MAP | SS_CAP_PCCARD);
	cap->irq_mask  = 0;
	cap->map_size  = PAGE_SIZE;
	cap->pci_irq   = EP93XX_PCMCIA_INT_RDY;
	cap->cardbus   = 0;
	cap->cb_bus    = NULL;
	cap->bus       = &(socket_info[sock].bus_ops);

	return 0;
}

/* ------------------------------------------------------------------------- */

static int ep93xx_get_socket(u_short sock, socket_state_t *state)
{
	DEBUG(3,"ep93xx_get_socket - sock = %d\n", sock );

	if (sock >= EP93XX_MAX_SOCK)
		return -EINVAL;

	*state = socket_info[sock].state; /* copy the whole structure */

	DEBUG(3, "GetSocket(%d) = flags %#3.3x, Vcc %d, Vpp %d, "
		  "io_irq %d, csc_mask %#2.2x\n", sock, state->flags,
		  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);

	return 0;
}

/* ------------------------------------------------------------------------- */

static int
ep93xx_set_socket( u_short sock, socket_state_t *state )
{
	struct socket_info_t * skt = &socket_info[sock];
	unsigned long ulTemp, IrqsToEnable = 0;
	int PcmciaReady = 0;

	DEBUG(2, "%s() for sock %u\n", __FUNCTION__, sock);

	if (sock >= EP93XX_MAX_SOCK)
		return -EINVAL;

	DEBUG(3, "\tmask:  %s%s%s%s%s%s\n\tflags: %s%s%s%s%s%s\n",
		(state->csc_mask==0)?"<NONE>":"",
		(state->csc_mask&SS_DETECT)?"DETECT ":"",
		(state->csc_mask&SS_READY)?"READY ":"",
		(state->csc_mask&SS_BATDEAD)?"BATDEAD ":"",
		(state->csc_mask&SS_BATWARN)?"BATWARN ":"",
		(state->csc_mask&SS_STSCHG)?"STSCHG ":"",
		(state->flags==0)?"<NONE>":"",
		(state->flags&SS_PWR_AUTO)?"PWR_AUTO ":"",
		(state->flags&SS_IOCARD)?"IOCARD ":"",
		(state->flags&SS_RESET)?"RESET ":"",
		(state->flags&SS_SPKR_ENA)?"SPKR_ENA ":"",
		(state->flags&SS_OUTPUT_ENA)?"OUTPUT_ENA ":"");

	DEBUG(3, "\tVcc %d  Vpp %d  irq %d\n",
		state->Vcc, state->Vpp, state->io_irq);

	ulTemp = inl( GPIO_FINTEN ) & ~EP93XX_GPIOS;
	outl( ulTemp, GPIO_FINTEN );
	ulTemp = inl( GPIO_FINTEN );
	if (socket_info[sock].Vcc && state->Vcc &&
	    !(state->flags & SS_RESET))
		PcmciaReady = EP93XX_PCMCIA_RDY;

	//
	// Set Vcc level.  If an illegal voltage is specified, bail w/ error.
	//
	if( ep93xx_set_voltage( sock, state->Vcc ) < 0 )
		return -EINVAL;


	/* Silently ignore output enable, speaker enable. */

	//
	// Enable PCMCIA, Enable Wait States, Set or Clear card reset.
	//
	ulTemp = (inl( SMC_PCMCIACtrl ) | PCCONT_WEN | PCCONT_PC1EN) & ~PCCONT_PC1RST;
	if( state->flags & SS_RESET )
		ulTemp |= PCCONT_PC1RST;

	outl( ulTemp, SMC_PCMCIACtrl );
	ulTemp = inl( SMC_PCMCIACtrl );
	
	//
	// Enable interrupts in hw that are specified in csc_mask.
	//
	if (state->csc_mask & SS_DETECT)
		IrqsToEnable |= EP93XX_PCMCIA_CD1 | EP93XX_PCMCIA_CD2;

	if( state->flags & SS_IOCARD ) 
	{
		if (state->csc_mask & SS_STSCHG)  IrqsToEnable |= EP93XX_PCMCIA_BVD1;
	} 
	else 
	{
		if (state->csc_mask & SS_BATDEAD) IrqsToEnable |= EP93XX_PCMCIA_BVD1;
		if (state->csc_mask & SS_BATWARN) IrqsToEnable |= EP93XX_PCMCIA_BVD2;
		if (state->csc_mask & SS_READY)   IrqsToEnable |= EP93XX_PCMCIA_RDY;
	}
	
	skt->IrqsToEnable = IrqsToEnable;

	//
	// Clear and enable the new interrupts.
	//
	outl( IrqsToEnable | PcmciaReady, GPIO_FEOI );
	ulTemp = inl( GPIO_FEOI );

	ulTemp = inl( GPIO_FINTEN ) & ~EP93XX_GPIOS;
	ulTemp |= IrqsToEnable | PcmciaReady;
	outl( ulTemp, GPIO_FINTEN );
	ulTemp = inl( GPIO_FINTEN );

	skt->state = *state;

	return 0;
}

/* ------------------------------------------------------------------------- */

static int
ep93xx_get_io_map(u_short sock, struct pccard_io_map *io_map)
{
	if ( (sock >= EP93XX_MAX_SOCK) || (io_map->map >= EP93XX_MAX_IO_WIN) )
		return -EINVAL;
		
	*io_map = socket_info[sock].io_map[io_map->map];

	DEBUG(3,"GetIOMap(%d, %d) = %#2.2x, %d ns, "
		  "%#4.4x-%#4.4x\n", sock, io_map->map, io_map->flags,
		  io_map->speed, io_map->start, io_map->stop);

	return 0;
}

/* ------------------------------------------------------------------------- */

static int
ep93xx_set_io_map(u_short sock, struct pccard_io_map *io_map)
{
	struct socket_info_t * skt = &socket_info[sock];
	unsigned long ulSMC_PCIO;
	unsigned int speed=0, i;
	
	DEBUG(2, "%s() for sock %u - Input:\n", __FUNCTION__, sock);

	if (sock >= EP93XX_MAX_SOCK)
		return -EINVAL;

	DEBUG(3, "\tmap %u  speed %u\n\tstart 0x%08x  stop 0x%08x\n",
		io_map->map, io_map->speed, io_map->start, io_map->stop);
		
	DEBUG(3, "\tflags: %s%s%s%s%s%s%s%s\n",
		(io_map->flags==0)?"<NONE>":"",
		(io_map->flags&MAP_ACTIVE)?"ACTIVE ":"",
		(io_map->flags&MAP_16BIT)?"16BIT ":"",
		(io_map->flags&MAP_AUTOSZ)?"AUTOSZ ":"",
		(io_map->flags&MAP_0WS)?"0WS ":"",
		(io_map->flags&MAP_WRPROT)?"WRPROT ":"",
		(io_map->flags&MAP_USE_WAIT)?"USE_WAIT ":"",
		(io_map->flags&MAP_PREFETCH)?"PREFETCH ":"");

	if ((io_map->map >= EP93XX_MAX_IO_WIN) || (io_map->stop < io_map->start)) 
		return -EINVAL;

	if (io_map->flags & MAP_ACTIVE) 
	{
		if (io_map->speed == 0)
			io_map->speed = EP93XX_PCMCIA_IO_ACCESS;

		speed = io_map->speed;

		// Go thru our array of mappings, find the lowest speed 
		// (largest # of nSec) for an active mapping...
		for( i = 0 ; i < EP93XX_MAX_IO_WIN ; i++ ) {
			if ( (skt->io_map[i].flags & MAP_ACTIVE) && 
				 (speed > skt->io_map[i].speed) )
				speed = skt->io_map[i].speed;
		}

		ulSMC_PCIO = ep93xx_calculate_timing( speed );

		//if( io_map->flags & MAP_16BIT )
			ulSMC_PCIO |= PCCONFIG_MW_16BIT;

		outl( ulSMC_PCIO, SMC_PCIO ); 
		ulSMC_PCIO = inl( SMC_PCIO );

		skt->speed_io = speed;
	}

	if (io_map->stop == 1)
		io_map->stop = PAGE_SIZE-1;


	io_map->stop = (io_map->stop - io_map->start) + (unsigned int)skt->virt_io;
	io_map->start = (unsigned int)skt->virt_io;

	skt->io_map[io_map->map] = *io_map;

	DEBUG(2, "%s() for sock %u - Output:\n", __FUNCTION__, sock);

	DEBUG(3, "\tmap %u  speed %u\n\tstart 0x%08x  stop 0x%08x (virt_io is 0x%08x)\n",
		io_map->map, io_map->speed, io_map->start, io_map->stop, (unsigned int)skt->virt_io);
		
	DEBUG(3, "\tflags: %s%s%s%s%s%s%s%s\n",
		(io_map->flags==0)?"<NONE>":"",
		(io_map->flags&MAP_ACTIVE)?"ACTIVE ":"",
		(io_map->flags&MAP_16BIT)?"16BIT ":"",
		(io_map->flags&MAP_AUTOSZ)?"AUTOSZ ":"",
		(io_map->flags&MAP_0WS)?"0WS ":"",
		(io_map->flags&MAP_WRPROT)?"WRPROT ":"",
		(io_map->flags&MAP_USE_WAIT)?"USE_WAIT ":"",
		(io_map->flags&MAP_PREFETCH)?"PREFETCH ":"");

//	printk("set_io_map: speed:%d  Attr:%08x  Comm:%08x  IO:%08x  Ctrl:%08x\n",  speed,
//		inl( SMC_PCAttribute ), inl( SMC_PCCommon), 
//		inl( SMC_PCIO ), inl( SMC_PCMCIACtrl ) );

	return 0;
}

/* ------------------------------------------------------------------------- */

static int 
ep93xx_get_mem_map(u_short sock, struct pccard_mem_map *mem_map)
{
	if( (sock>=EP93XX_MAX_SOCK) | (mem_map->map >= EP93XX_MAX_MEM_WIN) )
		return -EINVAL;
	
	*mem_map = socket_info[sock].mem_map[mem_map->map]; /* copy the struct */
	
	DEBUG(3, "GetMemMap(%d, %d) = %#2.2x, %d ns, "
		  "%#5.5lx-%#5.5lx, %#5.5x\n", sock, mem_map->map, mem_map->flags,
		  mem_map->speed, mem_map->sys_start, mem_map->sys_stop, mem_map->card_start);

	return 0;
}

/* ------------------------------------------------------------------------- */

static int
ep93xx_set_mem_map(u_short sock, struct pccard_mem_map *map)
{
	struct socket_info_t * skt = &socket_info[sock];
	unsigned long start, ulRegValue;
	unsigned int speed=0, i;

	DEBUG(2, "%s() for sock %u  Input:\n", __FUNCTION__, sock);

	if (sock >= EP93XX_MAX_SOCK)
		return -EINVAL;

	DEBUG(3, "\tmap %u speed %u sys_start %08lx sys_stop %08lx card_start %08x\n",
		map->map, map->speed, map->sys_start, map->sys_stop, map->card_start);

	DEBUG(3, "\tflags: %s%s%s%s%s%s%s%s\n",
		(map->flags==0)?"<NONE>":"",
		(map->flags&MAP_ACTIVE)?"ACTIVE ":"",
		(map->flags&MAP_16BIT)?"16BIT ":"",
		(map->flags&MAP_AUTOSZ)?"AUTOSZ ":"",
		(map->flags&MAP_0WS)?"0WS ":"",
		(map->flags&MAP_WRPROT)?"WRPROT ":"",
		(map->flags&MAP_ATTRIB)?"ATTRIB ":"",
		(map->flags&MAP_USE_WAIT)?"USE_WAIT ":"");

	if (map->map >= EP93XX_MAX_MEM_WIN){
		printk(KERN_ERR "%s(): map (%d) out of range\n", __FUNCTION__,
			map->map);
		return -1;
	}

	if (map->flags & MAP_ACTIVE){
		if (map->speed == 0){
			if (skt->state.Vcc == 33)
				map->speed = EP93XX_PCMCIA_3V_MEM_ACCESS;
			else
				map->speed = EP93XX_PCMCIA_5V_MEM_ACCESS;
		}
		
		speed = map->speed;

		if (map->flags & MAP_ATTRIB) 
		{
			// Go thru our array of mappings, find the lowest speed 
			// (largest # of nSec) for an active mapping...
			for( i = 0 ; i < EP93XX_MAX_MEM_WIN ; i++ ) {
				if( (skt->mem_map[i].flags & MAP_ACTIVE) && 
					(speed > skt->mem_map[i].speed) )
					speed = skt->mem_map[i].speed;
			}

			ulRegValue = ep93xx_calculate_timing( speed );

			//if( map->flags & MAP_16BIT )
				ulRegValue |= PCCONFIG_MW_16BIT;

			outl( ulRegValue, SMC_PCAttribute ); 
			ulRegValue = inl( SMC_PCAttribute );
			
			skt->speed_attr = speed;
		} 
		else 
		{
			for( i = 0 ; i < EP93XX_MAX_MEM_WIN ; i++ )
			{
				if ((skt->mem_map[i].flags & MAP_ACTIVE) && 
					(speed > skt->mem_map[i].speed) )
					speed = skt->mem_map[i].speed;
			}

			ulRegValue = ep93xx_calculate_timing( speed );
			//if( map->flags & MAP_16BIT )
				ulRegValue |= PCCONFIG_MW_16BIT;

			outl( ulRegValue, SMC_PCCommon ); 
			ulRegValue = inl( SMC_PCCommon );
			
			skt->speed_mem = speed;
		}
	}

	start = (map->flags & MAP_ATTRIB) ? EP93XX_PHYS_ADDR_PCMCIAATTR : EP93XX_PHYS_ADDR_PCMCIAMEM;

	if (map->sys_stop == 0)
		map->sys_stop = PAGE_SIZE-1;

	map->sys_stop -= map->sys_start;
	map->sys_stop += start;
	map->sys_start = start;

	skt->mem_map[map->map] = *map;

	DEBUG(2, "%s() for sock %u  Output:\n", __FUNCTION__, sock);

	DEBUG(3, "\tmap %u speed %u sys_start %08lx sys_stop %08lx card_start %08x\n",
		map->map, map->speed, map->sys_start, map->sys_stop, map->card_start);

	DEBUG(3, "\tflags: %s%s%s%s%s%s%s%s\n",
		(map->flags==0)?"<NONE>":"",
		(map->flags&MAP_ACTIVE)?"ACTIVE ":"",
		(map->flags&MAP_16BIT)?"16BIT ":"",
		(map->flags&MAP_AUTOSZ)?"AUTOSZ ":"",
		(map->flags&MAP_0WS)?"0WS ":"",
		(map->flags&MAP_WRPROT)?"WRPROT ":"",
		(map->flags&MAP_ATTRIB)?"ATTRIB ":"",
		(map->flags&MAP_USE_WAIT)?"USE_WAIT ":"");

	DEBUG(3, "ep93xx_set_mem_map - exit\n");
	return 0;
}

/*====================================================================*/

typedef int (*subfn_t)(u_short, void *);
	
static subfn_t service_table[] = 
{
	(subfn_t)&ep93xx_register_callback,
	(subfn_t)&ep93xx_inquire_socket,
	(subfn_t)&ep93xx_get_status,
	(subfn_t)&ep93xx_get_socket,
	(subfn_t)&ep93xx_set_socket,
	(subfn_t)&ep93xx_get_io_map,
	(subfn_t)&ep93xx_set_io_map,
	(subfn_t)&ep93xx_get_mem_map,
	(subfn_t)&ep93xx_set_mem_map,
};

#define NFUNC (sizeof(service_table)/sizeof(subfn_t))

static int ep93xx_service(u_int sock, u_int cmd, void *arg)
{
	return (cmd < NFUNC) ? service_table[cmd](sock, arg) : -EINVAL; 
}

/*====================================================================*/

module_init(ep93xx_init);
module_exit(ep93xx_exit);
