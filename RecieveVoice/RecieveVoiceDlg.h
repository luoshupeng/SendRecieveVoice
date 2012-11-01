
// RecieveVoiceDlg.h : 头文件
//

#pragma once
#include <MMSystem.h>

#define BLOCK_PER_BUFFER	3				//接收语音数据块
#define FRAME_NUM			8				//采样精确度
#define PCMBUFFER_SIZE		360*FRAME_NUM	//录音缓冲区大小
#define WM_DATACOME			WM_USER+1		//接收数据
#define WM_PLAY				WM_USER+2		//播放语音

// CRecieveVoiceDlg 对话框
class CRecieveVoiceDlg : public CDialogEx
{
// 构造
public:
	CRecieveVoiceDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_RECIEVEVOICE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 窗口句柄
	HWND hwnd;
	// 放音缓冲区
	char*	pOutBuffer1;
	char*	pOutBuffer2;
	// 设备标志
	HWAVEOUT hWaveOut;
	// 设备设置头
	PWAVEHDR	pWaveOut;
	// 设备返回结果
	MMRESULT	result;
	// 音频格式头
	WAVEFORMATEX	format;

	// 接收Socket
	SOCKET m_SforVoiceOut;
	// 接收地址
	SOCKADDR_IN	m_AcceptAddr;

	// 播放缓冲区序号
	BOOL	m_bPlayFirst;


	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnDataCome(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPlay(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWomDone(WPARAM wParam, LPARAM lParam);
	CRITICAL_SECTION m_section;
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDestroy();

protected:
	/// 语音输入
	SOCKET m_SforVoiceIn;
	SOCKADDR_IN m_SendAddr;
	int m_nSendAddrLen;
	HWAVEIN		m_hWavein;
	LPWAVEHDR	m_lpHdrIn1;
	LPWAVEHDR	m_lpHdrIn2;
	char*		m_pBufIn1;
	char*		m_pBufIn2;
	afx_msg LRESULT OnRecordFull(WPARAM wParam, LPARAM lParam);

	//音量控制
	HMIXER m_hMixer;
	// MIXERCONTROL结构中的dwControlID
	DWORD m_dwControlID;
	afx_msg LRESULT OnVolumeChanged(WPARAM wParam, LPARAM lParam);
	// 初始化音量控制
	BOOL MixerInit(DWORD& dwMin, DWORD& dwMax);
	// 获取音量
	DWORD GetVolume(HMIXER hMixer, DWORD dwControlID);
	// 设备音量
	void SetVolume(HMIXER hMixer, DWORD dwControlID, DWORD dwVolume);
};
