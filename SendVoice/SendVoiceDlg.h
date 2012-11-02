
// SendVoiceDlg.h : 头文件
//

#pragma once

#include <MMSystem.h>

#define BLOCK_PER_BUFFER	3				//接收语音数据块
#define FRAME_NUM			8				//采样精确度
/*这个缓冲区大小是发送缓冲区大小，如果大于1K则UDP的sendto函数会分包，
* 导致在若sendto目标不可连接时sendto函数会延迟返回，从而造成界面卡*/
#define PCMBUFFER_SIZE		360*FRAME_NUM	//录音缓冲区大小
#define WM_DATACOME			WM_USER+1		//接收数据
#define WM_PLAY				WM_USER+2		//播放声音

#include <vector>
using namespace std;

// CSendVoiceDlg 对话框
class CSendVoiceDlg : public CDialogEx
{
// 构造
public:
	CSendVoiceDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SENDVOICE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 窗口句柄
	HWND hwnd;
	//录音缓冲区
	char*	pInBuffer1;
	char*	pInBuffer2;
	// 设备标志
	HWAVEIN hWaveIn;
	// 设备设置头
	LPWAVEHDR	pWaveHdr1;
	LPWAVEHDR	pWaveHdr2;
	// 设备返回结果
	MMRESULT	result;
	// 音频格式头
	WAVEFORMATEX	format;

	// 发送Socket
	SOCKET m_SforVoiceIn;
	// 发送地址
	//SOCKADDR_IN	m_sendAddr;
	vector<SOCKADDR_IN> m_sendAddrVec;

	// WAV文件缓冲区
	char*	m_dataBuf;
	// WAV文件缓冲区长度
	DWORD	m_dataLen;

	//音量控制
	HMIXER m_hMixer;
	// MIXERCONTROL结构中的dwControlID
	DWORD m_dwControlID;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSend();
	afx_msg LRESULT OnRecordFull(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRecordClose(WPARAM wParam, LPARAM lParam);
	virtual void OnCancel();
private:
	// 保存WAV文件
	void SaveWavFile(void);
	// 增量保存WAV文件
	void SaveWavFile(char* data, UINT datalen);
public:
	afx_msg void OnCheckAll();
	afx_msg void OnCheckSaveFile();
	afx_msg void OnButtonBrowser();
private:
	// 是否在讲话时保存文件
	BOOL m_bSaveFile;
	// 讲话时保存文件的文件路径
	CString m_sSaveFile;
public:
	afx_msg void OnCheckT1();
	afx_msg void OnCheckT2();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
//	afx_msg void OnBnClickedButton1();
	afx_msg LRESULT OnVolumeChanged(WPARAM wParam, LPARAM lParam);
	// 初始化音量控制
	BOOL MixerInit(DWORD& dwMin, DWORD& dwMax);
	// 获取音量
	DWORD GetVolume(HMIXER hMixer, DWORD dwControlID);
	// 设备音量
	void SetVolume(HMIXER hMixer, DWORD dwControlID, DWORD dwVolume);

protected:
	//语音输出
	HWAVEOUT	m_hWaveOut;
	LPWAVEHDR	m_lpHdrOut;
	char*		m_pBufOut1;
	char*		m_pBufOut2;
	SOCKET		m_SforVoiceOut;
	SOCKADDR_IN	m_AcceptAddr;
	int			m_nAcceptAddrLen;
	BOOL		m_bPlayFirst;
	afx_msg LRESULT OnDataCome(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPlay(WPARAM wParam, LPARAM lParam);
};
