/***************************************************************************************
*
*  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
*
*  By downloading, copying, installing or using the software you agree to this license.
*  If you do not agree to this license, do not download, install,
*  copy or use the software.
*
*  Copyright (C) 2010-2017, Happytimesoft Corporation, all rights reserved.
*
*  Redistribution and use in binary forms, with or without modification, are permitted.
*
*  Unless required by applicable law or agreed to in writing, software distributed
*  under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
*  CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
*  language governing permissions and limitations under the License.
*
****************************************************************************************/

#include "sys_inc.h"
#include "http.h"
#include "onvif.h"
#include "onvif_probe.h"
#include "onvif_event.h"
#include "onvif_api.h"
#include "VSTAP.h"

#include <iostream>
#include <fstream>
#include "json.h"


Json::Value event;
Json::Value event_ind;
Json::Value event_act;
Json::Value event_camera;
Json::Value event_camera_ind;
Json::Value event_camera_action;
Json::StyledWriter styledWriter;
Json::FastWriter fastWriter;
std::string camera_output;
char str_nodes[256];
int camera_count = 0;
std::ofstream file_log;

BOOL StartOnVifThread = FALSE;
BOOL DidCameraFound = FALSE;
int CameraFound = 0;
//extern int g_probe_count;
extern "C" __declspec(dllimport) int g_probe_count;
//extern __declspec(selectany) int g_probe_count;
/***************************************************************************************/
PPSN_CTX * m_dev_fl;    // device free list
PPSN_CTX * m_dev_ul;    // device used list

						/***************************************************************************************/
#ifdef WIN32
void initWinSock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}
#endif

ONVIF_DEVICE_EX * findDevice(ONVIF_DEVICE_EX * pdevice)
{
	ONVIF_DEVICE_EX * p_dev = (ONVIF_DEVICE_EX *)pps_lookup_start(m_dev_ul);
	while (p_dev)
	{
		if (strcmp(p_dev->onvif_device.binfo.XAddr.host, pdevice->onvif_device.binfo.XAddr.host) == 0 &&
			p_dev->onvif_device.binfo.XAddr.port == pdevice->onvif_device.binfo.XAddr.port)
		{
			onvif_SetAuthInfo(&p_dev->onvif_device, "", "");
			GetCapabilities(&p_dev->onvif_device);
			break;
		}

		p_dev = (ONVIF_DEVICE_EX *)pps_lookup_next(m_dev_ul, p_dev);
	}

	pps_lookup_end(m_dev_ul);

	return p_dev;
}

//BOOL CreateUser(ONVIF_DEVICE_EX * p_dev) {
//	CreateUsers_REQ req;
//
//	memset(&req, 0, sizeof(req));
//
//	strcpy(req.User.Username, "quikav");
//	req.User.PasswordFlag = 1;
//	strcpy(req.User.Password, "quikav");
//	req.User.UserLevel = UserLevel_User;
//
//	if (onvif_CreateUsers(&p_dev->onvif_device, &req, NULL)) {
//		fprintf(stderr, "Create users successfully\n");
//	}
//	else {
//		fprintf(stderr, "Create users failed\n");
//	}
//
//	onvif_SetAuthInfo(&p_dev->onvif_device, "quikav", "quikav");
//	return TRUE;
//}
//
//BOOL ChangePassword(ONVIF_DEVICE_EX * p_dev) {
//	//	GetCapabilities(&p_dev->onvif_device);
//	//	GetDeviceInformation(&p_dev->onvif_device);
//	//	GetUsers_RES user;
//	//	memset(&user, 0, sizeof(user));
//	//	if (onvif_GetUsers(&p_dev->onvif_device, NULL, &user)) {
//	//	} else {
//	//	}
//	//if (GetDeviceInformation(&p_dev->onvif_device)) {
//	//	event["firmwareVersion"] = p_dev->onvif_device.DeviceInformation.FirmwareVersion;
//	//	event["hardwareId"] = p_dev->onvif_device.DeviceInformation.HardwareId;
//	//	event["manufacturer"] = p_dev->onvif_device.DeviceInformation.Manufacturer;
//	//	event["model"] = p_dev->onvif_device.DeviceInformation.Model;
//	//	event["serialNumber"] = p_dev->onvif_device.DeviceInformation.SerialNumber;
//	//}
//	SetUser_REQ req;
//	memset(&req, 0, sizeof(req));
//	strcpy(req.User.Username, "admin");
//	strcpy(req.User.Password, "admin");
//	req.User.PasswordFlag = 1;
//	req.User.UserLevel = UserLevel_Administrator;
//
//	SetUser_RES res;
//	if (onvif_SetUser(&p_dev->onvif_device, &req, &res)) {
//		onvif_SetAuthInfo(&p_dev->onvif_device, "admin", "admin");
//		fprintf(stderr, "Change password successfully\n");
//		GetCapabilities(&p_dev->onvif_device);
//		if ((p_dev->onvif_device.authFailed)) {
//			return FALSE;
//		}
//		else {
//			return TRUE;
//		}
//	}
//	else {
//		onvif_SetAuthInfo(&p_dev->onvif_device, "admin", "admin");
//		fprintf(stderr, "Change password failed\n");
//		return FALSE;
//	}
//
//}


ONVIF_DEVICE_EX * addDevice(ONVIF_DEVICE_EX * pdevice)
{
	ONVIF_DEVICE_EX * p_dev = (ONVIF_DEVICE_EX *)ppstack_fl_pop(m_dev_fl);
	if (p_dev)
	{
		memcpy(p_dev, pdevice, sizeof(ONVIF_DEVICE_EX));
		p_dev->p_user = 0;
		p_dev->onvif_device.events.init_term_time = 60;
		printf("Add devices now\n");
		// set device login information
		onvif_SetAuthInfo(&p_dev->onvif_device, "", "");

		pps_ctx_ul_add(m_dev_ul, p_dev);
	}
	return p_dev;
}

void onvifUserTest(ONVIF_DEVICE_EX * p_dev)
{
	BOOL ret;
	char username[32];
	CreateUsers_REQ req;

	memset(&req, 0, sizeof(req));

	sprintf(username, "testuser%d", rand() % 1000);

	strcpy(req.User.Username, username);
	req.User.PasswordFlag = 1;
	strcpy(req.User.Password, "testpass");
	req.User.UserLevel = UserLevel_Administrator;

	ret = onvif_CreateUsers(&p_dev->onvif_device, &req, NULL);

	printf("onvif_CreateUsers return ret = %d\n", ret);

	if (!ret)
	{
		return;
	}

	onvif_SetAuthInfo(&p_dev->onvif_device, username, "testpass");

	GetNetworkProtocols_REQ req1;
	GetNetworkProtocols_RES res1;

	memset(&req1, 0, sizeof(req1));
	memset(&res1, 0, sizeof(res1));

	ret = onvif_GetNetworkProtocols(&p_dev->onvif_device, &req1, &res1);

	printf("onvif_GetNetworkProtocols return ret = %d\n", ret);

}

