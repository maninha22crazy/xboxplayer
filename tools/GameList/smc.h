//Thanks to tmbinc for smc.c
#pragma once
#ifndef _SMC_H
#define _SMC_H

#include <stdafx.h>

//sorry for the extreme amount of constants trying to make functions
//easier to use

//Power LED
#define POWER_LED_BLINK            0x10
#define POWER_LED_DEFAULT        0x02
#define POWER_LED_ON            0x01
#define POWER_LED_OFF            0X03

//Quadrant LEDs
//NOTE: LED constants are determined with console laying down
//        with LED color bits being, starting from tope left 1, 2, 4, 8

//Thanks to unknown v2 for the following
typedef enum _LEDState
{
    OFF        = 0x00,
    RED        = 0x08,
    GREEN    = 0x80,
    ORANGE    = 0x88
}LEDState;

typedef enum _TEMP_INDEX
{
    CPU,
    GPU,
    MEM,
    BRD
}TEMP_INDEX;

typedef enum _TILT_STATE
{
    VERTICAL,
	HORIZONTAL
}TILT_STATE;

/* MATTIE: WRONG INFO?
typedef enum _TRAY_STATE
{
	OPEN,
	OPENING,
	CLOSED,
	CLOSING,
	PUSHED
}TRAY_STATE;
*/

typedef enum _TRAY_STATE
{
	OPEN,
	UNKNOWN,
	CLOSED,
	OPENING,
	CLOSING
}TRAY_STATE;

// Thanks goto Aaron for this
// Setting Names to AV Packs
typedef enum _AV_PACK {
	AV_HDMI = 0x1F,
	AV_COMPONENT = 0x0F,
	AV_VGA = 0x5B,
	AV_COMPOSITE = 0x43
} AV_PACK;


#define REQUEST_TEMP		0x07	//SMC message to request temperature 
#define REQUEST_TILT		0x17	//SMC message to request tilt
#define REQUEST_SMC_VERSION	0x12	//SMC message to request the smc version
#define	REQUEST_TRAY_STATE	0x0A	//SMC message to request the tray state
#define	REQUEST_AV_PACK		0x0F	//SMC message to request the AV-Pack type


//Thanks to tmbinc for smc.c


//Call to SMC message function in xboxkrnl.lib
extern "C" void __stdcall HalSendSMCMessage(void* input, void* output);

class smc
{
    public:
        void SetPowerLED(unsigned char command, bool animate);
        void SetLEDS(LEDState s1, LEDState s2, LEDState s3, LEDState s4);    //Thanks unknown v2
        void GetTemps(float *temps, bool celsius);
        void SetFanSpeed(int cpugpu, int speed);
        void SetFanAlgorithm(int algorithm);

        void AudioEnable(BYTE enable);
        void Shutdown();
		char* GetSMCVersion();
		TILT_STATE GetTiltState();
		unsigned char GetAVPack();
		const char* GetAVPackName(); // Thanks goto Aaron for this
		TRAY_STATE GetTrayState();
		void OpenTray();
		void CloseTray();
    private:
		//This is used to store our SMC data
		unsigned char m_SMCMessage[16];
		unsigned char m_SMCReturn[16];
		void PrepareBuffers();
};

#endif