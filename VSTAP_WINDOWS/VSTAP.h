/*
 * OnVif.h
 *
 *  Created on: 02-Aug-2017
 *      Author: arulsankar
 */

#ifndef ONVIF_H_
#define ONVIF_H_

#include "sys_inc.h"
#include "onvif.h"
#include "json.h"

//PPSN_CTX * m_dev_fl;    // device free list
//PPSN_CTX * m_dev_ul;    // device used list
void clearDevices();
typedef struct
{
	ONVIF_DEVICE	onvif_device;

	int				flags;      				// FLAG_MANUAL mean added manual, other auto discovery device
	int				state;      				// 0 -- offline; 1 -- online
	int				no_res_nums;				// probe not response numbers, when > 2 means offline

	void          * p_user;						// user data

    pthread_t		thread_handler;				// get information thread handler
	BOOL			need_update;				// if update device information

	int				snapshot_len;               // devcie snapshot buffer length
	unsigned char * p_snapshot;                 // device snapshot buffer
} ONVIF_DEVICE_EX;

void StartVstap();
void StopVstap();
void RefreshVstap();
bool VstapOnvifCameras();
bool vstapCameraIndividual(const char* Camera_IP, const char* Camera_Port, const char* Camera_Username, const char* Camera_Password);
//BOOL OnvifPTZactions();
bool CameraAction(const char* Camera_IP, const char* Camera_Port, const char* Camera_Username, const char* Camera_Password, const char* PTZoomReset, const char* ResolutionHeight, const char* ResolutionWidth, const char* New_Username, const char* New_Password);



#endif /* ONVIF_H_ */
