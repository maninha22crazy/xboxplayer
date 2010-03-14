#include "smc.h"
#include <string.h>

void smc::PrepareBuffers()
{
 	//Zero our mem
	ZeroMemory( m_SMCMessage, sizeof(m_SMCMessage) );
	ZeroMemory( m_SMCReturn, sizeof(m_SMCReturn) );
}

//Usage: command is one of the POWER_LED constants from smc_constant.h
//         animate is true for ring LED startup light sequence
void smc::SetPowerLED(unsigned char command, bool animate)
{
	PrepareBuffers();
    m_SMCMessage[0] = 0x8c;
    m_SMCMessage[1] = command;
    m_SMCMessage[2] = (animate ? 0x01 : 0x00);

    HalSendSMCMessage(m_SMCMessage, NULL);
}


//Usage: color is one of LED constants from smc_constant.h
void smc::SetLEDS(LEDState s1, LEDState s2, LEDState s3, LEDState s4)
{
	PrepareBuffers();
    m_SMCMessage[0] = 0x99;
    m_SMCMessage[1] = 0x01;
    m_SMCMessage[2] = ((s1>>3) | (s2>>2) | (s3>>1) | (s4));    //Thanks unknown v2
    HalSendSMCMessage(m_SMCMessage, NULL);
}

// cpugpu:0-cpu;1-gpu


void smc::SetFanAlgorithm(int algorithm)
{
	PrepareBuffers();
    m_SMCMessage[0] = 0x88;
    m_SMCMessage[1] = algorithm;
    HalSendSMCMessage(m_SMCMessage, NULL);
}

//Usage:  temps contains the returned temperature values
//          temps[0] = CPU
//          temps[1] = GPU
//          temps[2] = EDRAM
//          temps[3] = MB
void smc::GetTemps(float *temps, bool celsius)
{
    int i;
    PrepareBuffers();
    m_SMCMessage[0] = REQUEST_TEMP;
    HalSendSMCMessage(m_SMCMessage, m_SMCReturn);

    HalSendSMCMessage(m_SMCMessage, m_SMCReturn);

    for(i=0; i<4; i++)
        temps[i] = (m_SMCReturn[i * 2 + 1] | (m_SMCReturn[i * 2 +2] <<8)) / 256.0;        

    if(!celsius)
        for(i=0; i<4; i++)
            temps[i] = (9/5) * temps[i] + 32;
}

void smc::AudioEnable(BYTE enable)
{
	PrepareBuffers();
    m_SMCMessage[0] = 0x8D;
    m_SMCMessage[1] = enable & 1;
    HalSendSMCMessage(m_SMCMessage, m_SMCReturn);
    return;
}

void smc::Shutdown()
{
	PrepareBuffers();
    m_SMCMessage[0] = 0x82;
	m_SMCMessage[1] = 0x11;
	m_SMCMessage[2] = 0x01;

	HalSendSMCMessage(m_SMCMessage, m_SMCReturn);
}

char* smc::GetSMCVersion()
{
	PrepareBuffers();
    m_SMCMessage[0] = REQUEST_SMC_VERSION;
    HalSendSMCMessage(m_SMCMessage, m_SMCReturn);
	
	static char version[5] = "";
	sprintf_s(version, sizeof(version), "%d.%d", m_SMCReturn[2], m_SMCReturn[3]);

    return version;
}

TILT_STATE smc::GetTiltState()
{
	PrepareBuffers();
    m_SMCMessage[0] = REQUEST_TILT;
    HalSendSMCMessage(m_SMCMessage, m_SMCReturn);
	
	TILT_STATE sTiltState = ((m_SMCReturn[1] & 1) ? VERTICAL : HORIZONTAL);
	return sTiltState;
}

unsigned char smc::GetAVPack()
{
	//MATTIE: names for the types of av packs
	PrepareBuffers();
    m_SMCMessage[0] = REQUEST_AV_PACK;
	HalSendSMCMessage(m_SMCMessage, m_SMCReturn);
	
	return m_SMCReturn[1];
}

TRAY_STATE smc::GetTrayState()
{
	PrepareBuffers();
    m_SMCMessage[0] = REQUEST_TRAY_STATE;
	HalSendSMCMessage(m_SMCMessage, m_SMCReturn);
	
	return (TRAY_STATE)((m_SMCReturn[1] & 0xF) % 5);
}
void smc::OpenTray()
{
	PrepareBuffers();
    m_SMCMessage[0] = 0x8b;
	m_SMCMessage[1] = 0x60;
	HalSendSMCMessage(m_SMCMessage, NULL);
	
}
void smc::CloseTray()
{
	PrepareBuffers();
    m_SMCMessage[0] = 0x8b;
	m_SMCMessage[1] = 0x62;
	HalSendSMCMessage(m_SMCMessage, NULL);
	
}

void smc::SetFanSpeed(int fan, int speed)
{
	PrepareBuffers();
	m_SMCMessage[0] = fan ? 0x94 : 0x89;
	m_SMCMessage[1] = (unsigned char)speed | 0x80;

	HalSendSMCMessage(m_SMCMessage, NULL);
}

// Thanks goto Aaron for this
// Setting Names to AV Packs
const char* smc::GetAVPackName()
{
	switch (GetAVPack()) {
		case AV_HDMI: return "HDMI";
		case AV_COMPONENT: return "Component";
		case AV_VGA: return "VGA";
		case AV_COMPOSITE: return "Composite";
		default: return NULL;
	}
}
