/*
 * webserver_test.cpp
 *
 *  Created on: 20-Jul-2017
 *      Author: arulsankar
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS
 //#define _CRT_SECURE_NO_WARNINGS

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_WARNINGS 
//#define _CRT_SECURE_NO_WARNINGS_GLOBALS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string>
#include <sys/stat.h>
#include <winsock2.h> // socket related definitions
#include <ws2tcpip.h> // socket related definitions
#include <sys/types.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include "microhttpd.h"
#include "webserver.h"
#include "json.h"

#include "VSTAP.h"

#if defined(_WIN32)
#define strdup _strdup
#endif



static Webserver *wserver;
pthread_mutex_t nlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t glock = PTHREAD_MUTEX_INITIALIZER;
bool done = false;
long webport;
int debug = false;
struct tm tm;

char temp_buf[2048];

char * IP;
char *path;

char config_path[1024];
char log_path[1024];
char command_path[1024];

extern Json::Value event_camera;
extern Json::Value event_camera_ind;
extern Json::Value event_camera_action;
extern BOOL StartOnVifThread;
extern BOOL DidCameraFound;
extern int CameraFound;

/*
 #include <errno.h>
 #include <iostream>
 #include <string>
 */
#include "json.h"


#define NBSP(str)	(str) == NULL || (*str) == '\0' ? "&nbsp;" : (str)
#define BLANK(str)	(str) == NULL || (*str) == '\0' ? "" : (str)
#define FNF "<html><head><title>File not found</title></head><body>File not found: %s</body></html>"
#define UNKNOWN "<html><head><title>Nothingness</title></head><body>There is nothing here. Sorry.</body></html>\n"
#define DEFAULT "<html><body>Hello, browser!</body></html>"
#define EMPTY "<html></html>"
#define DEFAULT_PORT 8090

enum week {
	sunday, monday, tuesday, wednesday, thursday, friday, saturday
};
int weekDay = 0;
//enum WEEK { SUNDAY, MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY };
typedef struct _conninfo {
	conntype_t conn_type;
	char const *conn_url;
	void *conn_arg1;
	void *conn_arg2;
	void *conn_arg3;
	void *conn_arg4;
	void *conn_res;
	struct MHD_PostProcessor *conn_pp;
} conninfo_t;

bool Webserver::ready = false;

#define SET	1
#define CLEAR	0

extern pthread_mutex_t nlock;
extern pthread_mutex_t glock;
extern bool done;
extern int debug;
extern struct tm tm;

//string command_str;
//string msg;




/*
 * web_send_data
 * Send internal HTML string
 */
static int web_send_data(struct MHD_Connection *connection, const char *data,
		const int code, bool free, bool copy, const char *ct) {
	struct MHD_Response *response;
	int ret;

	if (!copy && !free)
		response = MHD_create_response_from_buffer(strlen(data), (void *) data,
				MHD_RESPMEM_PERSISTENT);
	else if (copy)
		response = MHD_create_response_from_buffer(strlen(data), (void *) data,
				MHD_RESPMEM_MUST_COPY);
	else
		response = MHD_create_response_from_buffer(strlen(data), (void *) data,
				MHD_RESPMEM_MUST_FREE);

	if (response == NULL)
		return MHD_NO;
	if (ct != NULL)
		MHD_add_response_header(response, "Content-type", ct);
	ret = MHD_queue_response(connection, code, response);
	MHD_destroy_response(response);
	return ret;
}

/*
 * web_read_file
 * Read files and send them out
 */
ssize_t web_read_file(void *cls, uint64_t pos, char *buf, size_t max) {
	FILE *file = (FILE *) cls;

	fseek(file, pos, SEEK_SET);
	return fread(buf, 1, max, file);
}

/*
 * web_close_file
 */
void web_close_file(void *cls) {
	FILE *fp = (FILE *) cls;

	fclose(fp);
}

/*
 * web_send_file
 * Read files and send them out
 */
