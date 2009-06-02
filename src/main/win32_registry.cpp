// win32_registry.cpp

#include "stdafx.h"
#include "string.hpp"
#include "win32.hpp"

//==============================================================================
// ���W�X�g������֐�.

HRESULT	RegGetString(WCHAR ret[MAX_PATH], HKEY hKey, PCWSTR subkey, PCWSTR value)
{
	if (!ret)
		return E_POINTER;
	DWORD type = REG_SZ;
	DWORD bufSize = (DWORD)(sizeof(WCHAR) * MAX_PATH);
	DWORD dwResult = SHGetValueW(hKey, subkey, value, &type, ret, &bufSize);
	if (dwResult == ERROR_SUCCESS)
		return S_OK;
	else
		return HRESULT_FROM_WIN32(dwResult);
}

HRESULT	RegGetStringV(WCHAR ret[MAX_PATH], HKEY hKey, PCWSTR subkey, PCWSTR value, ...)
{
	HRESULT	hr;
	WCHAR	buffer[1024];
	va_list	args;

	va_start(args, value);
	vswprintf_s(buffer, subkey, args);
	hr = RegGetString(ret, hKey, buffer, value);
	va_end(args);

	return hr;
}

// �ǂ�������݂���t�@�C���łȂ��ƃG���[�ɂȂ�̂Ŏg���Ȃ��I
//	if (FindExecutable(s, null, exe) > (HINSTANCE)32)
//	if SUCCEEDED(hr = AssocQueryString(ASSOCF_OPEN_BYEXENAME | ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, s, L"open"), exe, &ln))
HRESULT RegGetAssocExe(WCHAR exe[MAX_PATH], PCWSTR extension)
{
	if (str::empty(extension))
		return HRESULT_FROM_WIN32(ERROR_REGISTRY_IO_FAILED);

	HRESULT hr;
	WCHAR	key[MAX_PATH];
	WCHAR	value[MAX_PATH];

	if SUCCEEDED(RegGetStringV(value, HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s\\UserChoice",
		L"Progid", extension))
	{
		// �G�N�X�v���[���̊֘A�t������擾�ł��� exe ���g�p����B
		swprintf_s(key, L"%s\\shell", value);
	}
	else if SUCCEEDED(RegGetStringV(value, HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s",
		L"Application", extension))
	{
		// .exe ���w��̃p�^�[��������悤���B
		swprintf_s(key, L"Applications\\%s\\shell", value);
	}
	else
	{
		// �����Ȃ���Β��� HKEY_CLASSES_ROOT ����擾����B
		swprintf_s(key, L"%s\\shell", extension);
	}

	// �f�t�H���g�A�N�V�������擾����B���݂��Ȃ���� open ���g���B
	hr = RegGetString(value, HKEY_CLASSES_ROOT, key, null);
	wcscat_s(key, L"\\");
	if SUCCEEDED(hr)
		wcscat_s(key, value);
	else
		wcscat_s(key, L"open");
	wcscat_s(key, L"\\command");

	// �f�t�H���g�A�N�V�������� exe �p�X�����擾����B
	if FAILED(hr = RegGetString(value, HKEY_CLASSES_ROOT, key, null))
		return hr;
	if (str::empty(value))
		return HRESULT_FROM_WIN32(ERROR_REGISTRY_IO_FAILED);

	// ���A'EXE "%1"' ���낤�̂ŁA�͂��߂̃G���g���������擾����
	PCWSTR src = value;
	WCHAR delim = L' ';
	if (src[0] == L'"')
	{
		++src; 
		delim = L'\"';
	}
	PWSTR dst = exe;
	while (*src != L'\0' && *src != delim) { *dst++ = *src++; }
	*dst = L'\0';
	return S_OK;
}