void onvifImageTest(ONVIF_DEVICE_EX * p_dev)
{
	BOOL ret;

	while (1)
	{
		// get video sources
		GetVideoSources_RES res;
		memset(&res, 0, sizeof(res));

		ret = onvif_GetVideoSources(&p_dev->onvif_device, NULL, &res);

		printf("onvif_GetVideoSources return ret = %d\n", ret);

		if (!ret || NULL == res.VideoSources)
		{
			break;
		}

		// get move options
		img_GetMoveOptions_REQ req1;
		img_GetMoveOptions_RES res1;

		memset(&req1, 0, sizeof(req1));
		memset(&res1, 0, sizeof(res1));

		strcpy(req1.VideoSourceToken, res.VideoSources->VideoSource.token);

		ret = onvif_img_GetMoveOptions(&p_dev->onvif_device, &req1, &res1);

		printf("onvif_img_GetMoveOptions return ret = %d\n", ret);

		if (ret && res1.MoveOptions.ContinuousFlag)
		{
			// move
			img_Move_REQ req2;
			memset(&req2, 0, sizeof(req2));

			strcpy(req2.VideoSourceToken, res.VideoSources->VideoSource.token);
			req2.Focus.ContinuousFlag = 1;
			req2.Focus.Continuous.Speed = res1.MoveOptions.Continuous.Speed.Min;

			ret = onvif_img_Move(&p_dev->onvif_device, &req2, NULL);

			printf("onvif_img_Move return ret = %d\n", ret);

			// stop
			img_Stop_REQ req3;
			memset(&req3, 0, sizeof(req3));

			strcpy(req3.VideoSourceToken, res.VideoSources->VideoSource.token);

			ret = onvif_img_Stop(&p_dev->onvif_device, &req3, NULL);

			printf("onvif_img_Stop return ret = %d\n", ret);
		}

		onvif_free_VideoSources(&res.VideoSources);

		break;
	}
}

