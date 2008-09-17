/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/flashrom.h
   Copyright (C)2003 Dan Potter
   Copyright (C)2008 Lawrence Sebald

*/


#ifndef __DC_FLASHROM_H
#define __DC_FLASHROM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/**
  \file Implements wrappers for the BIOS flashrom syscalls, and some
  utilities to make it easier to use the flashrom info. Note that
  because the flash writing can be such a dangerous thing potentially
  (I haven't deleted my flash to see what happens, but given the
  info stored here it sounds like a Bad Idea(tm)) the syscalls for
  the WRITE and DELETE operations are not enabled by default. If you
  are 100% sure you really want to be writing to the flash and you
  know what you're doing, then you can edit flashrom.c and re-enable
  them there. */

/**
  An enumeration of partitions available in the flashrom. */
#define FLASHROM_PT_SYSTEM		0	/*< Factory settings (read-only, 8K) */
#define FLASHROM_PT_RESERVED	1	/*< reserved (all 0s, 8K) */
#define FLASHROM_PT_BLOCK_1		2	/*< Block allocated (16K) */
#define FLASHROM_PT_SETTINGS	3	/*< Game settings (block allocated, 32K) */
#define FLASHROM_PT_BLOCK_2		4	/*< Block allocated (64K) */

/**
  An enumeration of logical blocks available in the flashrom. */
#define FLASHROM_B1_SYSCFG			0x05	/*< System config (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_1	0x80	/*< PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_2	0x81	/*< PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_3	0x82	/*< PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_4	0x83	/*< PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_5	0x84	/*< PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_PPP1			0xC0	/*< PlanetWeb PPP settings (BLOCK_1) */
#define FLASHROM_B1_PW_PPP2			0xC1	/*< PlanetWeb PPP settings (BLOCK_1) */
#define FLASHROM_B1_PW_DNS			0xC2	/*< PlanetWeb DNS settings (BLOCK_1) */
#define FLASHROM_B1_PW_EMAIL1		0xC3	/*< PlanetWeb Email settings (BLOCK_1) */
#define FLASHROM_B1_PW_EMAIL2		0xC4	/*< PlanetWeb Email settings (BLOCK_1) */
#define FLASHROM_B1_PW_EMAIL_PROXY	0xC5	/*< PlanetWeb Email/Proxy settings (BLOCK_1) */
#define FLASHROM_B1_IP_SETTINGS		0xE0	/*< IP settings for BBA (BLOCK_1) */
#define FLASHROM_B1_EMAIL			0xE2	/*< Email address (BLOCK_1) */
#define FLASHROM_B1_SMTP			0xE4	/*< SMTP server setting (BLOCK_1) */
#define FLASHROM_B1_POP3			0xE5	/*< POP3 server setting (BLOCK_1) */
#define FLASHROM_B1_POP3LOGIN		0xE6	/*< POP3 login setting (BLOCK_1) */
#define FLASHROM_B1_POP3PASSWD		0xE7	/*< POP3 password setting + proxy (BLOCK_1) */
#define FLASHROM_B1_PPPLOGIN		0xE8	/*< PPP username + proxy (BLOCK_1) */
#define FLASHROM_B1_PPPPASSWD		0xE9	/*< PPP passwd (BLOCK_1) */

/**
  Implements the FLASHROM_INFO syscall; given a partition ID,
  return two ints specifying the beginning and the size of
  the partition (respectively) inside the flashrom. Returns zero
  if successful, -1 otherwise. */
int flashrom_info(int part, int * start_out, int * size_out);

/**
  Implements the FLASHROM_READ syscall; given a flashrom offset,
  an output buffer, and a count, this reads data from the
  flashrom. Returns the number of bytes read if successful,
  or -1 otherwise. */
int flashrom_read(int offset, void * buffer_out, int bytes);

/**
  Implements the FLASHROM_WRITE syscall; given a flashrom offset,
  an input buffer, and a count, this writes data to the flashrom.
  Returns the number of bytes written if successful, -1 otherwise.

  NOTE: It is not possible to write ones to the flashrom over zeros.
  If you want to do this, you must save the old data in the flashrom,
  delete it out, and save the new data back. */
int flashrom_write(int offset, void * buffer, int bytes);

/**
  Implements the FLASHROM_DELETE syscall; given a partition offset,
  that entire partition of the flashrom will be deleted and all data
  will be reset to FFs. Returns zero if successful, -1 on failure. */
int flashrom_delete(int offset);


/* Medium-level functions */
/**
  Returns a numbered logical block from the requested partition. The newest
  data is returned. 'buffer_out' must have enough room for 60 bytes of
  data. */
int flashrom_get_block(int partid, int blockid, uint8 * buffer_out);


/* Higher level functions */

/**
  Language settings possible in the BIOS menu. These will be returned
  from flashrom_get_language(). */
#define FLASHROM_LANG_JAPANESE	0
#define FLASHROM_LANG_ENGLISH	1
#define FLASHROM_LANG_GERMAN	2
#define FLASHROM_LANG_FRENCH	3
#define FLASHROM_LANG_SPANISH	4
#define FLASHROM_LANG_ITALIAN	5

/**
  This struct will be filled by calling the flashrom_get_syscfg call
  below. */
typedef struct flashrom_syscfg {
	int	language;	/*< Language setting (see defines above) */
	int	audio;		/*< 0 == mono, 1 == stereo */
	int	autostart;	/*< 0 == off, 1 == on */
} flashrom_syscfg_t;

/**
  Retrieves the current syscfg settings and fills them into the struct
  passed in to us. */
int flashrom_get_syscfg(flashrom_syscfg_t * out);


