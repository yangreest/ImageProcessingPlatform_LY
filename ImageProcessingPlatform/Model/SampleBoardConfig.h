#pragma once

class CSampleBoardConfig
{
public:
	CSampleBoardConfig();

	/// <summary>
	/// 厂家
	/// </summary>
	int m_nManufacturer;

	/// <summary>
	/// 型号
	/// </summary>
	int m_nModel;

	/// <summary>
	/// 图片类型
	/// </summary>
	int m_nMapType;

	/// <summary>
	/// 曝光模式 0-AED触发 1-手动
	/// </summary>
	int m_nExposureType;

	///	<summary>
	/// 多设备类型 0-指定的是老型号，1指定新型号
	/// <summary>
	int m_nMultiBoardType;

	/// <summary>
	/// 曝光时间
	/// </summary>
	int m_nExposureTime;
};
