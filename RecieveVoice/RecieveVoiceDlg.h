
// RecieveVoiceDlg.h : ͷ�ļ�
//

#pragma once
#include <MMSystem.h>

#define BLOCK_PER_BUFFER	3				//�����������ݿ�
#define FRAME_NUM			8				//������ȷ��
#define PCMBUFFER_SIZE		360*FRAME_NUM	//¼����������С
#define WM_DATACOME			WM_USER+1		//��������
#define WM_PLAY				WM_USER+2		//��������

// CRecieveVoiceDlg �Ի���
class CRecieveVoiceDlg : public CDialogEx
{
// ����
public:
	CRecieveVoiceDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_RECIEVEVOICE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ھ��
	HWND hwnd;
	// ����������
	char*	pOutBuffer1;
	char*	pOutBuffer2;
	// �豸��־
	HWAVEOUT hWaveOut;
	// �豸����ͷ
	PWAVEHDR	pWaveOut;
	// �豸���ؽ��
	MMRESULT	result;
	// ��Ƶ��ʽͷ
	WAVEFORMATEX	format;

	// ����Socket
	SOCKET m_SforVoiceOut;
	// ���յ�ַ
	SOCKADDR_IN	m_AcceptAddr;

	// ���Ż��������
	BOOL	m_bPlayFirst;


	// ���ɵ���Ϣӳ�亯��
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
};