void onvifMedia2Test(ONVIF_DEVICE_EX * p_dev)
{
	BOOL ret;

	while (1)
	{
		tr2_GetAudioEncoderConfigurationOptions_REQ req;
		tr2_GetAudioEncoderConfigurationOptions_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetAudioEncoderConfigurationOptions(&p_dev->onvif_device, &req, &res);
		if (ret)
		{
			onvif_free_AudioEncoder2ConfigurationOptions(&res.Options);
		}

		printf("onvif_tr2_GetAudioEncoderConfigurationOptions return ret = %d\n", ret);

		break;
	}

	while (1)
	{
		tr2_GetAudioEncoderConfigurations_REQ req;
		tr2_GetAudioEncoderConfigurations_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetAudioEncoderConfigurations(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_GetAudioEncoderConfigurations return ret = %d\n", ret);

		if (!ret || NULL == res.Configurations)
		{
			break;
		}

		tr2_SetAudioEncoderConfiguration_REQ req1;
		tr2_SetAudioEncoderConfiguration_RES res1;

		memset(&req1, 0, sizeof(req1));
		memset(&res1, 0, sizeof(res1));

		memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_AudioEncoder2Configuration));

		ret = onvif_tr2_SetAudioEncoderConfiguration(&p_dev->onvif_device, &req1, &res1);

		printf("onvif_tr2_SetAudioEncoderConfiguration return ret = %d\n", ret);

		onvif_free_AudioEncoder2Configurations(&res.Configurations);

		break;
	}

	while (1)
	{
		tr2_GetAudioSourceConfigurationOptions_REQ req;
		tr2_GetAudioSourceConfigurationOptions_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetAudioSourceConfigurationOptions(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_GetAudioSourceConfigurationOptions return ret = %d\n", ret);

		break;
	}

	while (1)
	{
		tr2_GetAudioSourceConfigurations_REQ req;
		tr2_GetAudioSourceConfigurations_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetAudioSourceConfigurations(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_GetAudioSourceConfigurations return ret = %d\n", ret);

		if (!ret || NULL == res.Configurations)
		{
			break;
		}

		tr2_SetAudioSourceConfiguration_REQ req1;
		tr2_SetAudioSourceConfiguration_RES res1;

		memset(&req1, 0, sizeof(req1));
		memset(&res1, 0, sizeof(res1));

		memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_AudioSourceConfiguration));

		ret = onvif_tr2_SetAudioSourceConfiguration(&p_dev->onvif_device, &req1, &res1);

		printf("onvif_tr2_SetAudioSourceConfiguration return ret = %d\n", ret);

		onvif_free_AudioSourceConfigurations(&res.Configurations);

		break;
	}

	while (1)
	{
		tr2_GetVideoEncoderConfigurationOptions_REQ req;
		tr2_GetVideoEncoderConfigurationOptions_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetVideoEncoderConfigurationOptions(&p_dev->onvif_device, &req, &res);
		if (ret)
		{
			onvif_free_VideoEncoder2ConfigurationOptions(&res.Options);
		}

		printf("onvif_tr2_GetVideoEncoderConfigurationOptions return ret = %d\n", ret);

		break;
	}

	while (1)
	{
		tr2_GetVideoEncoderConfigurations_REQ req;
		tr2_GetVideoEncoderConfigurations_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetVideoEncoderConfigurations(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_GetVideoEncoderConfigurations return ret = %d\n", ret);

		if (!ret || NULL == res.Configurations)
		{
			break;
		}

		tr2_SetVideoEncoderConfiguration_REQ req1;
		tr2_SetVideoEncoderConfiguration_RES res1;

		memset(&req1, 0, sizeof(req1));
		memset(&res1, 0, sizeof(res1));

		memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_VideoEncoder2Configuration));

		ret = onvif_tr2_SetVideoEncoderConfiguration(&p_dev->onvif_device, &req1, &res1);

		printf("onvif_tr2_SetVideoEncoderConfiguration return ret = %d\n", ret);

		onvif_free_VideoEncoder2Configurations(&res.Configurations);

		break;
	}

	while (1)
	{
		tr2_GetVideoSourceConfigurationOptions_REQ req;
		tr2_GetVideoSourceConfigurationOptions_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetVideoSourceConfigurationOptions(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_GetVideoSourceConfigurationOptions return ret = %d\n", ret);

		break;
	}

	while (1)
	{
		tr2_GetVideoSourceConfigurations_REQ req;
		tr2_GetVideoSourceConfigurations_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetVideoSourceConfigurations(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_GetVideoSourceConfigurations return ret = %d\n", ret);

		if (!ret || NULL == res.Configurations)
		{
			break;
		}

		tr2_SetVideoSourceConfiguration_REQ req1;
		tr2_SetVideoSourceConfiguration_RES res1;

		memset(&req1, 0, sizeof(req1));
		memset(&res1, 0, sizeof(res1));

		memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_VideoSourceConfiguration));

		ret = onvif_tr2_SetVideoSourceConfiguration(&p_dev->onvif_device, &req1, &res1);

		printf("onvif_tr2_SetVideoSourceConfiguration return ret = %d\n", ret);

		tr2_GetVideoEncoderInstances_REQ req2;
		tr2_GetVideoEncoderInstances_RES res2;

		memset(&req2, 0, sizeof(req2));
		memset(&res2, 0, sizeof(res2));

		strcpy(req2.ConfigurationToken, res.Configurations->Configuration.token);

		ret = onvif_tr2_GetVideoEncoderInstances(&p_dev->onvif_device, &req2, &res2);

		printf("onvif_tr2_GetVideoEncoderInstances return ret = %d\n", ret);

		onvif_free_VideoSourceConfigurations(&res.Configurations);

		break;
	}

	{
		tr2_GetMetadataConfigurationOptions_REQ req;
		tr2_GetMetadataConfigurationOptions_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetMetadataConfigurationOptions(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_GetMetadataConfigurationOptions return ret = %d\n", ret);
	}

	{
		tr2_GetMetadataConfigurations_REQ req;
		tr2_GetMetadataConfigurations_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetMetadataConfigurations(&p_dev->onvif_device, &req, &res);
		if (ret)
		{
			onvif_free_MetadataConfigurations(&res.Configurations);
		}

		printf("onvif_tr2_GetMetadataConfigurations return ret = %d\n", ret);
	}

	while (1)
	{
		tr2_GetProfiles_REQ req;
		tr2_GetProfiles_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		ret = onvif_tr2_GetProfiles(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_GetProfiles return ret = %d\n", ret);

		if (!ret || NULL == res.Profiles)
		{
			break;
		}

		// onvif_tr2_GetStreamUri
		tr2_GetStreamUri_REQ req1;
		tr2_GetStreamUri_RES res1;

		memset(&req1, 0, sizeof(req1));
		memset(&res1, 0, sizeof(res1));

		strcpy(req1.Protocol, "RtspUnicast");
		strcpy(req1.ProfileToken, res.Profiles->MediaProfile.token);

		ret = onvif_tr2_GetStreamUri(&p_dev->onvif_device, &req1, &res1);

		printf("onvif_tr2_GetStreamUri return ret = %d\n", ret);

		if (ret)
		{
			printf("onvif_tr2_GetStreamUri, profile=%s, uri=%s\n", req1.ProfileToken, res1.Uri);
		}

		// onvif_tr2_SetSynchronizationPoint
		tr2_SetSynchronizationPoint_REQ req2;
		tr2_SetSynchronizationPoint_RES res2;

		memset(&req2, 0, sizeof(req2));
		memset(&res2, 0, sizeof(res2));

		strcpy(req2.ProfileToken, res.Profiles->MediaProfile.token);

		ret = onvif_tr2_SetSynchronizationPoint(&p_dev->onvif_device, &req2, &res2);

		printf("onvif_tr2_SetSynchronizationPoint return ret = %d\n", ret);

		// onvif_tr2_AddConfiguration
		tr2_AddConfiguration_REQ req3;
		tr2_AddConfiguration_RES res3;

		memset(&req3, 0, sizeof(req3));
		memset(&res3, 0, sizeof(res3));

		req3.sizeConfiguration = 1;
		strcpy(req3.Configuration[0].Type, "All");
		strcpy(req3.ProfileToken, res.Profiles->MediaProfile.token);

		ret = onvif_tr2_AddConfiguration(&p_dev->onvif_device, &req3, &res3);

		printf("onvif_tr2_AddConfiguration return ret = %d\n", ret);


		// free profiles
		onvif_free_MediaProfiles(&res.Profiles);

		break;
	}

	while (1)
	{
		tr2_CreateProfile_REQ req;
		tr2_CreateProfile_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		strcpy(req.Name, "test");

		ret = onvif_tr2_CreateProfile(&p_dev->onvif_device, &req, &res);

		printf("onvif_tr2_CreateProfile return ret = %d\n", ret);

		if (ret)
		{
			printf("onvif_tr2_CreateProfile token = %s\n", res.Token);
		}
		else
		{
			break;
		}

		tr2_DeleteProfile_REQ req1;
		tr2_DeleteProfile_RES res1;

		memset(&req1, 0, sizeof(req1));
		memset(&res1, 0, sizeof(res1));

		strcpy(req1.Token, res.Token);

		ret = onvif_tr2_DeleteProfile(&p_dev->onvif_device, &req1, &res1);

		printf("onvif_tr2_DeleteProfile return ret = %d\n", ret);


		break;
	}

}

void onvifRecordingTest(ONVIF_DEVICE_EX * p_dev)
{
	BOOL ret;

	while (p_dev->onvif_device.Capabilities.recording.DynamicRecordings)
	{
		// create recording
		CreateRecording_REQ req;
		CreateRecording_RES res;

		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(res));

		strcpy(req.RecordingConfiguration.Source.SourceId, "test");
		strcpy(req.RecordingConfiguration.Source.Name, "test");
		strcpy(req.RecordingConfiguration.Source.Location, "test");
		strcpy(req.RecordingConfiguration.Source.Description, "test");
		strcpy(req.RecordingConfiguration.Source.Address, "test");
		strcpy(req.RecordingConfiguration.Content, "test");

		ret = onvif_CreateRecording(&p_dev->onvif_device, &req, &res);

		printf("onvif_CreateRecording return ret = %d\n", ret);

		if (!ret)
		{
			break;
		}

		// get recordings
		GetRecordings_RES res1;
		memset(&res1, 0, sizeof(res1));

		ret = onvif_GetRecordings(&p_dev->onvif_device, NULL, &res1);

		printf("onvif_GetRecordings return ret = %d\n", ret);

		if (!ret || NULL == res1.Recordings)
		{
			break;
		}

		// set recording configuration
		SetRecordingConfiguration_REQ req2;
		memset(&req2, 0, sizeof(req2));

		strcpy(req2.RecordingToken, res.RecordingToken);
		memcpy(&req2.RecordingConfiguration, &res1.Recordings->Recording.Configuration, sizeof(onvif_RecordingConfiguration));

		ret = onvif_SetRecordingConfiguration(&p_dev->onvif_device, &req2, NULL);

		printf("onvif_SetRecordingConfiguration return ret = %d\n", ret);

		// get recording configuration
		GetRecordingConfiguration_REQ req3;
		GetRecordingConfiguration_RES res3;

		memset(&req3, 0, sizeof(req3));
		memset(&res3, 0, sizeof(res3));

		strcpy(req3.RecordingToken, res.RecordingToken);

		ret = onvif_GetRecordingConfiguration(&p_dev->onvif_device, &req3, &res3);

		printf("onvif_GetRecordingConfiguration return ret = %d\n", ret);

		// get recording options
		GetRecordingOptions_REQ req4;
		GetRecordingOptions_RES res4;

		memset(&req4, 0, sizeof(req4));
		memset(&res4, 0, sizeof(res4));

		strcpy(req4.RecordingToken, res.RecordingToken);

		ret = onvif_GetRecordingOptions(&p_dev->onvif_device, &req4, &res4);

		printf("onvif_GetRecordingOptions return ret = %d\n", ret);

		// create track
		CreateTrack_REQ req5;
		CreateTrack_RES res5;

		memset(&req5, 0, sizeof(req5));
		memset(&res5, 0, sizeof(res5));

		strcpy(req5.RecordingToken, res.RecordingToken);
		req5.TrackConfiguration.TrackType = TrackType_Video;
		strcpy(req5.TrackConfiguration.Description, "trackdesc");

		ret = onvif_CreateTrack(&p_dev->onvif_device, &req5, &res5);

		printf("onvif_CreateTrack return ret = %d\n", ret);

		if (ret)
		{
			// get track configuration
			GetTrackConfiguration_REQ req6;
			GetTrackConfiguration_RES res6;

			memset(&req6, 0, sizeof(req6));
			memset(&res6, 0, sizeof(res6));

			strcpy(req6.RecordingToken, res.RecordingToken);
			strcpy(req6.TrackToken, res5.TrackToken);

			ret = onvif_GetTrackConfiguration(&p_dev->onvif_device, &req6, &res6);

			printf("onvif_GetTrackConfiguration return ret = %d\n", ret);

			// set track configuration
			SetTrackConfiguration_REQ req7;

			memset(&req7, 0, sizeof(req7));

			strcpy(req7.RecordingToken, res.RecordingToken);
			strcpy(req7.TrackToken, res5.TrackToken);
			memcpy(&req7.TrackConfiguration, &res6.TrackConfiguration, sizeof(onvif_TrackConfiguration));

			ret = onvif_SetTrackConfiguration(&p_dev->onvif_device, &req7, NULL);

			printf("onvif_SetTrackConfiguration return ret = %d\n", ret);

			// delete track
			DeleteTrack_REQ req8;

			memset(&req8, 0, sizeof(req8));

			strcpy(req8.RecordingToken, res.RecordingToken);
			strcpy(req8.TrackToken, res5.TrackToken);

			ret = onvif_DeleteTrack(&p_dev->onvif_device, &req8, NULL);

			printf("onvif_DeleteTrack return ret = %d\n", ret);
		}

		// create recording job
		CreateRecordingJob_REQ req9;
		CreateRecordingJob_RES res9;

		memset(&req9, 0, sizeof(req9));
		memset(&res9, 0, sizeof(res9));

		strcpy(req9.JobConfiguration.RecordingToken, res.RecordingToken);
		strcpy(req9.JobConfiguration.Mode, "Idle");
		req9.JobConfiguration.Priority = 1;

		ret = onvif_CreateRecordingJob(&p_dev->onvif_device, &req9, &res9);

		printf("onvif_CreateRecordingJob return ret = %d\n", ret);

		// get recording jobs
		GetRecordingJobs_RES res10;

		memset(&res10, 0, sizeof(res10));

		ret = onvif_GetRecordingJobs(&p_dev->onvif_device, NULL, &res10);

		printf("onvif_GetRecordingJobs return ret = %d\n", ret);

		if (ret && res10.RecordingJobs)
		{
			onvif_free_RecordingJobs(&res10.RecordingJobs);
		}

		// get recording job configuration
		GetRecordingJobConfiguration_REQ req11;
		GetRecordingJobConfiguration_RES res11;

		memset(&req11, 0, sizeof(req11));
		memset(&res11, 0, sizeof(res11));

		strcpy(req11.JobToken, res9.JobToken);

		ret = onvif_GetRecordingJobConfiguration(&p_dev->onvif_device, &req11, &res11);

		printf("onvif_GetRecordingJobConfiguration return ret = %d\n", ret);

		// set recording job configuration
		SetRecordingJobConfiguration_REQ req12;
		SetRecordingJobConfiguration_RES res12;

		memset(&req12, 0, sizeof(req12));
		memset(&res12, 0, sizeof(res12));

		strcpy(req12.JobToken, res9.JobToken);
		memcpy(&req12.JobConfiguration, &res11.JobConfiguration, sizeof(onvif_RecordingJobConfiguration));

		ret = onvif_SetRecordingJobConfiguration(&p_dev->onvif_device, &req12, &res12);

		printf("onvif_SetRecordingJobConfiguration return ret = %d\n", ret);

		// set recording job mode
		SetRecordingJobMode_REQ req13;

		memset(&req13, 0, sizeof(req13));

		strcpy(req13.JobToken, res9.JobToken);
		strcpy(req13.Mode, "Active");

		ret = onvif_SetRecordingJobMode(&p_dev->onvif_device, &req13, NULL);

		printf("onvif_SetRecordingJobMode return ret = %d\n", ret);

		// get recording job state
		GetRecordingJobState_REQ req14;
		GetRecordingJobState_RES res14;

		memset(&req14, 0, sizeof(req14));
		memset(&res14, 0, sizeof(res14));

		strcpy(req14.JobToken, res9.JobToken);

		ret = onvif_GetRecordingJobState(&p_dev->onvif_device, &req14, &res14);

		printf("onvif_GetRecordingJobState return ret = %d\n", ret);

		// set recording job mode
		SetRecordingJobMode_REQ req15;

		memset(&req15, 0, sizeof(req15));

		strcpy(req15.JobToken, res9.JobToken);
		strcpy(req15.Mode, "Idle");

		ret = onvif_SetRecordingJobMode(&p_dev->onvif_device, &req15, NULL);

		printf("onvif_SetRecordingJobMode return ret = %d\n", ret);

		// delete recording job
		DeleteRecordingJob_REQ req16;

		memset(&req16, 0, sizeof(req16));

		strcpy(req16.JobToken, res9.JobToken);

		ret = onvif_DeleteRecordingJob(&p_dev->onvif_device, &req16, NULL);

		printf("onvif_DeleteRecordingJob return ret = %d\n", ret);

		// delete recording
		DeleteRecording_REQ req17;

		memset(&req17, 0, sizeof(req17));

		strcpy(req17.RecordingToken, res.RecordingToken);

		ret = onvif_DeleteRecording(&p_dev->onvif_device, &req17, NULL);

		printf("onvif_DeleteRecording return ret = %d\n", ret);

		onvif_free_Recordings(&res1.Recordings);

		break;
	}
}