int web_send_file(struct MHD_Connection *conn, const char *filename, const int code, const bool unl)
{
	struct stat buf;
	FILE *fp;
	struct MHD_Response *response;
	const char *p;
	const char *ct = NULL;
	int ret;
	if ((p = strchr(filename, '.')) != NULL) {
		p++;
		if (strcmp(p, "json") == 0)
			ct = "application/json";
		else if (strcmp(p, "js") == 0)
			ct = "text/javascript";
	}
	if (stat(filename, &buf) == -1 ||
		((fp = fopen(filename, "rb")) == NULL)) {

		if (strcmp(p, "json") == 0) {
			response = MHD_create_response_from_buffer(0, (void *) "", MHD_RESPMEM_PERSISTENT);
		}
		else {
			int len = strlen(FNF) + strlen(filename) - 1; // len(%s) + 1 for \0
			char *s = (char *)malloc(len);
			if (s == NULL) {
				fprintf(stderr, "Out of memory FNF\n");
				exit(1);
			}
			snprintf(s, len, FNF, filename);
			response = MHD_create_response_from_buffer(len, (void *)s, MHD_RESPMEM_MUST_FREE); // free
		}
	}
	else
	{
		response = MHD_create_response_from_callback(buf.st_size, 32 * 1024, &web_read_file, fp,
			&web_close_file);
	}
	if (response == NULL)
		return MHD_YES;
	if (ct != NULL) {
		MHD_add_response_header(response, "Content-type", ct);
		fprintf(stderr, ct);
	}
	ret = MHD_queue_response(conn, code, response);
	MHD_destroy_response(response);
	if (unl)
		_unlink(filename);
	return ret;
}


/*ArulSankar API*/

