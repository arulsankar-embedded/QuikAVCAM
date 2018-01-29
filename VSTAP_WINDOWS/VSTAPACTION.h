void StartVstap();
void StopVstap();
void RefreshVstap();
bool VstapOnvifCameras();
bool vstapCameraIndividual(const char* Camera_IP, const char* Camera_Port, const char* Camera_Username, const char* Camera_Password);
//BOOL OnvifPTZactions();
bool CameraAction(const char* Camera_IP, const char* Camera_Port, const char* Camera_Username, const char* Camera_Password, const char* PTZoomReset, const char* ResolutionHeight, const char* ResolutionWidth, const char* New_Username, const char* New_Password);
