#include <windows.h>
#include "MultiPZMedical.h"
#include "NetCom.h"

#ifndef _DEBUG
#include <iostream>
#include <sstream>
#endif

#include <cstring>
#include <vector>
#include <thread>

#define FPDTOTALNUM 2   //total number of FPD

std::function<void(uint8_t*, int, int, int, int)> CPZMultiMedical::m_function_ImgCallBack = nullptr;

std::function<void(DeviceConnectStatus, int)> CPZMultiMedical::m_function_DeviceConnectStatusCallBack = nullptr;

std::function<void(DeviceRunStatus, int)> CPZMultiMedical::m_function_DeviceRunStatusCallBack = nullptr;

std::function<void(DeviceValue, double, int)> CPZMultiMedical::m_function_DeviceValueCallBack = nullptr;

std::function<void(std::string, int)> CPZMultiMedical::m_function_DevicSNCallBack = nullptr;

std::vector<uint8_t> CPZMultiMedical::m_vector_ImgBuffer = {};

std::vector<std::string> CPZMultiMedical::m_vector_DeviceSN = { "","" };
//std::string CPZMultiMedical::m_str_DeviceSN = "";
//CHAR FPSn[FPDTOTALNUM][32] = { '\0' };
//CHAR FPSnOpened[32] = { 0 };

int index = 0;

bool CPZMultiMedical::m_bIsInited = false;
bool CPZMultiMedical::m_bLinked = false;
bool CPZMultiMedical::m_bIsNeedExit = false;
bool CPZMultiMedical::m_bSoftwareTrigger = false;
int8_t CPZMultiMedical::m_cFpCurStat = 0;
int CPZMultiMedical::m_nInitStatus = 0;

CPZMultiMedical::CPZMultiMedical()
{
	m_bIsNeedExit = false;
}

bool CPZMultiMedical::Init(int nConfigType)
{
	m_bSoftwareTrigger = nConfigType > 0;
	if (m_bIsInited)
	{
		return false;
	}
	COM_RegisterEvCallBack(EVENT_LINKUP, FuncLinkCallBack);
	COM_RegisterEvCallBack(EVENT_LINKDOWN, FuncBreakCallBack);
	COM_RegisterEvCallBack(EVENT_IMAGEVALID, FuncImageCallBack);
	COM_RegisterEvCallBack(EVENT_HEARTBEAT, FuncHeartBeatCallBack);

	if (COM_Init())
	{
		if (COM_SetCalibMode(6))
		{

			m_bIsInited = true;
		}
	}

	return m_bIsInited;
}

// 切换到指定索引的设备
void CPZMultiMedical::SwitchToDevice(int deviceIndex)
{
	if (deviceIndex >= 0 && deviceIndex < FPDTOTALNUM) {
		if (m_vector_DeviceSN.size() > 0) {
			std::cout << "Switching to device " << deviceIndex << " with SN: " << m_vector_DeviceSN[deviceIndex] << std::endl;
			// 关闭当前设备（如果已打开）
			COM_Close();
			index = deviceIndex;

			//m_str_DeviceSN= m_vector_DeviceSN[deviceIndex];

		}
		else {
			std::cout << "Device " << deviceIndex << " SN not available. Please initialize first." << std::endl;
		}
	}
	else {
		std::cout << "Invalid device index. Must be between 0 and " << (FPDTOTALNUM - 1) << std::endl;
	}
}

bool CPZMultiMedical::BeginWork()
{
	if (m_function_DeviceConnectStatusCallBack != nullptr)
	{
		m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::UnKnown, 0);
		m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::UnKnown, 1);
	}

	if (COM_SetCalibMode(IMG_CALIB_GAIN | IMG_CALIB_DEFECT))
	{
		m_nInitStatus = 0;
		std::cout << "Calibration mode set successfully, starting connection loop" << std::endl;
		std::thread td(LoopAutoConnect);
		td.detach();
		return true;
	}
	std::cout << "Failed to set calibration mode" << std::endl;
	return false;
}

