
// SendVoiceDlg.h : ͷ�ļ�
//

#pragma once

#include <MMSystem.h>

#define BLOCK_PER_BUFFER	3				//�����������ݿ�
#define FRAME_NUM			8				//������ȷ��
/*�����������С�Ƿ��ͻ�������С���������1K��UDP��sendto������ְ���
* ��������sendtoĿ�겻������ʱsendto�������ӳٷ��أ��Ӷ���ɽ��濨*/
#define PCMBUFFER_SIZE		360*FRAME_NUM	//¼����������С
#define WM_DATACOME			WM_USER+1		//��������
#define WM_PLAY				WM_USER+2		//��������

#include <vector>
using namespace std;

// CSendVoiceDlg �Ի���
class CSendVoiceDlg : public CDialogEx
{
// ����
public:
	CSendVoiceDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_SENDVOICE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ھ��
	HWND hwnd;
	//¼��������
	char*	pInBuffer1;
	char*	pInBuffer2;
	// �豸��־
	HWAVEIN hWaveIn;
	// �豸����ͷ
	LPWAVEHDR	pWaveHdr1;
	LPWAVEHDR	pWaveHdr2;
	// �豸���ؽ��
	MMRESULT	result;
	// ��Ƶ��ʽͷ
	WAVEFORMATEX	format;

	// ����Socket
	SOCKET m_SforVoiceIn;
	// ���͵�ַ
	//SOCKADDR_IN	m_sendAddr;
	vector<SOCKADDR_IN> m_sendAddrVec;

	// WAV�ļ�������
	char*	m_dataBuf;
	// WAV�ļ�����������
	DWORD	m_dataLen;

	//��������
	HMIXER m_hMixer;
	// MIXERCONTROL�ṹ�е�dwControlID
	DWORD m_dwControlID;

	// ���ɵ���Ϣӳ�亯��
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
	// ����WAV�ļ�
	void SaveWavFile(void);
	// ��������WAV�ļ�
	void SaveWavFile(char* data, UINT datalen);
public:
	afx_msg void OnCheckAll();
	afx_msg void OnCheckSaveFile();
	afx_msg void OnButtonBrowser();
private:
	// �Ƿ��ڽ���ʱ�����ļ�
	BOOL m_bSaveFile;
	// ����ʱ�����ļ����ļ�·��
	CString m_sSaveFile;
public:
	afx_msg void OnCheckT1();
	afx_msg void OnCheckT2();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
//	afx_msg void OnBnClickedButton1();
	afx_msg LRESULT OnVolumeChanged(WPARAM wParam, LPARAM lParam);
	// ��ʼ����������
	BOOL MixerInit(DWORD& dwMin, DWORD& dwMax);
	// ��ȡ����
	DWORD GetVolume(HMIXER hMixer, DWORD dwControlID);
	// �豸����
	void SetVolume(HMIXER hMixer, DWORD dwControlID, DWORD dwVolume);

protected:
	//�������
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