void onvifSystemMaintainTest(ONVIF_DEVICE_EX * p_device)
{
	BOOL ret;

	ret = SystemBackup(&p_device->onvif_device, "onvifsystem.backup");
	printf("SystemBackup return ret = %d\n", ret);

	if (ret)
	{
		//ret = SystemRestore(&p_device->onvif_device, "onvifsystem.backup");
		//printf("SystemRestore return ret = %d\n", ret);
	}

	GetSystemLog_REQ req;
	GetSystemLog_RES res;

	req.LogType = SystemLogType_System;

	ret = onvif_GetSystemLog(&p_device->onvif_device, &req, &res);
	printf("onvif_GetSystemLog return ret = %d\n", ret);

	req.LogType = SystemLogType_Access;

	ret = onvif_GetSystemLog(&p_device->onvif_device, &req, &res);
	printf("onvif_GetSystemLog return ret = %d\n", ret);

}

void * getDevInfoThread(void * argv)
{
	char profileToken[ONVIF_TOKEN_LEN];
	ONVIF_Profile   * p_profile = NULL;
	ONVIF_DEVICE_EX * p_device = (ONVIF_DEVICE_EX *)argv;

	GetSystemDateAndTime(&p_device->onvif_device);

	GetCapabilities(&p_device->onvif_device);

	GetServices(&p_device->onvif_device);

	GetDeviceInformation(&p_device->onvif_device);

	GetVideoSources(&p_device->onvif_device);

	GetImagingSettings(&p_device->onvif_device);

	GetVideoSourceConfigurations(&p_device->onvif_device);

	GetVideoEncoderConfigurations(&p_device->onvif_device);

	if (GetAudioSources(&p_device->onvif_device))
	{
		GetAudioSourceConfigurations(&p_device->onvif_device);

		GetAudioEncoderConfigurations(&p_device->onvif_device);
	}

	///* save currrent profile token */

	if (p_device->onvif_device.curProfile)
	{
		strcpy(profileToken, p_device->onvif_device.curProfile->token);
	}
	else
	{
		memset(profileToken, 0, sizeof(profileToken));
	}

	GetProfiles(&p_device->onvif_device);

	GetStreamUris(&p_device->onvif_device);

	///* resume current profile */
	if (profileToken[0] != '\0')
	{
		p_profile = onvif_find_Profile(&p_device->onvif_device, profileToken);
	}

	if (NULL == p_profile)
	{
		p_profile = p_device->onvif_device.profiles;
	}

	p_device->onvif_device.curProfile = p_profile;

	if (p_device->onvif_device.Capabilities.ptz.support)
	{
		GetNodes(&p_device->onvif_device);

		GetConfigurations(&p_device->onvif_device);
	}

	if (p_device->onvif_device.Capabilities.media.SnapshotUri == 1 && p_profile)
	{
		int len;
		unsigned char * p_buff;

		if (GetSnapshot(&p_device->onvif_device, p_profile->token, &p_buff, &len))
		{
			if (p_device->p_snapshot)
			{
				FreeSnapshotBuff(p_device->p_snapshot);
				p_device->p_snapshot = NULL;
				p_device->snapshot_len = 0;
			}

			p_device->p_snapshot = p_buff;
			p_device->snapshot_len = len;
		}
	}

	if (p_device->onvif_device.Capabilities.events.support == 1 &&
		p_device->onvif_device.events.subscribe == FALSE)
	{
		// the caller should call Unsubscribe when free this device
		Subscribe(&p_device->onvif_device, pps_get_index(m_dev_ul, p_device));
	}

	////onvifUserTest(p_device);

	//if (p_device->onvif_device.Capabilities.image.support)
	//{
	//	onvifImageTest(p_device);
	//}

	//if (p_device->onvif_device.Capabilities.media2.support)
	//{
	//	onvifMedia2Test(p_device);
	//}

	//if (p_device->onvif_device.Capabilities.recording.support)
	//{
	//	onvifRecordingTest(p_device);
	//}
	//onvifSystemMaintainTest(p_device);

	p_device->thread_handler = 0;

	return NULL;
}