int Webserver::SendDeviceID_userdata(struct MHD_Connection *conn)
{
	const char* Command = NULL;
	const char* Camera_IP = NULL;
	const char* Camera_Port = NULL;
	const char* Camera_UserName = NULL;
	const char* Camera_PassWord = NULL;
	const char* New_UserName = NULL;
	const char* New_PassWord = NULL;
	const char* PTZoomReset = NULL;
	const char* ResolutionHeight = NULL;
	const char* ResolutionWidth = NULL;
	int32 ret;
	char *fn = NULL;

	char fntemp[512];
	std::ofstream file_id;
	Json::Value event_status;
	Json::Value Devices_status;
	Json::StyledWriter styledWriter;

	Command = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "action");
	Camera_IP = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "cameraIP");
	Camera_Port = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "cameraPort");
	Camera_UserName = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "cameraUsername");
	Camera_PassWord = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "cameraPassword");
	New_UserName = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "newUsername");
	New_PassWord = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "newPassword");
	PTZoomReset = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "setAction");
	ResolutionHeight = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "resolutionHeight");
	ResolutionWidth = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "resolutionWidth");

	if (Command != NULL && Command[0] != '\0')
	{
		//system("del /q/f/s %TEMP%\\*.json");
		system("del /Q C:\\ProgramData\\QuikAV\\IOT\\Command\\*.json");
		Devices_status.clear();
		if (strcmp(Command, "GetCameraProfile") == 0) { /* Camera Profile */
			pthread_mutex_lock(&nlock);
			event_status.clear();
			CameraFound = 0;
			//			StartOnVIF();

			//			sleep(5);

			fprintf(stderr, "I'm Here!\n");
			//system("./server_bash.sh");
			while (!StartOnVifThread) {
				if (!DidCameraFound) {
					fprintf(stderr, "CameraFound->%d\n", CameraFound);
					CameraFound++;
					if (CameraFound > 10) {
						Devices_status["statusMessage"] = "No camera found!";
						CameraFound = 0;
						goto Search;
					}
				}
				Sleep(1000);
			}
			Devices_status["statusMessage"] = "Cameras found!";
			if (VstapOnvifCameras()) {
				event_status = event_camera;
			}
		Search:
			Devices_status["statusCode"] = MHD_HTTP_OK;
			RefreshVstap();
			pthread_mutex_unlock(&nlock);
		}
		else if (strcmp(Command, "CameraIndividual") == 0) { /* Camera Profile */
			event_status.clear();
			pthread_mutex_lock(&nlock);
			if (Camera_IP != NULL && Camera_IP[0] != '\0') {
				if (Camera_Port != NULL && Camera_Port[0] != '\0') {
					if (Camera_UserName != NULL && Camera_UserName[0] != '\0') {
						if ((Camera_PassWord != NULL && Camera_PassWord[0] != '\0') || Camera_PassWord[0] == '\0') {
							//if (Camera_PassWord[0] == '\0') {
							//	Camera_PassWord = "admin";
							//}
							if (vstapCameraIndividual(Camera_IP, Camera_Port, Camera_UserName, Camera_PassWord)) {
								event_status = event_camera_ind;
								Devices_status["statusMessage"] = "Camera found!";
							}
							else {
								event_status = event_camera_ind;
								Devices_status["statusMessage"] = "No camera found!";
							}
							Devices_status["statusCode"] = MHD_HTTP_OK;
						}
						else {
							std::string str_error = "Please check your cameraPassword option!";
							Devices_status["statusCode"] = MHD_HTTP_BAD_REQUEST;
							Devices_status["statusMessage"] = str_error.c_str();
						}
					}
					else {
						std::string str_error = "Please check your cameraUsername option!";
						Devices_status["statusCode"] = MHD_HTTP_BAD_REQUEST;
						Devices_status["statusMessage"] = str_error.c_str();
					}
				}
				else {
					std::string str_error = "Please check your cameraPort option!";
					Devices_status["statusCode"] = MHD_HTTP_BAD_REQUEST;
					Devices_status["statusMessage"] = str_error.c_str();
				}
			}
			else {
				std::string str_error = "Please check your cameraIP option!";
				Devices_status["statusCode"] = MHD_HTTP_BAD_REQUEST;
				Devices_status["statusMessage"] = str_error.c_str();
			}
			RefreshVstap();
			pthread_mutex_unlock(&nlock);
		}
		else if (strcmp(Command, "CameraAction") == 0) { /* Camera Action */
			pthread_mutex_lock(&nlock);
			event_status.clear();
			//			StartOnVIF();
			if (Camera_IP != NULL && Camera_IP[0] != '\0') {
				if (Camera_Port != NULL && Camera_Port[0] != '\0') {
					if (Camera_UserName != NULL && Camera_UserName[0] != '\0') {
						if ((Camera_PassWord != NULL && Camera_PassWord[0] != '\0') || Camera_PassWord[0] == '\0') {
							if (PTZoomReset != NULL && PTZoomReset[0] != '\0') {
								//						const char* PTZoomReset = NULL;
								//						if (strcmp(PTZoomReset, "PTZ") == 0){
								//							PTZoomReset = PTZ;
								//						}else if (strcmp(PTZoomReset, "PTZ") == 0){
								//							PTZoomReset = Zoom;
								//						}else if (strcmp(PTZoomReset, "Reboot") == 0){
								//							fprintf(stderr, "Reboot Ok!\n");
								//							PTZoomReset = Reboot;
								//						}
								//if (Camera_PassWord[0] == '\0') {
								//	Camera_PassWord = "admin";
								//}
								ResolutionHeight = NULL;
								ResolutionWidth = NULL;
								fprintf(stderr, "I'm Here!\n");
								if (CameraAction(Camera_IP, Camera_Port, Camera_UserName, Camera_PassWord, PTZoomReset, ResolutionHeight, ResolutionWidth, New_UserName, New_PassWord)) {
									event_status = event_camera_action;
									Devices_status["statusMessage"] =
										"Camera action is successfully executed!";
								}
								else {
									event_status = event_camera_action;
									Devices_status["statusMessage"] = "No camera found!";
								}
								Devices_status["statusCode"] = MHD_HTTP_OK;
							}
							else {
								std::string str_error =
									"Please check your camera_setAction (PTZ/Zoom/Reset(@start)/Resolution) option!";
								Devices_status["statusCode"] =
									MHD_HTTP_BAD_REQUEST;
								Devices_status["statusMessage"] =
									str_error.c_str();
							}
						}
						else {
							std::string str_error =
								"Please check your camera_Password option!";
							Devices_status["statusCode"] = MHD_HTTP_BAD_REQUEST;
							Devices_status["statusMessage"] = str_error.c_str();
						}
					}
					else {
						std::string str_error =
							"Please check your camera_Username option!";
						Devices_status["statusCode"] = MHD_HTTP_BAD_REQUEST;
						Devices_status["statusMessage"] = str_error.c_str();
					}
				}
				else {
					std::string str_error = "Please check your camera_Port option!";
					Devices_status["statusCode"] = MHD_HTTP_BAD_REQUEST;
					Devices_status["statusMessage"] = str_error.c_str();
				}
			}
			else {
				std::string str_error = "Please check your camera_IP option!";
				Devices_status["statusCode"] = MHD_HTTP_BAD_REQUEST;
				Devices_status["statusMessage"] = str_error.c_str();
			}
			RefreshVstap();
			pthread_mutex_unlock(&nlock);
		}
		else {
			Devices_status["status_code"] = MHD_HTTP_BAD_REQUEST;
			Devices_status["help"] = "Please check your actions (Add/Remove/Cancel/Refresh/Reset/Rename/SetTarget)";
			//pthread_mutex_unlock(&nlock);
		}
	}
	else {
		std::string str_error = "Oh Something went Wrong please check your API!!!!!";
		Devices_status["status_code"] = MHD_HTTP_BAD_REQUEST;
		Devices_status["help"] = str_error.c_str();
	}

	event_status["controllerCommand"] = Devices_status;
	snprintf(temp_buf, sizeof(temp_buf), "%sozwcp.device_action.XXXXXX", command_path);

	strncpy(fntemp, temp_buf, sizeof(fntemp));

	fn = mktemp(fntemp);
	if (fn == NULL)
		return MHD_YES;
	strncat(fntemp, ".json", sizeof(fntemp));
	//printf("Temp path is:%s\n", fntemp);
	file_id.open(fn);
	file_id << styledWriter.write(event_status);
	file_id.close();

	ret = web_send_file(conn, fn, MHD_HTTP_OK, true);

	remove(fntemp);
	//free(fn);
	//system("del /q/f/s %TEMP%\\*.json");
	return ret;
}
/*
 * web_config_post
 * Handle the post of the updated data
 */

