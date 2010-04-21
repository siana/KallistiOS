/* KallistiOS ##version##

   dc/cdrom.h
   (c)2000-2001 Dan Potter

*/

#ifndef __DC_CDROM_H
#define __DC_CDROM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup cmd_codes Command codes (thanks maiwe)
	@{
*/
#define CMD_PIOREAD	16
#define CMD_DMAREAD	17
#define CMD_GETTOC	18
#define CMD_GETTOC2	19
#define CMD_PLAY	20
#define CMD_PLAY2	21
#define CMD_PAUSE	22
#define CMD_RELEASE	23
#define CMD_INIT	24
#define CMD_SEEK	27
#define CMD_READ	28
#define CMD_STOP	33
#define CMD_GETSCD	34
#define CMD_GETSES	35
/** @} */

/** \defgroup cmd_response Command responses
	@{
*/
#define ERR_OK		0
#define ERR_NO_DISC	1
#define ERR_DISC_CHG	2
#define ERR_SYS		3
#define ERR_ABORTED	4
#define ERR_NO_ACTIVE	5
/** @} */

/** \defgroup cmd_status_response Command Status responses
	@{
*/
#define FAILED		-1
#define NO_ACTIVE	0
#define PROCESSING	1
#define COMPLETED	2
#define ABORTED		3
/** @} */

/** \defgroup cdda_read_modes CDDA Read Modes
	@{
*/
#define CDDA_TRACKS	1
#define CDDA_SECTORS	2
/** @} */

/** \defgroup status_values Status values
	@{
*/
#define CD_STATUS_BUSY		0
#define CD_STATUS_PAUSED	1
#define CD_STATUS_STANDBY	2
#define CD_STATUS_PLAYING	3
#define CD_STATUS_SEEKING	4
#define CD_STATUS_SCANNING	5
#define CD_STATUS_OPEN		6
#define CD_STATUS_NO_DISC	7
/** @} */

/** \defgroup disk_types Disk types
	@{
*/
#define CD_CDDA		0
#define CD_CDROM	0x10
#define CD_CDROM_XA	0x20
#define CD_CDI		0x30
#define CD_GDROM	0x80
/** @} */

/** TOC structure returned by the BIOS */
typedef struct {
	uint32	entry[99];
	uint32	first, last;
	uint32	leadout_sector;
} CDROM_TOC;

/** \defgroup toc_access TOC access macros
	@{
*/
#define TOC_LBA(n) ((n) & 0x00ffffff)
#define TOC_ADR(n) ( ((n) & 0x0f000000) >> 24 )
#define TOC_CTRL(n) ( ((n) & 0xf0000000) >> 28 )
#define TOC_TRACK(n) ( ((n) & 0x00ff0000) >> 16 )
/** @} */

/**
	\brief Sets the sector size

	\param size The size of the sector you're desiring
*/
void set_sector_size (int size);

/**
	\brief Command execution sequence

	\return \ref cmd_response
*/
int cdrom_exec_cmd(int cmd, void *param);

/**
	\brief Gets the status of the the cdrom
	\return the status of the drive as two integers (see constants)
*/
int cdrom_get_status(int *status, int *disc_type);

/**
	\brief Re-init the drive, e.g., after a disc change, etc

	\return \ref cmd_response
*/
int cdrom_reinit();

/**
	\brief Read the table of contents

	\return \ref cmd_response
*/
int cdrom_read_toc(CDROM_TOC *toc_buffer, int session);

/**
	\brief Read one or more sector

	\return \ref cmd_response
*/
int cdrom_read_sectors(void *buffer, int sector, int cnt);

/**
	\brief Locate the LBA sector of the data track
*/
uint32 cdrom_locate_data_track(CDROM_TOC *toc);

/**
	\brief Play CDDA audio tracks or sectors

	\return \ref cmd_response
*/
int cdrom_cdda_play(uint32 start, uint32 end, uint32 loops, int mode);

/*
	\brief Pause CDDA audio playback

	\return \ref cmd_response
*/
int cdrom_cdda_pause();

/**
	\brief Resume CDDA audio playback

	\return \ref cmd_response
*/
int cdrom_cdda_resume();

/**
	\brief Spin down the CD

	\return \ref cmd_response
*/
int cdrom_spin_down();

/**
	\brief Initialize cdrom

	\return -1 if cdrom_init has already been called, otherwise 0
*/
int cdrom_init();

/**
	\brief Shutdown cdrom
*/
void cdrom_shutdown();

__END_DECLS

#endif	/* __DC_CDROM_H */