ONVIF_DEVICE_EX * findDeviceByNotify(Notify_REQ * p_notify)
{
	int	index = -1;
	ONVIF_DEVICE_EX * p_dev = NULL;

	sscanf(p_notify->PostUrl, "/subscription%d", &index);
	if (index >= 0)
	{
		p_dev = (ONVIF_DEVICE_EX *)pps_get_node_by_index(m_dev_ul, index);
	}

	return p_dev;
}

/**
* onvif device probed callback
*/
void probeCallback(DEVICE_BINFO * p_res, void * p_data)
{
	ONVIF_DEVICE_EX * p_dev = NULL;
	ONVIF_DEVICE_EX device;
	memset(&device, 0, sizeof(ONVIF_DEVICE_EX));

	memcpy(&device.onvif_device.binfo, p_res, sizeof(DEVICE_BINFO));
	device.state = 1;

	DidCameraFound = TRUE;

	p_dev = findDevice(&device);
	if (p_dev == NULL)
	{

		p_dev = addDevice(&device);

		if (p_dev)
		{
			p_dev->thread_handler = sys_os_create_thread((void *)getDevInfoThread, p_dev);
		}
	}
	else
	{
		if (p_dev->no_res_nums >= 2 && 0 == p_dev->thread_handler)
		{
			p_dev->thread_handler = sys_os_create_thread((void *)getDevInfoThread, p_dev);
		}

		p_dev->no_res_nums = 0;
		p_dev->state = 1;
		if (0 == p_dev->thread_handler) {
			StartOnVifThread = TRUE;
			//    		stop_probe();
		}
		//    	StopOnVIF();

	}
}

/**
* onvif event notify callback
*/
void eventNotifyCallback(Notify_REQ * p_req, void * p_data)
{
	ONVIF_DEVICE_EX * p_dev;
	ONVIF_NotificationMessage * p_notify = p_req->notify;

	p_dev = findDeviceByNotify(p_req);
	if (p_dev)
	{
		onvif_device_add_NotificationMessages(&p_dev->onvif_device, p_notify);

		p_dev->onvif_device.events.notify_nums += onvif_get_NotificationMessages_nums(p_notify);
		// max save 100 event notify
		if (p_dev->onvif_device.events.notify_nums > 100)
		{
			p_dev->onvif_device.events.notify_nums -=
				onvif_device_free_NotificationMessages(&p_dev->onvif_device, p_dev->onvif_device.events.notify_nums - 100);
		}
	}
}

/**
* onvif event subscribe disconnect callback
*/
void subscribeDisconnectCallback(ONVIF_DEVICE * p_dev, void * p_data)
{
	Unsubscribe(p_dev);
}

/**
* free device memory
*/
void freeDevice(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev)
	{
		return;
	}

	while (p_dev->thread_handler)
	{
		usleep(1000);
	}

	onvif_free_device(&p_dev->onvif_device);

	if (p_dev->p_snapshot)
	{
		FreeSnapshotBuff(p_dev->p_snapshot);
		p_dev->p_snapshot = NULL;
		p_dev->snapshot_len = 0;
	}
}

void clearDevices()
{
	ONVIF_DEVICE_EX * next_dev;
	ONVIF_DEVICE_EX * dev = (ONVIF_DEVICE_EX *)pps_lookup_start(m_dev_ul);
	while (dev)
	{
		next_dev = (ONVIF_DEVICE_EX *)pps_lookup_next(m_dev_ul, dev);

		freeDevice(dev);

		dev = next_dev;
	}

	pps_lookup_end(m_dev_ul);
}