/**
  Region settings possible in the system flash (partition 0). */
#define FLASHROM_REGION_UNKNOWN	0
#define FLASHROM_REGION_JAPAN	1
#define FLASHROM_REGION_US	2
#define FLASHROM_REGION_EUROPE	3

/**
  Retrieves the console's region code. This is still somewhat 
  experimental, it may not function 100% on all DCs. Returns
  one of the codes above or -1 on error. */
int flashrom_get_region();

/**
  Method constants for the ispcfg struct */
#define FLASHROM_ISP_DHCP	0
#define FLASHROM_ISP_STATIC	1
#define FLASHROM_ISP_DIALUP	2
#define FLASHROM_ISP_PPPOE	4

/**
  Valid field constants in the ispcfg structure. */
#define FLASHROM_ISP_IP			(1 <<  0)
#define FLASHROM_ISP_NETMASK	(1 <<  1)
#define FLASHROM_ISP_BROADCAST	(1 <<  2)
#define FLASHROM_ISP_GATEWAY	(1 <<  3)
#define FLASHROM_ISP_DNS		(1 <<  4)
#define FLASHROM_ISP_HOSTNAME	(1 <<  5)
#define FLASHROM_ISP_EMAIL		(1 <<  6)
#define FLASHROM_ISP_SMTP		(1 <<  7)
#define FLASHROM_ISP_POP3		(1 <<  8)
#define FLASHROM_ISP_POP3_USER	(1 <<  9)
#define FLASHROM_ISP_POP3_PASS	(1 << 10)
#define FLASHROM_ISP_PROXY_HOST	(1 << 11)
#define FLASHROM_ISP_PROXY_PORT	(1 << 12)
#define FLASHROM_ISP_PPP_USER	(1 << 13)
#define FLASHROM_ISP_PPP_PASS	(1 << 14)
#define FLASHROM_ISP_OUT_PREFIX	(1 << 15)
#define FLASHROM_ISP_CW_PREFIX	(1 << 16)
#define FLASHROM_ISP_REAL_NAME	(1 << 17)
#define FLASHROM_ISP_MODEM_INIT	(1 << 18)
#define FLASHROM_ISP_AREA_CODE	(1 << 19)
#define FLASHROM_ISP_LD_PREFIX	(1 << 20)
#define FLASHROM_ISP_PHONE1		(1 << 21)
#define FLASHROM_ISP_PHONE2		(1 << 22)

/**
  Flags for the ispcfg structure */
#define FLASHROM_ISP_DIAL_AREACODE	(1 <<  0)
#define FLASHROM_ISP_USE_PROXY		(1 <<  1)
#define FLASHROM_ISP_PULSE_DIAL		(1 <<  2)
#define FLASHROM_ISP_BLIND_DIAL		(1 <<  3)

/**
  This struct will be filled by calling flashrom_get_ispcfg below.
  Thanks to Sam Steele for the information on DreamPassport's ISP settings.
  Note that this structure has been completely reworked so that it is more
  generic and can support both DreamPassport and PlanetWeb's settings. */
typedef struct flashrom_ispcfg {
	int	method;			/*< DHCP, Static, dialup(?), PPPoE */
	uint32	valid_fields;	/*< Which fields are valid? */
	uint32	flags;		/*< Various flags that can be set in options */

	uint8	ip[4];		/*< Host IP address */
	uint8	nm[4];		/*< Netmask */
	uint8	bc[4];		/*< Broadcast address */
	uint8	gw[4];		/*< Gateway address */
	uint8	dns[2][4];	/*< DNS servers (2) */
	int	proxy_port;			/*< Proxy server port */
	char	hostname[24];	/*< DHCP/Host name */
	char	email[64];	/*< Email address */
	char	smtp[31];	/*< SMTP server */
	char	pop3[31];	/*< POP3 server */
	char	pop3_login[20];		/*< POP3 login */
	char	pop3_passwd[32];	/*< POP3 passwd */
	char	proxy_host[31];		/*< Proxy server hostname */
	char	ppp_login[29];	/*< PPP login */
	char	ppp_passwd[20];	/*< PPP password */
	char	out_prefix[9];	/*< Outside dial prefix */
	char	cw_prefix[9];	/*< Call waiting prefix */
	char	real_name[31];	/*< The "Real Name" field of PlanetWeb */
	char	modem_init[33];	/*< The modem init string to use */
	char	area_code[4];	/*< The area code the user is in */
	char	ld_prefix[21];	/*< The long-distance dial prefix */
	char	p1_areacode[4];	/*< Phone number 1's area code */
	char	phone1[26];	/*< Phone number 1 */
	char	p2_areacode[4];	/*< Phone number 2's area code */
	char	phone2[26];	/*< Phone number 2 */
} flashrom_ispcfg_t;

/**
  Retrieves the console's ISP settings as set by DreamPassport, if they exist.
  Returns -1 on error (none of the settings can be found, or some other error),
  or >=0 on success. You should check the valid_fields bitfield for the part of
  the struct you want before relying on the data. */
int flashrom_get_ispcfg(flashrom_ispcfg_t * out);

/**
  Retrieves the console's ISP settings as set by PlanetWeb (1.0 and 2.1 have
  been verified to work), if they exist. Returns -1 on error (generally if the
  PlanetWeb settings are non-existant) or >= 0 on success. You should check the
  valid_fields bitfield for the part of the struct you want before relying on
  the data. */
int flashrom_get_pw_ispcfg(flashrom_ispcfg_t *out);

/* More to come later */

__END_DECLS

#endif	/* __DC_FLASHROM_H */