int web_config_post(void *cls, enum MHD_ValueKind kind, const char *key,
		const char *filename, const char *content_type,
		const char *transfer_encoding, const char *data, uint64_t off,
		size_t size) {
	//conninfo_t *cp = (conninfo_t *) cls;

	fprintf(stderr, "post: key=%s data=%s size=%d\n", key, data, size);
	return MHD_YES;
}



/*
 * Process web requests
 */
int Webserver::HandlerEP(void *cls, struct MHD_Connection *conn,
		const char *url, const char *method, const char *version,
		const char *up_data, size_t *up_data_size, void **ptr) {
	Webserver *ws = (Webserver *) cls;

	return ws->Handler(conn, url, method, version, up_data, up_data_size, ptr);
}

int Webserver::Handler(struct MHD_Connection *conn, const char *url,
		const char *method, const char *version, const char *up_data,
		size_t *up_data_size, void **ptr) {
	int ret;
	conninfo_t *cp;

	if (debug)
		fprintf(stderr, "%x: %s: \"%s\" conn=%x size=%d *ptr=%x\n",
				pthread_self(), method, url, conn, *up_data_size, *ptr);
	if (*ptr == NULL) { /* do never respond on first call */
		cp = (conninfo_t *) malloc(sizeof(conninfo_t));
		if (cp == NULL)
			return MHD_NO;
		cp->conn_url = url;
		cp->conn_arg1 = NULL;
		cp->conn_arg2 = NULL;
		cp->conn_arg3 = NULL;
		cp->conn_arg4 = NULL;
		cp->conn_res = NULL;
		if (strcmp(method, MHD_HTTP_METHOD_POST) == 0) {
			cp->conn_pp = MHD_create_post_processor(conn, 1024, web_config_post,
					(void *) cp);
			if (cp->conn_pp == NULL) {
				free(cp);
				return MHD_NO;
			}
			cp->conn_type = CON_POST;
		} else if (strcmp(method, MHD_HTTP_METHOD_GET) == 0) {
			cp->conn_type = CON_GET;
		} else {
			free(cp);
			return MHD_NO;
		}
		*ptr = (void *) cp;
		return MHD_YES;
	}
	if (strcmp(method, MHD_HTTP_METHOD_GET) == 0) {
		if (strcmp(url, "/") == 0 || strcmp(url, "/index.html") == 0)
			ret = web_send_data(conn, DEFAULT, MHD_HTTP_OK, false, false,
							NULL);
		/*ArulSankar Changes*/
		else if (strcmp(url, "/user_request") == 0) {
			ret = SendDeviceID_userdata(conn);
		} else
			ret = web_send_data(conn, UNKNOWN, MHD_HTTP_NOT_FOUND, false, false,
			NULL); // no free, no copy
		return ret;
	} else if (strcmp(method, MHD_HTTP_METHOD_POST) == 0) {
		return 0;
	}else
		return MHD_NO;
}