void CPZMultiMedical::LoopAutoConnect()
{
	while (!m_bIsNeedExit)
	{
		Sleep(500);

		// 获取设备列表
		TComFpList tComFpList = { 0 };
		COM_List(&tComFpList);

		char cnt = tComFpList.ncount;
		//std::cout << "Found " << (int)cnt << " devices" << std::endl;
		if ((int)cnt == 1 || (int)cnt == 2)
		{
			m_vector_DeviceSN.clear();
			for (char i = 0; i < cnt; i++)
			{
				std::string snString(tComFpList.tFpNode[i].FPPsn);
				m_function_DevicSNCallBack(snString, i);
				m_vector_DeviceSN.push_back(snString);
				//std::cout << "Device " << (int)i << " SN: " << snString << std::endl;
			}
		}
		else
		{
			std::cout << "Invalid device count: " << (int)cnt << std::endl;
			continue;
		}

		switch (m_nInitStatus)
		{
		case 0:
		{
			const CHAR* psn;
			if (m_vector_DeviceSN.size() > index)
			{
				psn = m_vector_DeviceSN.at(index).c_str();
			}
			else
			{
				psn = "";
			}
			std::cout << "m_nInitStatus =" << m_nInitStatus << std::endl;
			if (COM_Open(const_cast<CHAR*>(psn)))
			{
				m_nInitStatus = 1;
				std::cout << "PZ COM_Open Sucecss" << std::endl;
			}
			else
			{
				std::cout << "PZ COM_Open Failed" << std::endl;
			}
			break;
		}
		case 1:
		{
			if (COM_Stop())
			{
				m_nInitStatus = 2;
			}
			break;
		}
		case 2:
		{
			bool reb = false;
			if (m_bSoftwareTrigger)
			{
				reb = COM_HstAcq();
			}
			else
			{
				reb = COM_AedAcq();
			}
			if (reb)
			{
				m_nInitStatus = 3;
			}
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

bool CPZMultiMedical::EndWork()
{
	return COM_Close();
}

void CPZMultiMedical::RegisterImgCallback(
	const std::function<void(uint8_t*, int, int, int, int)>& f)
{
	m_function_ImgCallBack = f;
}

void CPZMultiMedical::RegisterDeviceConnectCallback(const std::function<void(DeviceConnectStatus, int)>& f)
{
	m_function_DeviceConnectStatusCallBack = f;
}

void CPZMultiMedical::RegisterDeviceRunCallback(const std::function<void(DeviceRunStatus, int)>& f)
{
	m_function_DeviceRunStatusCallBack = f;
}

void CPZMultiMedical::RegisterDeviceValueCallback(const std::function<void(DeviceValue, double, int)>& f)
{
	m_function_DeviceValueCallBack = f;
}

void CPZMultiMedical::RegisterDeviceSNCallback(const std::function<void(std::string, int)>& f)
{
	m_function_DevicSNCallBack = f;
}

bool CPZMultiMedical::ManuallyTrigger()
{
	return COM_Trigger();
}

bool CPZMultiMedical::ManuallyTest()
{
	// 测试执行所有的槽函数
	if (m_function_DeviceRunStatusCallBack != nullptr)
	{
		m_function_DeviceRunStatusCallBack(DeviceRunStatus::Awake, 0);
		m_function_DeviceRunStatusCallBack(DeviceRunStatus::Awake, 1);
	}
	if (m_function_DeviceValueCallBack != nullptr)
	{
		m_function_DeviceValueCallBack(DeviceValue::Battery, 98.0, 0);
		m_function_DeviceValueCallBack(DeviceValue::Battery, 97.0, 1);
		m_function_DeviceValueCallBack(DeviceValue::Temperature, 22.5, 0);
		m_function_DeviceValueCallBack(DeviceValue::Temperature, 23.5, 1);
		m_function_DeviceValueCallBack(DeviceValue::Wifi, 16.0, 0);
		m_function_DeviceValueCallBack(DeviceValue::Wifi, 17.0, 1);
	}
	if (m_function_DeviceConnectStatusCallBack != nullptr)
	{
		m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::UnKnown, 0);
		m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::UnKnown, 1);
	}
	if (m_function_DevicSNCallBack != nullptr)
	{
		m_function_DevicSNCallBack("PZ_SN_1", 0);
		m_function_DevicSNCallBack("PZ_SN_2", 1);
	}

	return true;
}

int CPZMultiMedical::FuncLinkCallBack(char nEvent)
{
	m_bLinked = true;
#ifndef _DEBUG
	std::cout << "PZ FuncLinkCallBack" << std::endl;
#endif
	CHAR FPSnOpened[32] = { 0 };
	COM_GetFPsn(FPSnOpened);

	TComFpList tComFpList = { 0 };
	COM_List(&tComFpList);

	for (CHAR i = 0; i < FPDTOTALNUM; i++)
	{
		if (0 == strncmp(FPSnOpened, tComFpList.tFpNode[i].FPPsn, 32))
		{
			if (m_function_DeviceConnectStatusCallBack != nullptr)
			{
				m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::OpendListen, index);
			}
		}
		else
		{
			if (m_function_DeviceConnectStatusCallBack != nullptr)
			{
				m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::Connected, index);
			}
		}
	}
	return 0;
}

