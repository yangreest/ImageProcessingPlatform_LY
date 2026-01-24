#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <windows.h>

#include "SampleBoardBase.h"

class CPZMultiMedical
{
public:
	CPZMultiMedical();
	bool Init(int nConfigType,int nMultiBdTpye);
	bool BeginWork();
	bool EndWork();
	void RegisterImgCallback(const std::function<void(uint8_t*, int, int, int,int)>& f);
	void RegisterDeviceConnectCallback(const std::function<void(DeviceConnectStatus,int)>& f);
	void RegisterDeviceRunCallback(const std::function<void(DeviceRunStatus,int)>& f);
	void RegisterDeviceValueCallback(const std::function<void(DeviceValue, double,int)>& f) ;
	void RegisterDeviceSNCallback(const std::function<void(std::string,int)>& f) ;
	bool ManuallyTrigger();
	bool ManuallyTest();
	// «–ªª…Ë±∏
	void SwitchToDevice(int deviceIndex);

private:
	static void LoopAutoConnect();
	static bool m_bLinked;

	static int FuncLinkCallBack(char nEvent);
	static int FuncBreakCallBack(char nEvent);
	static int FuncImageCallBack(char nEvent);
	static int FuncHeartBeatCallBack(char nEvent);

	static int FuncLinkupCallBackEx(INT16 nEvent, char index);
	static int FuncBreakCallBackEx(INT16 nEvent, char index);
	static int FuncHeartBeatCallBackEx(INT16 nEvent, char index);
	static int FuncImageCallBackEx(INT16 nEvent, char index);

	static std::function<void(uint8_t*, int, int, int,int)> m_function_ImgCallBack;
	static std::function<void(DeviceConnectStatus, int)> m_function_DeviceConnectStatusCallBack;
	static std::function<void(DeviceRunStatus, int)> m_function_DeviceRunStatusCallBack;
	static std::function<void(DeviceValue, double, int)> m_function_DeviceValueCallBack;
	static std::function<void(std::string, int)> m_function_DevicSNCallBack;
	static std::vector<uint8_t> m_vector_ImgBuffer;
	static bool m_bIsInited;
	static bool m_bIsNeedExit;
	static bool m_bSoftwareTrigger;
	static int m_nMultiBdTpye;
	static int m_nInitStatus;
	static int8_t m_cFpCurStat;

	static std::vector<std::string> m_vector_DeviceSN;

	static std::string m_str_DeviceSN;
};
