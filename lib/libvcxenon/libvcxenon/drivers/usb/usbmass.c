/*  *********************************************************************
    *  Broadcom Common Firmware Environment (CFE)
    *  
    *  USB Mass-Storage driver			File: usbmass.c
    *  
    *  This driver deals with mass-storage devices that support
    *  the SCSI Transparent command set and USB Bulk-Only protocol
    *  
    *  Author:  Mitch Lichtenberg
    *  
    *********************************************************************  
    *
    *  Copyright 2000,2001,2002,2003
    *  Broadcom Corporation. All rights reserved.
    *  
    *  This software is furnished under license and may be used and 
    *  copied only in accordance with the following terms and 
    *  conditions.  Subject to these conditions, you may download, 
    *  copy, install, use, modify and distribute modified or unmodified 
    *  copies of this software in source and/or binary form.  No title 
    *  or ownership is transferred hereby.
    *  
    *  1) Any source code used, modified or distributed must reproduce 
    *     and retain this copyright notice and list of conditions 
    *     as they appear in the source file.
    *  
    *  2) No right is granted to use any trade name, trademark, or 
    *     logo of Broadcom Corporation.  The "Broadcom Corporation" 
    *     name may not be used to endorse or promote products derived 
    *     from this software without the prior written permission of 
    *     Broadcom Corporation.
    *  
    *  3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR
    *     IMPLIED WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED
    *     WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
    *     PURPOSE, OR NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT 
    *     SHALL BROADCOM BE LIABLE FOR ANY DAMAGES WHATSOEVER, AND IN 
    *     PARTICULAR, BROADCOM SHALL NOT BE LIABLE FOR DIRECT, INDIRECT,
    *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
    *     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    *     GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    *     BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
    *     OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
    *     TORT (INCLUDING NEGLIGENCE OR OTHERWISE), EVEN IF ADVISED OF 
    *     THE POSSIBILITY OF SUCH DAMAGE.
    ********************************************************************* */


#ifndef _CFE_
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <stdint.h>
#include "usbhack.h"
#include "nocfe/lib_malloc.h"
#include "nocfe/lib_queue.h"
#else
#include "nocfe/cfe.h"
#endif

//=====================
#include "nocfe/cfe.h"

#include "usbchap9.h"
#include "usbd.h"

/*  *********************************************************************
    *  USB Mass-Storage class Constants
    ********************************************************************* */

#define USBMASS_CBI_PROTOCOL	0
#define USBMASS_CBI_NOCOMPLETE_PROTOCOL 1
#define USBMASS_BULKONLY_PROTOCOL 0x50

#define USBMASS_SUBCLASS_RBC	0x01
#define USBMASS_SUBCLASS_SFF8020 0x02
#define USBMASS_SUBCLASS_QIC157	0x03
#define USBMASS_SUBCLASS_UFI	0x04
#define USBMASS_SUBCLASS_SFF8070 0x05
#define USBMASS_SUBCLASS_SCSI	0x06

#define USBMASS_CSW_PASS	0x00
#define USBMASS_CSW_FAILED	0x01
#define USBMASS_CSW_PHASEERR	0x02

#define USBMASS_CBW_SIGNATURE	0x43425355
#define USBMASS_CSW_SIGNATURE	0x53425355

/*  *********************************************************************
    *  USB Mass-Storage class Structures
    ********************************************************************* */

typedef struct usbmass_cbw_s {
    uint8_t dCBWSignature0,dCBWSignature1,dCBWSignature2,dCBWSignature3;
    uint8_t dCBWTag0,dCBWTag1,dCBWTag2,dCBWTag3;
    uint8_t dCBWDataTransferLength0,dCBWDataTransferLength1,
	dCBWDataTransferLength2,dCBWDataTransferLength3;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN;
    uint8_t bCBWCBLength;
    uint8_t CBWCB[16];
} usbmass_cbw_t;

typedef struct usbmass_csw_s {
    uint8_t dCSWSignature0,dCSWSignature1,dCSWSignature2,dCSWSignature3;
    uint8_t dCSWTag0,dCSWTag1,dCSWTag2,dCSWTag3;
    uint8_t dCSWDataResidue0,dCSWDataResidue1,dCSWDataResidue2,dCSWDataResidue3;
    uint8_t bCSWStatus;
} usbmass_csw_t;