/*
 * web_free
 * Free up any allocated data after connection closed
 */

void Webserver::FreeEP(void *cls, struct MHD_Connection *conn, void **ptr,
		enum MHD_RequestTerminationCode code) {
	Webserver *ws = (Webserver *) cls;

	ws->Free(conn, ptr, code);
}

void Webserver::Free(struct MHD_Connection *conn, void **ptr,
		enum MHD_RequestTerminationCode code) {
	conninfo_t *cp = (conninfo_t *) *ptr;

	if (cp != NULL) {
		if (cp->conn_arg1 != NULL)
			free(cp->conn_arg1);
		if (cp->conn_arg2 != NULL)
			free(cp->conn_arg2);
		if (cp->conn_arg3 != NULL)
			free(cp->conn_arg3);
		if (cp->conn_arg4 != NULL)
			free(cp->conn_arg4);
		if (cp->conn_type == CON_POST) {
			MHD_destroy_post_processor(cp->conn_pp);
		}
		free(cp);
		*ptr = NULL;
	}
}

/*
 * Constructor
 * Start up the web server
 */

Webserver::Webserver(int const wport) :
	sortcol(COL_NODE), logbytes(0), adminstate(false) {
	fprintf(stderr, "webserver starting port %d\n", wport);

	wdata = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
			DEFAULT_PORT,
			NULL, NULL, &Webserver::HandlerEP, NULL,
			MHD_OPTION_END);

	if (wdata != NULL) {
		ready = true;
	}
}

/*
 * Destructor
 * Stop the web server
 */

Webserver::~Webserver() {
	if (wdata != NULL) {
		MHD_stop_daemon((MHD_Daemon *) wdata);
		wdata = NULL;
		ready = false;
	}
	pthread_mutex_lock(&glock);

	pthread_mutex_unlock(&glock);
}


/*Get IP Address*/

int GetIpaddress()
{
	char ac[80];

	if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
		return 1;
	}
	std::cout << "Host name is " << ac << "." << std::endl;

	struct hostent *phe = gethostbyname(ac);
	if (phe == 0) {
		return 1;
	}

	for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
		struct in_addr addr;
		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		IP = inet_ntoa(addr);
		std::cout << "Address " << i << ": " << IP << std::endl;
	}

	return 0;
}

int GetConfig_Log_Path(char word[])
{
	FILE *f;
	char buf[1024];
	int ch = 0;
	int i = 0;

	f = fopen("C:\\Program Files\\QuikAV\\IOT\\config.txt", "r");
	if (f == NULL)
		return 1;
	while (fgets(buf, 1024, f) != NULL) {
		if (strstr(buf, word) != NULL)
		{

			i = strlen(word);
			if ((strcmp("command_path:", word)) == 0)
			{
				while (buf[i] != '\n')
				{
					command_path[ch] = buf[i];
					ch++;
					i++;
				}
				ch++;
				command_path[ch] = '\n';
				printf("CommandPathis=%s\n", command_path);
			}
			else
			{
				break;
			}
		}
		ch = 0;
	}
	fclose(f);
	return 0;
}

//-----------------------------------------------------------------------------
// <main>
// Create the driver and then wait
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {

	//long webport;
	//char *ptr;
	GetIpaddress();
	StartVstap();
	GetConfig_Log_Path("command_path:");
	wserver = new Webserver(DEFAULT_PORT);

	while (!wserver->isReady()) {
		delete wserver;
		Sleep(1000);
		wserver = new Webserver(DEFAULT_PORT);
	}

	while (!done) {	// now wait until we are done
		Sleep(500);
	}

	delete wserver;
	exit(0);
}