bool ptzLeftUpper(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.PanTiltFlag = 1;
	req.Translation.PanTilt.x = -0.2;
	req.Translation.PanTilt.y = 0.2;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzUpper(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.PanTiltFlag = 1;
	req.Translation.PanTilt.x = 0;
	req.Translation.PanTilt.y = 0.2;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzRightUpper(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.PanTiltFlag = 1;
	req.Translation.PanTilt.x = 0.2;
	req.Translation.PanTilt.y = 0.2;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzPTZLeft(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.PanTiltFlag = 1;
	req.Translation.PanTilt.x = -0.2;
	req.Translation.PanTilt.y = 0;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzRight(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.PanTiltFlag = 1;
	req.Translation.PanTilt.x = 0.2;
	req.Translation.PanTilt.y = 0;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);
	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzLeftDown(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.PanTiltFlag = 1;
	req.Translation.PanTilt.x = -0.2;
	req.Translation.PanTilt.y = -0.2;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzDown(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.PanTiltFlag = 1;
	req.Translation.PanTilt.x = 0;
	req.Translation.PanTilt.y = -0.2;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzRightDown(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.PanTiltFlag = 1;
	req.Translation.PanTilt.x = 0.2;
	req.Translation.PanTilt.y = -0.2;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzZoomOut(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.ZoomFlag = 1;
	req.Translation.Zoom.x = -0.2;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzZoomIn(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	RelativeMove_REQ req;
	memset(&req, 0, sizeof(req));

	req.Translation.ZoomFlag = 1;
	req.Translation.Zoom.x = 0.2;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_RelativeMove(&p_dev->onvif_device, &req, NULL)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

bool ptzHome(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return FALSE;
	}

	GotoHomePosition_REQ req;
	memset(&req, 0, sizeof(req));

	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	if (onvif_GotoHomePosition(&p_dev->onvif_device, &req, NULL)) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

void ptzStop(ONVIF_DEVICE_EX * p_dev)
{
	if (NULL == p_dev ||
		!p_dev->onvif_device.Capabilities.ptz.support ||
		NULL == p_dev->onvif_device.curProfile)
	{
		return;
	}

	PTZStop_REQ req;
	memset(&req, 0, sizeof(req));

	req.ZoomFlag = 1;
	req.Zoom = TRUE;
	req.PanTiltFlag = 1;
	req.PanTilt = TRUE;
	strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

	onvif_PTZStop(&p_dev->onvif_device, &req, NULL);
}

void StartVstap() {

	event_camera.clear();
	camera_count = 0;
#ifdef WIN32
	initWinSock();
#endif
	//if(!g_probe_running){
	// open log file
	//log_init("ipsee.txt");
	//log_set_level(LOG_DBG);

	// init sys buffer
	sys_buf_init(100);

	// init http message buffer
	http_msg_buf_fl_init(100);

	// max support 100 devices
	m_dev_fl = pps_ctx_fl_init(100, sizeof(ONVIF_DEVICE_EX), TRUE);
	m_dev_ul = pps_ctx_ul_init(m_dev_fl, TRUE);

	// set event callback
	onvif_set_event_notify_cb(eventNotifyCallback, 0);
	// set event subscribe disconnect callback
	onvif_set_subscribe_disconnect_cb(subscribeDisconnectCallback, 0);

	// set probe callback
	set_probe_cb(probeCallback, 0);
	// start probe thread
	start_probe(NULL, 10);
	//    sleep(5);
	//    stop_probe();
	//    	clearDevices();

	//    	pps_ul_free(m_dev_ul);
	//        pps_fl_free(m_dev_fl);
	//
	//        http_msg_buf_fl_deinit();
	//        sys_buf_deinit();
	//
	//log_close();

}

void StopVstap() {

	event_camera.clear();
	event_camera_ind.clear();
	event_camera_action.clear();
	camera_count = 0;
	StartOnVifThread = FALSE;
	DidCameraFound = FALSE;
	stop_probe();
	clearDevices();

	pps_ul_free(m_dev_ul);
	pps_fl_free(m_dev_fl);

	http_msg_buf_fl_deinit();
}

void RefreshVstap() {

	event_camera.clear();
	event_camera_ind.clear();
	event_camera_action.clear();
	camera_count = 0;
	StartOnVifThread = FALSE;
	DidCameraFound = FALSE;
}

bool VstapOnvifCameras()
{
	event_camera.clear();
	event.clear();
	ONVIF_DEVICE_EX * p_devCamera = (ONVIF_DEVICE_EX *)pps_lookup_start(m_dev_ul);
	// if ((p_devCamera->onvif_device.errCode) == 0){
	if (p_devCamera) {
		event.clear();
		while (p_devCamera) {
			GetCapabilities(&p_devCamera->onvif_device);
			GetSystemDateAndTime(&p_devCamera->onvif_device);
			//event["deviceType"] = p_devCamera->onvif_device.binfo.type;
			event["cameraIP"] = p_devCamera->onvif_device.binfo.XAddr.host;
			event["cameraPortNumber"] = p_devCamera->onvif_device.binfo.XAddr.port;
			event["deviceIdendifier"] = p_devCamera->onvif_device.binfo.EndpointReference;
			//event["isAuthenticationRequired"] = p_devCamera->onvif_device.needAuth;
			if ((p_devCamera->onvif_device.errCode) == -1) {
				event["dead"] = TRUE;
			}
			else {
				event["dead"] = FALSE;
				event["isAuthenticationRequired"] = p_devCamera->onvif_device.needAuth;
				if ((p_devCamera->onvif_device.authFailed)) {
					event["isAuthenticated"] = FALSE;
				}
				else {
						event["isAuthenticated"] = TRUE;
						if (GetDeviceInformation(&p_devCamera->onvif_device)) {
							event["firmwareVersion"] = p_devCamera->onvif_device.DeviceInformation.FirmwareVersion;
							event["hardwareId"] = p_devCamera->onvif_device.DeviceInformation.HardwareId;
							event["manufacturer"] = p_devCamera->onvif_device.DeviceInformation.Manufacturer;
							event["model"] = p_devCamera->onvif_device.DeviceInformation.Model;
							event["serialNumber"] = p_devCamera->onvif_device.DeviceInformation.SerialNumber;
						}
				}
			}
			//BREAK_CAMERA_LOOP:
			event_camera["Cameras"].append(event);
			event.clear();
			p_devCamera = (ONVIF_DEVICE_EX *)pps_lookup_next(m_dev_ul, p_devCamera);
		}
		event_camera["DidCameraFound"] = TRUE;
	}
	else {
		event_camera["DidCameraFound"] = FALSE;
	}
	pps_lookup_end(m_dev_ul);
	//fprintf(stderr, "I'm HereGetCameraEndLoop!\n");
	//std::ostringstream CameraOut;
	//CameraOut << styledWriter.write(event_camera);
	//std::string CameraString = CameraOut.str();
	return TRUE;
}


bool vstapCameraIndividual(const char* Camera_IP, const char* Camera_Port, const char* Camera_Username, const char* Camera_Password) {
	if (g_probe_count >= 9) { usleep(3000000); }
	ONVIF_DEVICE_EX deviceindividual;
	memset(&deviceindividual, 0, sizeof(ONVIF_DEVICE_EX));
	strcpy(deviceindividual.onvif_device.binfo.XAddr.host, Camera_IP);
	deviceindividual.onvif_device.binfo.XAddr.port = atoi(Camera_Port);

	event_camera_ind.clear();
	event_ind.clear();
	ONVIF_DEVICE_EX * p_devCamera = (ONVIF_DEVICE_EX *)pps_lookup_start(m_dev_ul);
	//    if ((p_devCamera->onvif_device.errCode) == 0){
	while (p_devCamera)
	{
		if (strcmp(p_devCamera->onvif_device.binfo.XAddr.host, deviceindividual.onvif_device.binfo.XAddr.host) == 0 &&
			p_devCamera->onvif_device.binfo.XAddr.port == deviceindividual.onvif_device.binfo.XAddr.port)
		{
			event_ind["cameraIP"] = p_devCamera->onvif_device.binfo.XAddr.host;
			event_ind["cameraPortNumber"] = p_devCamera->onvif_device.binfo.XAddr.port;
			event_ind["deviceIdendifier"] = p_devCamera->onvif_device.binfo.EndpointReference;

			event_camera_ind.clear();
			//onvif_SetAuthInfo(&p_devCamera->onvif_device, Camera_Username, Camera_Password);
			GetCapabilities(&p_devCamera->onvif_device);
			GetSystemDateAndTime(&p_devCamera->onvif_device);

			if ((p_devCamera->onvif_device.errCode) == -1) {
				event_ind["cameraIP"] = p_devCamera->onvif_device.binfo.XAddr.host;
				event_ind["cameraPortNumber"] = p_devCamera->onvif_device.binfo.XAddr.port;
				event_ind["dead"] = TRUE;

			}
			else {
				event_ind["dead"] = FALSE;
				event_ind["isAuthenticationRequired"] = p_devCamera->onvif_device.needAuth;
				if (Camera_Username != NULL && Camera_Username[0] != '\0') {
					if ((Camera_Password != NULL && Camera_Password[0] != '\0') || Camera_Password[0] == '\0') {
						
						onvif_SetAuthInfo(&p_devCamera->onvif_device, Camera_Username, Camera_Password);
						GetCapabilities(&p_devCamera->onvif_device);
						GetServices(&p_devCamera->onvif_device);
						//GetSystemDateAndTime(&p_devCamera->onvif_device);
						if ((p_devCamera->onvif_device.authFailed)) {
							event_ind["isAuthenticated"] = FALSE;
						}
						else {
							event_ind["isAuthenticated"] = TRUE;
							if (GetDeviceInformation(&p_devCamera->onvif_device)) {
								event_ind["firmwareVersion"] = p_devCamera->onvif_device.DeviceInformation.FirmwareVersion;
								event_ind["hardwareId"] = p_devCamera->onvif_device.DeviceInformation.HardwareId;
								event_ind["manufacturer"] = p_devCamera->onvif_device.DeviceInformation.Manufacturer;
								event_ind["model"] = p_devCamera->onvif_device.DeviceInformation.Model;
								event_ind["serialNumber"] = p_devCamera->onvif_device.DeviceInformation.SerialNumber;
							}

							if ((p_devCamera->onvif_device.errCode) != 0) {
								GetVideoSources(&p_devCamera->onvif_device);

								GetImagingSettings(&p_devCamera->onvif_device);

								GetVideoSourceConfigurations(&p_devCamera->onvif_device);
							}
							if (GetVideoEncoderConfigurations(&p_devCamera->onvif_device)) {
								GetVideoEncoderConfigurationOptions_REQ encoder_req;
								GetVideoEncoderConfigurationOptions_RES encoder_res;
								memset(&encoder_req, 0, sizeof(encoder_req));
								memset(&encoder_res, 0, sizeof(encoder_res));
								onvif_VideoResolution supportedResolutions[MAX_RES_NUMS] = {};
								if (onvif_GetVideoEncoderConfigurationOptions(&p_devCamera->onvif_device, &encoder_req, &encoder_res)) {
									if (p_devCamera->onvif_device.video_enc != NULL) {
										if (p_devCamera->onvif_device.video_enc->Configuration.H264Flag) {
											event_ind["resolution"]["height"] = p_devCamera->onvif_device.video_enc->Configuration.Resolution.Height;
											event_ind["resolution"]["width"] = p_devCamera->onvif_device.video_enc->Configuration.Resolution.Width;
											memcpy(supportedResolutions, encoder_res.Options.H264.ResolutionsAvailable, sizeof(encoder_res.Options.H264.ResolutionsAvailable));
										}
										else if (p_devCamera->onvif_device.video_enc->Configuration.MPEG4Flag) {
											event_ind["resolution"]["height"] = p_devCamera->onvif_device.video_src->VideoSource.Resolution.Height;
											event_ind["resolution"]["width"] = p_devCamera->onvif_device.video_src->VideoSource.Resolution.Width;
											memcpy(supportedResolutions, encoder_res.Options.MPEG4.ResolutionsAvailable, sizeof(encoder_res.Options.MPEG4.ResolutionsAvailable));
										}
										for (int resolutionCount = 0; resolutionCount < MAX_RES_NUMS; resolutionCount++) {
											if (supportedResolutions[resolutionCount].Height == 0 && supportedResolutions[resolutionCount].Width == 0) {
												break;
											}
											event_ind["supportedResolutions"][resolutionCount]["height"] = supportedResolutions[resolutionCount].Height;
											event_ind["supportedResolutions"][resolutionCount]["width"] = supportedResolutions[resolutionCount].Width;
										}
									}
								}
							}

							if (GetAudioSources(&p_devCamera->onvif_device)) {
								event_ind["audioSupport"] = TRUE;
							}
							else {
								event_ind["audioSupport"] = FALSE;
							}
							char profileToken[ONVIF_TOKEN_LEN];
							ONVIF_Profile   * p_profile = NULL;

							/* save currrent profile token */
							if (p_devCamera->onvif_device.curProfile) {
								strcpy(profileToken, p_devCamera->onvif_device.curProfile->token);
							}
							else {
								memset(profileToken, 0, sizeof(profileToken));
							}

							GetProfiles(&p_devCamera->onvif_device);

							/* resume current profile */
							if (profileToken[0] != '\0') {
								p_profile = onvif_find_Profile(&p_devCamera->onvif_device, profileToken);
							}

							if (NULL == p_profile) {
								p_profile = p_devCamera->onvif_device.profiles;
							}

							p_devCamera->onvif_device.curProfile = p_profile;

							if (p_devCamera->onvif_device.Capabilities.ptz.support) {
								event["ptzSupport"] = TRUE;
								//GetNodes(&p_devCamera->onvif_device);
								//GetConfigurations(&p_devCamera->onvif_device);
							}
							else {
								event_ind["ptzSupport"] = FALSE;
							}
							if (GetStreamUris(&p_devCamera->onvif_device)) {
								if (p_devCamera->onvif_device.profiles) {
									event_ind["rtspURL"] = p_devCamera->onvif_device.profiles->stream_uri;
								}
								else {
									event_ind["rtspURL"] = FALSE;
								}
							}
						}
					}
				}
				else {
					//onvif_SetAuthInfo(&p_devCamera->onvif_device, "admin", "admin");
				}
				//        event["cameraIP"] = p_devCamera->onvif_device.binfo.XAddr.host;
				//        event["cameraPortNumber"] = p_devCamera->onvif_device.binfo.XAddr.port;
				//        event["deviceIdendifier"] = p_devCamera->onvif_device.binfo.EndpointReference;
			}
		//BREAK_LOOP:
			event_camera_ind["Cameras"].append(event_ind);
			event_camera_ind["DidCameraFound"] = TRUE;
			event_ind.clear();
			pps_lookup_end(m_dev_ul);
			break;
		}
		p_devCamera = (ONVIF_DEVICE_EX *)pps_lookup_next(m_dev_ul, p_devCamera);
	}

	if (event_camera_ind["Cameras"].isNull()) {
		//event_camera["Cameras"].append(event);
		event_camera_ind["DidCameraFound"] = FALSE;
		event_ind.clear();
		return FALSE;
	}

	pps_lookup_end(m_dev_ul);
	return TRUE;
}

bool CameraAction(const char* Camera_IP, const char* Camera_Port, const char* Camera_Username, const char* Camera_Password, const char* PTZoomReset, const char* ResolutionHeight, const char* ResolutionWidth, const char* New_Username, const char* New_Password) {
	if (g_probe_count >= 9) { usleep(3000000); }
	event_camera_action.clear();
	event_act.clear();
	//fprintf(stderr, "Count is:%d\n", g_probe_running);
	ONVIF_DEVICE_EX device;
	memset(&device, 0, sizeof(ONVIF_DEVICE_EX));
	strcpy(device.onvif_device.binfo.XAddr.host, Camera_IP);
	device.onvif_device.binfo.XAddr.port = atoi(Camera_Port);

	//camera_count = 0;
	ONVIF_DEVICE_EX * p_devCamera = (ONVIF_DEVICE_EX *)pps_lookup_start(m_dev_ul);
	// if ((p_devCamera->onvif_device.errCode) == 0){
	while (p_devCamera)
	{
		if (strcmp(p_devCamera->onvif_device.binfo.XAddr.host, device.onvif_device.binfo.XAddr.host) == 0 &&
			p_devCamera->onvif_device.binfo.XAddr.port == device.onvif_device.binfo.XAddr.port)
		{
			event_act["cameraIP"] = p_devCamera->onvif_device.binfo.XAddr.host;
			event_act["cameraPortNumber"] = p_devCamera->onvif_device.binfo.XAddr.port;
			event_act["deviceIdendifier"] = p_devCamera->onvif_device.binfo.EndpointReference;
			//onvif_SetAuthInfo(&p_devCamera->onvif_device, Camera_Username, Camera_Password);
			GetCapabilities(&p_devCamera->onvif_device);
			GetSystemDateAndTime(&p_devCamera->onvif_device);

			if ((p_devCamera->onvif_device.errCode) == -1 || (p_devCamera->onvif_device.errCode) == -2) {

				if ((p_devCamera->onvif_device.errCode) == -1) {
					event_act["dead"] = TRUE;
				}

			}
			else {
				event_act["dead"] = FALSE;
				if (Camera_Username != NULL && Camera_Username[0] != '\0') {
					if ((Camera_Password != NULL && Camera_Password[0] != '\0') || Camera_Password[0] == '\0') {
						onvif_SetAuthInfo(&p_devCamera->onvif_device, Camera_Username, Camera_Password);
						GetCapabilities(&p_devCamera->onvif_device);
						GetSystemDateAndTime(&p_devCamera->onvif_device);
						if ((p_devCamera->onvif_device.authFailed)) {
							event_act["isAuthenticated"] = FALSE;
						}
						else {
							event_act["isAuthenticated"] = TRUE;
							//                    }
							char profileToken[ONVIF_TOKEN_LEN];
							ONVIF_Profile   * p_profile = NULL;

							/* save currrent profile token */
							if (p_devCamera->onvif_device.curProfile) {
								strcpy(profileToken, p_devCamera->onvif_device.curProfile->token);
							}
							else {
								memset(profileToken, 0, sizeof(profileToken));
							}

							GetProfiles(&p_devCamera->onvif_device);

							/* resume current profile */
							if (profileToken[0] != '\0') {
								p_profile = onvif_find_Profile(&p_devCamera->onvif_device, profileToken);
							}

							if (NULL == p_profile) {
								p_profile = p_devCamera->onvif_device.profiles;
							}

							p_devCamera->onvif_device.curProfile = p_profile;

							if (p_devCamera->onvif_device.Capabilities.ptz.support) {
								//event["ptzSupport"] = TRUE;
								GetNodes(&p_devCamera->onvif_device);
								GetConfigurations(&p_devCamera->onvif_device);
							}
							if (PTZoomReset != NULL && PTZoomReset[0] != '\0') {
								event_act["checkAction"] = TRUE;
								if (strcmp(PTZoomReset, "Right") == 0) {
									if (ptzRight(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "Left") == 0) {
									if (ptzPTZLeft(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "Upper") == 0) {
									if (ptzUpper(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "Down") == 0) {
									if (ptzDown(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "Home") == 0) {
									if (ptzHome(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "ZoomIn") == 0) {
									if (ptzZoomIn(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "ZoomOut") == 0) {
									if (ptzZoomOut(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "LeftDown") == 0) {
									if (ptzLeftDown(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "LeftUpper") == 0) {
									if (ptzLeftUpper(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "RightDown") == 0) {
									if (ptzRightDown(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "RightUpper") == 0) {
									if (ptzRightUpper(p_devCamera)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "ResetHard") == 0) {
									SetSystemFactoryDefault_REQ p_req;
									const char * ResetHardSoft = NULL;
									ResetHardSoft = "Hard";
									p_req.FactoryDefault = onvif_StringToFactoryDefaultType(ResetHardSoft);
									SetSystemFactoryDefault_RES p_res;
									//            	onvif_SystemReboot(p_dev, p_req, p_res);
									if (onvif_SetSystemFactoryDefault(&p_devCamera->onvif_device, &p_req, &p_res)) {
										//            	if(onvif_SystemReboot(&device.onvif_device, &p_req, &p_res)){
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "ResetSoft") == 0) {
									SetSystemFactoryDefault_REQ p_req;
									const char * ResetHardSoft = NULL;
									ResetHardSoft = "Soft";
									p_req.FactoryDefault = onvif_StringToFactoryDefaultType(ResetHardSoft);
									SetSystemFactoryDefault_RES p_res;
									//            	onvif_SystemReboot(p_dev, p_req, p_res);
									if (onvif_SetSystemFactoryDefault(&p_devCamera->onvif_device, &p_req, &p_res)) {
										//            	if(onvif_SystemReboot(&device.onvif_device, &p_req, &p_res)){
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else if (strcmp(PTZoomReset, "Reboot") == 0) {
									SystemReboot_REQ p_req;
									SystemReboot_RES p_res;
									//            	onvif_SystemReboot(p_dev, p_req, p_res);
									if (onvif_SystemReboot(&p_devCamera->onvif_device, &p_req, &p_res)) {
										event_act["actionStatus"] = TRUE;
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
									//            	return TRUE;
								}
								else if (strcmp(PTZoomReset, "Resolution") == 0) {
									if (ResolutionHeight != 0 && ResolutionWidth != 0) {
										GetVideoEncoderConfigurations(&p_devCamera->onvif_device);
										//            			GetAudioEncoderConfigurations(&device.onvif_device);
										SetVideoEncoderConfiguration_REQ p_req;
										p_req.ForcePersistence = TRUE;
										p_req.VideoEncoderConfiguration = p_devCamera->onvif_device.video_enc->Configuration;
										p_req.VideoEncoderConfiguration.Resolution.Height = atoi(ResolutionHeight);
										p_req.VideoEncoderConfiguration.Resolution.Width = atoi(ResolutionWidth);
										SetVideoEncoderConfiguration_RES p_res;
										if (onvif_SetVideoEncoderConfiguration(&p_devCamera->onvif_device, &p_req, &p_res)) {
											event_act["actionStatus"] = FALSE;
										}
										else {
											event_act["actionStatus"] = TRUE;
										}
									}
								}
								else if (strcmp(PTZoomReset, "changepassword") == 0) {
									GetCapabilities(&p_devCamera->onvif_device);
									GetDeviceInformation(&p_devCamera->onvif_device);
									GetUsers_RES user;
									memset(&user, 0, sizeof(user));
									if (onvif_GetUsers(&p_devCamera->onvif_device, NULL, &user)) {
									}
									else {
									}

									SetUser_REQ req;
									memset(&req, 0, sizeof(req));

									// if (Camera_Username != NULL && Camera_Username[0] != '\0') {
									if (New_Password != NULL && New_Password[0] != '\0') {
										strcpy(req.User.Username, Camera_Username);
										strcpy(req.User.Password, New_Password);
										req.User.PasswordFlag = 1;
										req.User.UserLevel = UserLevel_Administrator;
									}
									else {
										strcpy(req.User.Username, Camera_Username);
										strcpy(req.User.Password, Camera_Password);
										req.User.PasswordFlag = 1;
										req.User.UserLevel = UserLevel_Administrator;
									}
									SetUser_RES res;
									if (onvif_SetUser(&p_devCamera->onvif_device, &req, &res)) {
										event_act["actionStatus"] = TRUE;
										onvif_SetAuthInfo(&p_devCamera->onvif_device, req.User.Username, req.User.Password);
									}
									else {
										event_act["actionStatus"] = FALSE;
									}
								}
								else {
									event_act["checkAction"] = FALSE;
								}
							}
							else {
								event_act["checkAction"] = FALSE;
							}
						}

					}
				}
				else {
					//onvif_SetAuthInfo(&p_devCamera->onvif_device, "admin", "admin");
				}
				//        event["cameraIP"] = p_devCamera->onvif_device.binfo.XAddr.host;
				//        event["cameraPortNumber"] = p_devCamera->onvif_device.binfo.XAddr.port;
				//        event["deviceIdendifier"] = p_devCamera->onvif_device.binfo.EndpointReference;
			}
		BREAK_ACTION_LOOP:
			event_camera_action["Camera_Action"].append(event_act);
			event_camera_action["DidCameraFound"] = TRUE;
			event_act.clear();
			pps_lookup_end(m_dev_ul);
			//break;
		}
		p_devCamera = (ONVIF_DEVICE_EX *)pps_lookup_next(m_dev_ul, p_devCamera);
	}
	//Action:
	if (event_camera_action["Camera_Action"].isNull()) {
		event_camera_action["DidCameraFound"] = FALSE;
		event_act.clear();
		return FALSE;
	}
	pps_lookup_end(m_dev_ul);
	return TRUE;
}



