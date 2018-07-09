#include <windows.h>
#include <tchar.h>
#include <TLHELP32.H>

#include "resource.h"

DWORD FindTarget( LPCTSTR lpszProcess )
{
    DWORD dwRet = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof( PROCESSENTRY32 );
    Process32First( hSnapshot, &pe32 );
    do
    {
        if ( lstrcmpi( pe32.szExeFile, lpszProcess ) == 0 )
        {
            dwRet = pe32.th32ProcessID;
            break;
        }
    } while ( Process32Next( hSnapshot, &pe32 ) );
    CloseHandle( hSnapshot );
    return dwRet;
}

BOOL RemoteLoadLibrary( DWORD dwProcessID, LPCSTR lpszDll )
{
    // ��Ŀ�����
    HANDLE hProcess = OpenProcess( PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, dwProcessID );
    // ��Ŀ����̵�ַ�ռ�д��DLL����
    DWORD dwSize, dwWritten;
    dwSize = lstrlenA( lpszDll ) + 1;
    LPVOID lpBuf = VirtualAllocEx( hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE );
    if ( NULL == lpBuf )
    {
        CloseHandle( hProcess );
        return FALSE;
    }
    if ( WriteProcessMemory( hProcess, lpBuf, (LPVOID)lpszDll, dwSize, &dwWritten ) )
    {
        // Ҫд���ֽ�����ʵ��д���ֽ�������ȣ�����ʧ��
        if ( dwWritten != dwSize )
        {
            VirtualFreeEx( hProcess, lpBuf, dwSize, MEM_DECOMMIT );
            CloseHandle( hProcess );
            return FALSE;
        }
    }
    else
    {
        CloseHandle( hProcess );
        return FALSE;
    }
    // ʹĿ����̵���LoadLibrary������DLL
    DWORD dwID;
    LPVOID pFunc = LoadLibraryA;
    HANDLE hThread = CreateRemoteThread( hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, lpBuf, 0, &dwID );
    // �ȴ�LoadLibrary�������
    WaitForSingleObject( hThread, INFINITE );
    // �ͷ�Ŀ�����������Ŀռ�
    VirtualFreeEx( hProcess, lpBuf, dwSize, MEM_DECOMMIT );
    CloseHandle( hThread );
    CloseHandle( hProcess );
    return TRUE;
}

BOOL RemoteFreeLibrary( DWORD dwProcessID, LPCSTR lpszDll )
{
    // ��Ŀ�����
    HANDLE hProcess = OpenProcess( PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, dwProcessID );
    // ��Ŀ����̵�ַ�ռ�д��DLL����
    DWORD dwSize, dwWritten;
    dwSize = lstrlenA( lpszDll ) + 1;
    LPVOID lpBuf = VirtualAllocEx( hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE );
    if ( NULL == lpBuf )
    {
        CloseHandle( hProcess );
        return FALSE;
    }
    if ( WriteProcessMemory( hProcess, lpBuf, (LPVOID)lpszDll, dwSize, &dwWritten ) )
    {
        // Ҫд���ֽ�����ʵ��д���ֽ�������ȣ�����ʧ��
        if ( dwWritten != dwSize )
        {
            VirtualFreeEx( hProcess, lpBuf, dwSize, MEM_DECOMMIT );
            CloseHandle( hProcess );
            return FALSE;
        }
    }
    else
    {
        CloseHandle( hProcess );
        return FALSE;
    }
    // ʹĿ����̵���GetModuleHandle�����DLL��Ŀ������еľ��
    DWORD dwHandle, dwID;
    LPVOID pFunc = GetModuleHandleA;
    HANDLE hThread = CreateRemoteThread( hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, lpBuf, 0, &dwID );
    // �ȴ�GetModuleHandle�������
    WaitForSingleObject( hThread, INFINITE );
    // ���GetModuleHandle�ķ���ֵ
    GetExitCodeThread( hThread, &dwHandle );
    // �ͷ�Ŀ�����������Ŀռ�
    VirtualFreeEx( hProcess, lpBuf, dwSize, MEM_DECOMMIT );
    CloseHandle( hThread );
    // ʹĿ����̵���FreeLibrary��ж��DLL
    pFunc = FreeLibrary;
    hThread = CreateRemoteThread( hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, (LPVOID)dwHandle, 0, &dwID );
    // �ȴ�FreeLibraryж�����
    WaitForSingleObject( hThread, INFINITE );
    CloseHandle( hThread );
    CloseHandle( hProcess );
    return TRUE;
}

int CALLBACK MainDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static DWORD dwProcessID;
    switch ( uMsg )
    {
    case WM_INITDIALOG:
        {
            dwProcessID = 0;
            SendDlgItemMessage( hDlg, IDC_EDT_TARGET, EM_LIMITTEXT, MAX_PATH, 0 );
        }
        break;
    case WM_COMMAND:
        {
            switch ( LOWORD( wParam ) )
            {
            case IDC_BTN_EXIT:
                {
                    EndDialog( hDlg, 0 );
                }
                break;
            case IDC_BTN_INSERT:
                {
                    TCHAR szTarget[MAX_PATH];
                    GetDlgItemText( hDlg, IDC_EDT_TARGET, szTarget, MAX_PATH );
                    dwProcessID = FindTarget( szTarget );
                    if ( 0 == dwProcessID )
                    {
                        MessageBox( hDlg, _T("�Ҳ���Ŀ����̡�"), _T("����"), MB_ICONINFORMATION );
                        break;
                    }
                    if ( !RemoteLoadLibrary( dwProcessID, "DLL.dll" ) )
                    {
                        MessageBox( hDlg, _T("Զ��DLL����ʧ�ܡ�"), _T("����"), MB_ICONINFORMATION );
                    }
                }
                break;
            case IDC_BTN_DETACH:
                {
                    if ( 0 == dwProcessID )
                    {
                        MessageBox( hDlg, _T("�Ҳ���Ŀ����̡�"), _T("����"), MB_ICONINFORMATION );
                        break;
                    }
                    if ( !RemoteFreeLibrary( dwProcessID, "DLL.dll" ) )
                    {
                        MessageBox( hDlg, _T("Զ��DLLж��ʧ�ܡ�"), _T("����"), MB_ICONINFORMATION );
                    }
                }
                break;
            }
        }
        break;
    case WM_CLOSE:
        {
            EndDialog( hDlg, 0 );
        }
        break;
    }
    return 0;
}

int WINAPI _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd )
{
    return DialogBox( hInstance, MAKEINTRESOURCE( IDD_MAIN_DLG ), NULL, MainDlgProc );
}