#define GETCBWFIELD(s,f) ((uint32_t)((s)->f##0) | ((uint32_t)((s)->f##1) << 8) | \
                          ((uint32_t)((s)->f##2) << 16) | ((uint32_t)((s)->f##3) << 24))
#define PUTCBWFIELD(s,f,v) (s)->f##0 = (v & 0xFF); \
                           (s)->f##1 = ((v)>>8 & 0xFF); \
                           (s)->f##2 = ((v)>>16 & 0xFF); \
                           (s)->f##3 = ((v)>>24 & 0xFF);


int usbmass_request_sense(usbdev_t *dev);

/*  *********************************************************************
    *  Linkage to CFE
    ********************************************************************* */

/*
 * Softc for the CFE side of the disk driver.
 */
#define MAX_SECTORSIZE 2048
typedef struct usbdisk_s {
    uint32_t usbdisk_sectorsize;
    uint32_t usbdisk_ttlsect;
    uint32_t usbdisk_devtype;
    int usbdisk_unit;
} usbdisk_t;



/*  *********************************************************************
    *  Forward Definitions
    ********************************************************************* */

static int usbmass_attach(usbdev_t *dev,usb_driver_t *drv);
static int usbmass_detach(usbdev_t *dev);

/*  *********************************************************************
    *  Structures
    ********************************************************************* */

typedef struct usbmass_softc_s {
    int umass_inpipe;
    int umass_outpipe;
    int umass_devtype;
    uint32_t umass_curtag;
    int umass_unit;
    void *bdev;
    usbdev_t *dev;
} usbmass_softc_t;

usb_driver_t usbmass_driver = {
    "Mass-Storage Device",
    usbmass_attach,
    usbmass_detach
};

usbdev_t *usbmass_dev = NULL;		/* XX hack for testing only */

/*  *********************************************************************
    *  usbmass_mass_storage_reset(dev,ifc)
    *  
    *  Do a bulk-only mass-storage reset.
    *  
    *  Input parameters: 
    *  	   dev - device to reset
    *      ifc - interface number to reset (bInterfaceNum)
    *  	   
    *  Return value:
    *  	   status
    ********************************************************************* */

#define usbmass_mass_storage_reset(dev,ifc) \
     usb_simple_request(dev,0x21,0xFF,ifc,0)

#if 0
/*  *********************************************************************
    *  usbmass_get_max_lun(dev,lunp)
    *  
    *  Get maximum LUN from device
    *  
    *  Input parameters: 
    *  	   dev - device to reset
    *      lunp - pointer to int to receive max lun
    *  	   
    *  Return value:
    *  	   status
    ********************************************************************* */

static int usbmass_get_max_lun(usbdev_t *dev,int *lunp)
{
    uint8_t buf = 0;
    int res;

    res = usb_std_request(dev,0xA1,0xFE,0,0,&buf,1);

    if (res < 0) return res;

    if (lunp) *lunp = (int) buf;
    return 0;
}

#endif


/*  *********************************************************************
    *  usbmass_stall_recovery(dev)
    *  
    *  Do whatever it takes to unstick a stalled mass-storage device.
    *  
    *  Input parameters: 
    *  	   dev - usb device
    *  	   
    *  Return value:
    *  	   nothing
    ********************************************************************* */

static void usbmass_stall_recovery(usbdev_t *dev)
{
    usbmass_softc_t *softc;

    softc = (usbmass_softc_t *) dev->ud_private;

    usb_clear_stall(dev,softc->umass_inpipe);

    usbmass_request_sense(dev);
}


/*  *********************************************************************
    *  usbmass_read_capacity(dev,sectornum,buffer)
    *  
    *  Reads a sector from the device.
    *  
    *  Input parameters: 
    *  	   dev - usb device
    *  	   sectornum - sector number to read
    *  	   buffer - place to put sector we read
    *  	   
    *  Return value:
    *  	   status
    ********************************************************************* */

int usbmass_request_sense(usbdev_t *dev)
{
    uint8_t *cbwcsw;
    uint8_t *sector;
    usbmass_cbw_t *cbw;
    usbmass_csw_t *csw;
    usbmass_softc_t *softc;
    int res;

    softc = (usbmass_softc_t *) dev->ud_private;

    cbwcsw = KMALLOC(64,32);
    sector = KMALLOC(64,32);

    memset(sector,0,64);

    cbw = (usbmass_cbw_t *) cbwcsw;
    csw = (usbmass_csw_t *) cbwcsw;

    /*
     * Fill in the fields of the CBW
     */

    PUTCBWFIELD(cbw,dCBWSignature,USBMASS_CBW_SIGNATURE);
    PUTCBWFIELD(cbw,dCBWTag,softc->umass_curtag);
    PUTCBWFIELD(cbw,dCBWDataTransferLength,18);
    cbw->bmCBWFlags = 0x80;		/* IN */
    cbw->bCBWLUN = 0;
    cbw->bCBWCBLength = 12;
    cbw->CBWCB[0] = 0x3;		/* REQUEST SENSE */
    cbw->CBWCB[1] = 0;
    cbw->CBWCB[2] = 0;
    cbw->CBWCB[3] = 0;
    cbw->CBWCB[4] = 18;			/* allocation length */
    cbw->CBWCB[5] = 0;
    cbw->CBWCB[6] = 0;
    cbw->CBWCB[7] = 0;
    cbw->CBWCB[8] = 0;
    cbw->CBWCB[9] = 0;

    softc->umass_curtag++;

    /*
     * Send the CBW 
     */

    res = usb_make_sync_request(dev,softc->umass_outpipe,(uint8_t *) cbw,
				sizeof(usbmass_cbw_t),UR_FLAG_OUT);

    /*
     * Get the data
     */

    memset(sector,0,18);
    res = usb_make_sync_request(dev,softc->umass_inpipe,sector,
				18,UR_FLAG_IN | UR_FLAG_SHORTOK);

    /*
     * Get the Status
     */

    memset(csw,0,sizeof(usbmass_csw_t));
    res = usb_make_sync_request(dev,softc->umass_inpipe,(uint8_t *) csw,
				sizeof(usbmass_csw_t),UR_FLAG_IN);

    KFREE(cbwcsw);

    KFREE(sector);

    return 0;

}

/*  *********************************************************************
    *  usbmass_read_sector(dev,sectornum,seccnt,buffer)
    *  
    *  Reads a sector from the device.
    *  
    *  Input parameters: 
    *  	   dev - usb device
    *  	   sectornum - sector number to read
    * 	   seccnt - count of sectors to read
    *  	   buffer - place to put sector we read
    *  	   
    *  Return value:
    *  	   status
    ********************************************************************* */

int usbmass_read_sector(usbdev_t *dev,uint32_t sectornum,uint32_t seccnt,
			hsaddr_t buffer);
int usbmass_read_sector(usbdev_t *dev,uint32_t sectornum,uint32_t seccnt,
				hsaddr_t buffer)
{
    uint8_t *cbwcsw;
    uint8_t *sector;
    usbmass_cbw_t *cbw;
    usbmass_csw_t *csw;
    usbmass_softc_t *softc;
    int res;

    softc = (usbmass_softc_t *) dev->ud_private;

    cbwcsw = KMALLOC(64,32);
    sector = (uint8_t *) HSADDR2PTR(buffer);		/* XXX not 64-bit compatible */

    cbw = (usbmass_cbw_t *) cbwcsw;
    csw = (usbmass_csw_t *) cbwcsw;

    /*
     * Fill in the fields of the CBW
     */

    PUTCBWFIELD(cbw,dCBWSignature,USBMASS_CBW_SIGNATURE);
    PUTCBWFIELD(cbw,dCBWTag,softc->umass_curtag);
    PUTCBWFIELD(cbw,dCBWDataTransferLength,(512*seccnt));
    cbw->bmCBWFlags = 0x80;		/* IN */
    cbw->bCBWLUN = 0;
    cbw->bCBWCBLength = 10;
    cbw->CBWCB[0] = 0x28;		/* READ */
    cbw->CBWCB[1] = 0;
    cbw->CBWCB[2] = (sectornum >> 24) & 0xFF;	/* LUN 0 & MSB's of sector */
    cbw->CBWCB[3] = (sectornum >> 16) & 0xFF;
    cbw->CBWCB[4] = (sectornum >>  8) & 0xFF;
    cbw->CBWCB[5] = (sectornum >>  0) & 0xFF;
    cbw->CBWCB[6] = 0;
    cbw->CBWCB[7] = 0;
    cbw->CBWCB[8] = seccnt;
    cbw->CBWCB[9] = 0;

    softc->umass_curtag++;

    /*
     * Send the CBW 
     */

    res = usb_make_sync_request(dev,softc->umass_outpipe,(uint8_t *) cbw,
				sizeof(usbmass_cbw_t),UR_FLAG_OUT);
    if (res == 4) {
	usbmass_stall_recovery(dev);
	KFREE(cbwcsw);
	return -1;
	}


    /*
     * Get the data
     */

    res = usb_make_sync_request(dev,softc->umass_inpipe,sector,
				512*seccnt,UR_FLAG_IN | UR_FLAG_SHORTOK);
    if (res == 4) {
	usbmass_stall_recovery(dev);
	KFREE(cbwcsw);
	return -1;
	}


    /*
     * Get the Status
     */

    memset(csw,0,sizeof(usbmass_csw_t));
    res = usb_make_sync_request(dev,softc->umass_inpipe,(uint8_t *) csw,
				sizeof(usbmass_csw_t),UR_FLAG_IN);
    if (res == 4) {
	usbmass_stall_recovery(dev);
	KFREE(cbwcsw);
	return -1;
	}


#if 0
    printf("CSW: Signature=%08X  Tag=%08X  Residue=%08X  Status=%02X\n",
	   GETCBWFIELD(csw,dCSWSignature),
	   GETCBWFIELD(csw,dCSWTag),
	   GETCBWFIELD(csw,dCSWDataResidue),
	   csw->bCSWStatus);
#endif

    res = (csw->bCSWStatus == USBMASS_CSW_PASS) ? 0 : -1;

    KFREE(cbwcsw);

    return res;

}

/*  *********************************************************************
    *  usbmass_write_sector(dev,sectornum,seccnt,buffer)
    *  
    *  Writes a sector to the device
    *  
    *  Input parameters: 
    *  	   dev - usb device
    *  	   sectornum - sector number to write
    * 	   seccnt - count of sectors to write
    *  	   buffer - place to get sector to write
    *  	   
    *  Return value:
    *  	   status
    ********************************************************************* */

static int usbmass_write_sector(usbdev_t *dev,uint32_t sectornum,uint32_t seccnt,
				hsaddr_t buffer)
{
    uint8_t *cbwcsw;
    uint8_t *sector;
    usbmass_cbw_t *cbw;
    usbmass_csw_t *csw;
    usbmass_softc_t *softc;
    int res;

    softc = (usbmass_softc_t *) dev->ud_private;

    cbwcsw = KMALLOC(64,32);
    sector = (uint8_t *) HSADDR2PTR(buffer);		/* XXX not 64-bit compatible */

    cbw = (usbmass_cbw_t *) cbwcsw;
    csw = (usbmass_csw_t *) cbwcsw;

    /*
     * Fill in the fields of the CBW
     */

    PUTCBWFIELD(cbw,dCBWSignature,USBMASS_CBW_SIGNATURE);
    PUTCBWFIELD(cbw,dCBWTag,softc->umass_curtag);
    PUTCBWFIELD(cbw,dCBWDataTransferLength,(512*seccnt));
    cbw->bmCBWFlags = 0x00;		/* OUT */
    cbw->bCBWLUN = 0;
    cbw->bCBWCBLength = 10;
    cbw->CBWCB[0] = 0x2A;		/* WRITE */
    cbw->CBWCB[1] = 0;
    cbw->CBWCB[2] = (sectornum >> 24) & 0xFF;	/* LUN 0 & MSB's of sector */
    cbw->CBWCB[3] = (sectornum >> 16) & 0xFF;
    cbw->CBWCB[4] = (sectornum >>  8) & 0xFF;
    cbw->CBWCB[5] = (sectornum >>  0) & 0xFF;
    cbw->CBWCB[6] = 0;
    cbw->CBWCB[7] = 0;
    cbw->CBWCB[8] = seccnt;
    cbw->CBWCB[9] = 0;

    softc->umass_curtag++;

    /*
     * Send the CBW 
     */

    res = usb_make_sync_request(dev,softc->umass_outpipe,(uint8_t *) cbw,
				sizeof(usbmass_cbw_t),UR_FLAG_OUT);

    /*
     * Send the data
     */

    res = usb_make_sync_request(dev,softc->umass_outpipe,sector,
				512*seccnt,UR_FLAG_OUT);

    /*
     * Get the Status
     */

    memset(csw,0,sizeof(usbmass_csw_t));
    res = usb_make_sync_request(dev,softc->umass_inpipe,(uint8_t *) csw,
				sizeof(usbmass_csw_t),UR_FLAG_IN);

#if 0
    printf("CSW: Signature=%08X  Tag=%08X  Residue=%08X  Status=%02X\n",
	   GETCBWFIELD(csw,dCSWSignature),
	   GETCBWFIELD(csw,dCSWTag),
	   GETCBWFIELD(csw,dCSWDataResidue),
	   csw->bCSWStatus);
#endif

    res = (csw->bCSWStatus == USBMASS_CSW_PASS) ? 0 : -1;

    KFREE(cbwcsw);

    return res;
}

/*  *********************************************************************
    *  usbmass_read_capacity(dev,sectornum,buffer)
    *  
    *  Reads a sector from the device.
    *  
    *  Input parameters: 
    *  	   dev - usb device
    *  	   sectornum - sector number to read
    *  	   buffer - place to put sector we read
    *  	   
    *  Return value:
    *  	   status
    ********************************************************************* */

int usbmass_read_capacity(usbdev_t *dev,uint32_t *size);
int usbmass_read_capacity(usbdev_t *dev,uint32_t *size)
{
    uint8_t *cbwcsw;
    uint8_t *sector;
    usbmass_cbw_t *cbw;
    usbmass_csw_t *csw;
    usbmass_softc_t *softc;
    int res;

    softc = (usbmass_softc_t *) dev->ud_private;

    cbwcsw = KMALLOC(64,32);
    sector = KMALLOC(64,32);

    memset(sector,0,64);

    cbw = (usbmass_cbw_t *) cbwcsw;
    csw = (usbmass_csw_t *) cbwcsw;

    *size = 0;

    /*
     * Fill in the fields of the CBW
     */

    PUTCBWFIELD(cbw,dCBWSignature,USBMASS_CBW_SIGNATURE);
    PUTCBWFIELD(cbw,dCBWTag,softc->umass_curtag);
    PUTCBWFIELD(cbw,dCBWDataTransferLength,8);
    cbw->bmCBWFlags = 0x80;		/* IN */
    cbw->bCBWLUN = 0;
    cbw->bCBWCBLength = 10;
    cbw->CBWCB[0] = 0x25;		/* READ CAPACITY */
    cbw->CBWCB[1] = 0;
    cbw->CBWCB[2] = 0;
    cbw->CBWCB[3] = 0;
    cbw->CBWCB[4] = 0;
    cbw->CBWCB[5] = 0;
    cbw->CBWCB[6] = 0;
    cbw->CBWCB[7] = 0;
    cbw->CBWCB[8] = 0;
    cbw->CBWCB[9] = 0;

    softc->umass_curtag++;

    /*
     * Send the CBW 
     */

    res = usb_make_sync_request(dev,softc->umass_outpipe,(uint8_t *) cbw,
				sizeof(usbmass_cbw_t),UR_FLAG_OUT);
    if (res == 4) {
	usbmass_stall_recovery(dev);
	KFREE(cbwcsw);
	KFREE(sector);
	return -1;
	}

    /*
     * Get the data
     */

    res = usb_make_sync_request(dev,softc->umass_inpipe,sector,
				8,UR_FLAG_IN | UR_FLAG_SHORTOK);
    if (res == 4) {
	usbmass_stall_recovery(dev);
	KFREE(cbwcsw);
	KFREE(sector);
	return -1;
	}

    /*
     * Get the Status
     */

    memset(csw,0,sizeof(usbmass_csw_t));
    res = usb_make_sync_request(dev,softc->umass_inpipe,(uint8_t *) csw,
				sizeof(usbmass_csw_t),UR_FLAG_IN);

    KFREE(cbwcsw);

    *size = (((uint32_t) sector[0]) << 24) |
	(((uint32_t) sector[1]) << 16) |
	(((uint32_t) sector[2]) << 8) |
	(((uint32_t) sector[3]) << 0);

    KFREE(sector);

    return 0;

}

#include <diskio/diskio.h>

//========================================================
int usbmass_read(struct bdev *dev, void *data, lba_t lba, int num);

struct bdev_ops usbmass_ops =
{
	usbmass_read  //.read: 
};

static int usbmass_attach(usbdev_t *dev,usb_driver_t *drv)
{
    usb_config_descr_t *cfgdscr = dev->ud_cfgdescr;
    usb_endpoint_descr_t *epdscr;
    usb_endpoint_descr_t *indscr = NULL;
    usb_endpoint_descr_t *outdscr = NULL;
    usb_interface_descr_t *ifdscr;
    usbmass_softc_t *softc;
    int idx;
    dev->ud_drv = drv;

    softc = KMALLOC(sizeof(usbmass_softc_t),0);
    memset(softc,0,sizeof(usbmass_softc_t));
    dev->ud_private = softc;

    ifdscr = usb_find_cfg_descr(dev,USB_INTERFACE_DESCRIPTOR_TYPE,0);
    if (ifdscr == NULL) {
	return -1;
	}

    if ((ifdscr->bInterfaceSubClass != USBMASS_SUBCLASS_SCSI) ||
	(ifdscr->bInterfaceProtocol != USBMASS_BULKONLY_PROTOCOL)) {
	console_log("USBMASS: Do not understand devices with SubClass 0x%02X, Protocol 0x%02X",
		    ifdscr->bInterfaceSubClass,
		    ifdscr->bInterfaceProtocol);
	return -1;
	}

    for (idx = 0; idx < 2; idx++) {
	epdscr = usb_find_cfg_descr(dev,USB_ENDPOINT_DESCRIPTOR_TYPE,idx);

	if (USB_ENDPOINT_DIR_OUT(epdscr->bEndpointAddress)) {
	    outdscr = epdscr;
	    }
	else {
	    indscr = epdscr;
	    }
	}


    if (!indscr || !outdscr) {
	/*
	 * Could not get descriptors, something is very wrong.
	 * Leave device addressed but not configured.
	 */
	return -1;
	}

    /*
     * Choose the standard configuration.
     */

    usb_set_configuration(dev,cfgdscr->bConfigurationValue);

    /*
     * Open the pipes.
     */

    softc->umass_inpipe     = usb_open_pipe(dev,indscr);
    softc->umass_outpipe    = usb_open_pipe(dev,outdscr);
    softc->umass_curtag     = 0x12345678;
    softc->dev = dev;

    /*
     * Save pointer in global unit table so we can
     * match CFE devices up with USB ones
     */

    // = 0;
	//num = 0;
 //   char name[10];
 //   sprintf(name, "ud%c", 'a' + num); num++; num %= 26;
 //   softc->bdev = register_bdev(softc, &usbmass_ops, name);

 //   usbmass_dev = dev;

    return 0;
}

static int usbmass_detach(usbdev_t *dev)
{
    usbmass_softc_t *softc;
    softc = (usbmass_softc_t *) dev->ud_private;

    unregister_bdev(softc->bdev);

    KFREE(softc);
    return 0;
}

int usbmass_read(struct bdev *dev, void *data, lba_t lba, int num)
{
	int r = 0;
	int tl = 0;
	usbmass_softc_t *softc = dev->ctx;
	lba += dev->offset;
	
	while (num)
	{
		tl = num;
		if (usbmass_read_sector(softc->dev, lba, num, PTR2HSADDR(data)))
			break;
		//data += tl;
		(char*) data += tl;
		num -= tl;
		r += tl;
	}
	
	return r;
}