int CPZMultiMedical::FuncBreakCallBack(char nEvent)
{
	m_bLinked = false;
	m_nInitStatus = 0;
#ifndef _DEBUG
	std::cout << "PZ FuncBreakCallBackEx" << std::endl;
#endif

	CHAR acSn[32] = { '\0' };
	COM_GetFPsn(acSn);
	CHAR cFpCurStat = COM_GetFPCurStatus();
	TFPStat tFPStat = { 0 };
	COM_GetFPStatus(&tFPStat);

	if (m_function_DeviceConnectStatusCallBack != nullptr)
	{
		m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::Closed, index);
	}
	return 0;
}

int CPZMultiMedical::FuncImageCallBack(char nEvent)
{
#ifndef _DEBUG
	std::cout << "PZ FuncImageCallBackEx" << std::endl;
#endif
	TImageMode tImageMode;
	COM_GetImageMode(&tImageMode);
	auto u16ImgRow = tImageMode.usRow;
	auto u16ImgCol = tImageMode.usCol;
	if (m_vector_ImgBuffer.size() != u16ImgRow * u16ImgCol * sizeof(UINT16))
	{
		m_vector_ImgBuffer.resize(u16ImgRow * u16ImgCol * sizeof(UINT16));
	}
	COM_GetImage((CHAR*)m_vector_ImgBuffer.data());

	if (m_function_ImgCallBack != nullptr)
	{
		m_function_ImgCallBack(m_vector_ImgBuffer.data(), u16ImgCol, u16ImgRow, sizeof(UINT16), index);
	}
	return 0;
}

int CPZMultiMedical::FuncHeartBeatCallBack(char nEvent)
{
#ifndef _DEBUG
	//COM_HstAcq();
	//std::stringstream ss;
	//std::cout << "PZ FuncHeartBeatCallBackEx" << std::endl;
	//心跳，暂时还不知道怎么处理
#endif
	m_cFpCurStat = COM_GetFPCurStatus();
	TFPStat tFPStat = { 0 };
	CHAR acSn[32] = { '\0' };
	COM_GetFPsn(acSn);
	COM_GetFPStatus(&tFPStat);

	std::string auctmp = "Sn:";
	auctmp += acSn;

	if (m_function_DeviceRunStatusCallBack != nullptr)
	{
		switch (m_cFpCurStat)
		{
		case STATUS_IDLE:
		{
			m_function_DeviceRunStatusCallBack(DeviceRunStatus::Awake, index);
			if (!m_bSoftwareTrigger)
			{
				m_nInitStatus = 2;
			}
			break;
		}
		case STATUS_HST:
		case STATUS_AED1:
		case STATUS_AED2:
		{
			m_function_DeviceRunStatusCallBack(DeviceRunStatus::Ready, index);
			break;
		}
		default:
		{
			m_function_DeviceRunStatusCallBack(DeviceRunStatus::Unknow, index);
			break;
		}
		}
	}
	if (m_function_DeviceValueCallBack != nullptr)
	{
		if (0 != (tFPStat.tBatInfo1.full + tFPStat.tBatInfo2.full))
		{
			double value = 100 * (tFPStat.tBatInfo1.Remain + tFPStat.tBatInfo2.Remain) / (tFPStat.tBatInfo1.full +
				tFPStat.tBatInfo2.full);
			m_function_DeviceValueCallBack(DeviceValue::Battery, value, index);
		}
		//double humValue = double(tFPStat.tFpTempHum.Hum) / 10;
		m_function_DeviceValueCallBack(DeviceValue::Temperature, tFPStat.tFpTempHum.Temp / 10, index);
		m_function_DeviceValueCallBack(DeviceValue::Wifi, tFPStat.tWifiStatus.ucSignal_level, index);
	}
	return 0;
}
