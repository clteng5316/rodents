// win32_thread.cpp
#include "stdafx.h"
#include <set>
#include "string.hpp"
#include "win32.hpp"
#include "window.hpp"
#include "unknown.hpp"

class Thread : public Unknown< implements<IDropTarget>, mixin<StaticLife> >
{
	enum MouseState
	{
		MouseEnabled,	// �ʏ�i�W�F�X�`�����Ă��Ȃ��j
		MouseGesture,	// �W�F�X�`����
		MouseDisabled,	// �}�E�X�G�~�����[�V������
	};

public:
	Hwnd			m_hwnd;			// �X���b�h���C���E�B���h�E
	HHOOK			m_hookMSG;		// WH_GETMESSAGE �t�b�N
	HHOOK			m_hookCBT;		// WH_CBT �t�b�N
	HWND			m_hwndWheel;	// �}�E�X�z�C�[���̃��_�C���N�g��
	bool			m_needsUpdate;
	SUBCLASSPROC	wndprocMain;
	WNDPROC			wndprocSave;
	MouseState		m_mouseState;

	Window*					m_dropTarget;
	ref<IDataObject>		m_dropData;
	RECT					m_dropBounds;
	ref<IDropTargetHelper>	m_dropHelper;

public:
	void	run(Window* form) throw();
	void	mouseDrag(UINT mk, POINT from, POINT to) throw();
	void	mouseUp(UINT mk, POINT pt) throw();
	void	markUpdate() throw();

private:
	void	onMessage(MSG& msg) throw();
	bool	onKeyDown(const MSG& msg) throw();
	bool	onMouseDown(const MSG& msg) throw();
	bool	onMouseUp(const MSG& msg) throw();
	void	onMouseWheel(MSG& msg) throw();
	void	doUpdate() throw();
	static LRESULT CALLBACK onMessage(int code, WPARAM wParam, LPARAM lParam) throw();
	static LRESULT CALLBACK onCBT(int code, WPARAM wParam, LPARAM lParam) throw();

public: // IDropTarget
	STDMETHODIMP DragEnter(IDataObject* data, DWORD keys, POINTL pt, DWORD* effect);
	STDMETHODIMP DragOver(DWORD keys, POINTL pt, DWORD* effect);
	STDMETHODIMP DragLeave();
	STDMETHODIMP Drop(IDataObject* data, DWORD keys, POINTL pt, DWORD* effect);
};

namespace
{
	static Thread theThread;

	//=============================================================================

	static inline void CancelMessage(MSG& msg) throw()
	{
		// ���b�Z�[�W���n���h�������̂ŁA���b�Z�[�W���̂𖳌�������K�v������B
		// �߂�l��CallNextHookEx()���Ă΂Ȃ��Ȃǂ̕��@�ł͂Ȃ��A
		// ���b�Z�[�W��ύX���āu�������Ȃ����b�Z�[�W�v�ɂ��Ă��܂��̂��ǂ��悤���B
		msg.message = WM_NULL;
	}

	static void SetINPUT(INPUT* in, const POINT* pt, UINT mk, bool down)
	{
		in->type = INPUT_MOUSE;
		if (pt)
		{
			in->mi.dx = ((DWORD)pt->x * 0xFFFF / ::GetSystemMetrics(SM_CXSCREEN));
			in->mi.dy = ((DWORD)pt->y * 0xFFFF / ::GetSystemMetrics(SM_CYSCREEN));
			in->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
		}
		switch (mk)
		{
		case MK_LBUTTON:
			in->mi.dwFlags |= (down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
			break;
		case MK_RBUTTON:
			in->mi.dwFlags |= (down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
			break;
		case MK_MBUTTON:
			in->mi.dwFlags |= (down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP);
			break;
		case MK_XBUTTON1:
			in->mi.dwFlags |= (down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP);
			in->mi.mouseData = XBUTTON1;
			break;
		case MK_XBUTTON2:
			in->mi.dwFlags |= (down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP);
			in->mi.mouseData = XBUTTON2;
			break;
		}
	}

	static HWND HwndFromPoint(POINT pt)
	{
		HWND hwnd = ::WindowFromPoint(pt);
		if (hwnd && ::GetWindowThreadProcessId(hwnd, null) == ::GetCurrentThreadId() && ::IsWindowEnabled(hwnd))
			return hwnd;
		else
			return null;
	}
}

static LRESULT CALLBACK Bootstrap(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC wndproc = theThread.wndprocSave;

	if (msg == WM_NCCREATE)
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		theThread.wndprocSave = null;
		SetWindowProc(hwnd, wndproc);
		if (theThread.wndprocMain)
		{
			SUBCLASSPROC	proc = theThread.wndprocMain;
			DWORD_PTR		data = (DWORD_PTR)cs->lpCreateParams;

			theThread.wndprocMain = null;
			if (!::SetWindowSubclass(hwnd, proc, 0, data))
				return FALSE;	// ���s
			LRESULT lResult = ::CallWindowProc(wndproc, hwnd, msg, wParam, lParam);
			if (lResult == 0)
				return lResult;
			return proc(hwnd, msg, wParam, lParam, 0, data);
		}
	}

	ASSERT(wndproc);
	return ::CallWindowProc(wndproc, hwnd, msg, wParam, lParam);
}

// wndproc �� WM_NCCREATE �̂� DefSubclassProc ���Ă�ł͂Ȃ�Ȃ��B
HWND CreateWindow(const WNDCLASSEX& wc_, const CREATESTRUCT& cs, SUBCLASSPROC wndproc) throw()
{
	WNDCLASSEX wc = wc_;

	theThread.wndprocMain = wndproc;
	theThread.wndprocSave = wc.lpfnWndProc;

	wc.lpfnWndProc = Bootstrap;
	wc.hInstance = ::GetModuleHandle(NULL);
	::RegisterClassEx(&wc); // TODO: 2�x�ڈȍ~�̌Ăяo���͎��s����̂Œ���

	HWND hwnd = ::CreateWindowEx(cs.dwExStyle, wc.lpszClassName, cs.lpszName, cs.style,
		cs.x, cs.y, cs.cx, cs.cy, cs.hwndParent, cs.hMenu, wc.hInstance, cs.lpCreateParams);
	ASSERT(hwnd);

	return hwnd;
}

namespace
{
	// TLS�ɒu���΃O���[�o���ϐ��ɂ���K�v�͂Ȃ����A��邱�Ƃ͂قƂ�ǖ������낤�̂ŁB
	DWORD	theMessageBoxThread;
	HHOOK	theMessageBoxHook;

	LRESULT CALLBACK AlertProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR)
	{
		LRESULT lResult = DefSubclassProc(hwnd, msg, wParam, lParam);

		switch (msg)
		{
		case WM_INITDIALOG:
			CenterWindow(hwnd);
			::RemoveWindowSubclass(hwnd, AlertProc, uIdSubclass);
			break;
		}

		return lResult;
	}

	inline bool IsMessageBox(HWND hwnd)
	{
		WCHAR clsname[7];
		return ::GetClassName(hwnd, clsname, 7) && wcscmp(L"#32770", clsname) == 0;
	}

	LRESULT CALLBACK MessageBoxHook(int code, WPARAM wParam, LPARAM lParam)
	{
		HHOOK hook = theMessageBoxHook;
		CWPRETSTRUCT* msg = (CWPRETSTRUCT*)lParam;
		if (WM_NCCREATE == msg->message && IsMessageBox(msg->hwnd))
		{
			::SetWindowSubclass(msg->hwnd, AlertProc, 0, 0);
			::UnhookWindowsHookEx(theMessageBoxHook);
			theMessageBoxHook = NULL;
		}
		return ::CallNextHookEx(hook, code, wParam, lParam);
	}
}

#define MB_ICONS	(MB_ICONERROR | MB_ICONQUESTION | MB_ICONWARNING | MB_ICONINFORMATION)

static PCWSTR MB_ICON_TO_TD_ICON(int type)
{
	switch (type & MB_ICONS)
	{
	case MB_ICONERROR:
		return TD_ERROR_ICON;
	case MB_ICONWARNING:
		return TD_WARNING_ICON;
	case MB_ICONQUESTION:
		return TD_SHIELD_ICON;	// XXX: ???
	case MB_ICONINFORMATION:
		return TD_INFORMATION_ICON;
	default:
		return null;
	}
}

#define MB_BUTTONS (MB_OK | MB_OKCANCEL | MB_ABORTRETRYIGNORE | MB_YESNOCANCEL | MB_YESNO | MB_RETRYCANCEL | MB_CANCELTRYCONTINUE)


static TASKDIALOG_COMMON_BUTTON_FLAGS MB_TYPE_TO_TD_TYPE(int type)
{
	switch (type & MB_BUTTONS)
	{
	case MB_OK:
		return TDCBF_OK_BUTTON;
	case MB_OKCANCEL:
		return TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
	case MB_YESNOCANCEL:
		return TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON;
	case MB_YESNO:
		return TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
	case MB_RETRYCANCEL:
		return TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON;
	case MB_ABORTRETRYIGNORE:
	case MB_CANCELTRYCONTINUE:
		return TDCBF_CANCEL_BUTTON | TDCBF_RETRY_BUTTON | TDCBF_OK_BUTTON;
	default:
		return TDCBF_OK_BUTTON;
	}
}

static int MessageDialog(HWND hwnd, PCWSTR message, PCWSTR details, PCWSTR title, UINT type) throw()
{
	static HRESULT (__stdcall *fn)(HWND, HINSTANCE, PCWSTR, PCWSTR, PCWSTR, TASKDIALOG_COMMON_BUTTON_FLAGS, PCWSTR, int*) = null;
	if (DynamicLoad(&fn, L"comctl32.dll", "TaskDialog"))
	{
		TASKDIALOG_COMMON_BUTTON_FLAGS	buttons = MB_TYPE_TO_TD_TYPE(type);
		PCWSTR	icon = MB_ICON_TO_TD_ICON(type);
		int	ret;
		if SUCCEEDED(fn(hwnd, null, title, message, details, buttons, icon, &ret))
			return ret;
		else
			return IDCANCEL;
	}
	else
	{
		// TaskDialog �����p�ł��Ȃ��� (XP?) �ł� MessageBox �ő�p����.
		StringBuffer str;
		str.append(message);
		str.append(L"\n\n");
		str.append(details);
		str.terminate();
		return ::MessageBox(hwnd, str, title, type);
	}
}

int Alert(PCWSTR message, PCWSTR details, PCWSTR title, UINT type) throw()
{
	static bool	inAlert = false;

	if (inAlert)
		return IDCANCEL; // ����q�ɂȂ�̂͂܂����B

	::ReleaseCapture();	// �O�̂��߁B

	if (!title)
		title = APPNAME;
	if ((type & MB_ICONS) == 0)
		type |= MB_ICONINFORMATION;

	int		ret = IDCANCEL;
	HWND	hwnd = theThread.m_hwnd;

	if (hwnd && !theMessageBoxHook)
	{
		DWORD thread = GetCurrentThreadId();
		theMessageBoxThread = thread;
		theMessageBoxHook = ::SetWindowsHookEx(WH_CALLWNDPROCRET, MessageBoxHook, GetModuleHandle(NULL), thread);

		__try
		{
			inAlert = true;
			ret = MessageDialog(hwnd, message, details, title, type);
		}
		__finally
		{
			inAlert = false;

			// ���ɉ�������Ă���͂������A�O�̂��ߍă`�F�b�N����B
			if (theMessageBoxHook && theMessageBoxThread == thread)
			{
				::UnhookWindowsHookEx(theMessageBoxHook);
				theMessageBoxHook = NULL;
			}
		}
	}
	else
	{
		ret = MessageDialog(NULL, message, details, title, type);
	}
	return ret;
}

// path �� ���p�X�܂��̓��W�X�g���L�[
void Sound(PCWSTR path) throw()
{
	if (str::empty(path))
	{
		::MessageBeep(0);
	}
	else
	{
		WCHAR filename[MAX_PATH];

		if (PathIsRelative(path))
			if SUCCEEDED(RegGetString(filename, HKEY_CURRENT_USER, path, null))
				path = filename;

		if (PathFileExists(path))
			::PlaySound(path, null, SND_FILENAME | SND_ASYNC | SND_NOWAIT);
	}
}

//=============================================================================

// WM_KEYDOWN or WM_SYSKEYDOWN
// true ��Ԃ��ƃ��b�Z�[�W�̓L�����Z�������
bool Thread::onKeyDown(const MSG& msg)
{
	return Window::handlePress(msg);
}

// WM_*BUTTONDOWN or WM_*BUTTONDBLCLK
// true ��Ԃ��ƃ��b�Z�[�W�̓L�����Z�������
bool Thread::onMouseDown(const MSG& msg) throw()
{
	switch (m_mouseState)
	{
	case MouseDisabled:
		return true;
	case MouseGesture:
		return false;
	default:
		break;
	}

	m_mouseState = MouseGesture;
	bool ret = Window::handleGesture(msg);
	m_mouseState = MouseEnabled;
	return ret;
}

// WM_*BUTTONUP or WM_MOUSEWHEEL
// true ��Ԃ��ƃ��b�Z�[�W�̓L�����Z�������
bool Thread::onMouseUp(const MSG& msg) throw()
{
	return m_mouseState == MouseDisabled;
}

/// �}�E�X�z�C�[�����J�[�\�������̃E�B���h�E�ɑ���.
void Thread::onMouseWheel(MSG& msg)
{
	// WM_MOUSEWHEEL/WM_MOUSEHWHEEL �̃}�E�X�ʒu�̓X�N���[�����W�n�Ȃ̂ŁA�C������K�v�Ȃ�.
	POINT pt = { GET_XY_LPARAM(msg.lParam) };
	HWND hwnd = HwndFromPoint(pt);
	if (hwnd)
		m_hwndWheel = msg.hwnd = hwnd;
	else if (::IsWindow(m_hwndWheel) && ::IsWindowEnabled(m_hwndWheel))
		msg.hwnd = m_hwndWheel;
	else
		m_hwndWheel = null;
}

void Thread::markUpdate() throw()
{
	m_needsUpdate = true;
}

void Thread::doUpdate() throw()
{
	BoardcastMessage(WM_UPDATEUISTATE);
	m_needsUpdate = false;
}

void Thread::onMessage(MSG& msg)
{
	switch (msg.message)
	{
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		if (onMouseUp(msg))
			CancelMessage(msg);
		else
			markUpdate();
		break;
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
		if (onMouseDown(msg))
			CancelMessage(msg);
		else
			markUpdate();
		break;
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		if (onMouseUp(msg)) // �P���̃z�C�[���W�F�X�`��
			CancelMessage(msg);
		else
		{
			onMouseWheel(msg);
			markUpdate();
		}
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		markUpdate();
		if (onKeyDown(msg))
			CancelMessage(msg);
		break;
	case WM_CLOSE:
		if (!msg.hwnd)
			::PostMessage(m_hwnd, WM_CLOSE, 0, 0);
		// fall down
	case WM_CHAR:
	case WM_ACTIVATEAPP:
	case WM_COMMAND:
	case WM_MENUCOMMAND:
		markUpdate();
		break;
	}

	// ���b�Z�[�W�L���[����̏ꍇ�AWM_UPDATEUISTATE ���|�X�g����B
	// QS_PAINT�݂̂̏ꍇ�͋�Ƃ݂Ȃ��BWM_PAINT��͍Ăт����֖߂��Ă��Ȃ����߁B
	if (m_needsUpdate && HIWORD(GetQueueStatus(QS_ALLINPUT & ~QS_PAINT)) == 0)
		doUpdate();
}

LRESULT CALLBACK Thread::onMessage(int code, WPARAM wParam, LPARAM lParam) throw()
{
	if (code == HC_ACTION && wParam == PM_REMOVE)
		theThread.onMessage(*(MSG*)lParam);
	return ::CallNextHookEx(theThread.m_hookMSG, code, wParam, lParam);
}

LRESULT CALLBACK Thread::onCBT(int code, WPARAM wParam, LPARAM lParam) throw()
{
	// �W�F�X�`�����Ƀ��j���[���\�����ꂽ�ꍇ�ɂ́A�ĂуW�F�X�`�����󂯓�����鏀��������B
	if (code == HCBT_CREATEWND &&
		theThread.m_mouseState == MouseGesture &&
		IsMenuWindow((HWND)wParam))
	{
		theThread.m_mouseState = MouseEnabled;
	}
	return ::CallNextHookEx(theThread.m_hookCBT, code, wParam, lParam);
}

extern void RegisterHotKeys();
extern void UnregisterHotKeys();

void Thread::mouseDrag(UINT mk, POINT from, POINT to) throw()
{
	INPUT in[2] = { 0 };

	// ���̈ʒu�ŗ����B����ɂ�郁�b�Z�[�W�̓]����}������B
	m_mouseState = MouseDisabled;
	SetINPUT(&in[0], null, mk, false);
	::SendInput(1, in, sizeof(INPUT));
	PumpMessage();
	// �h���b�O�̃G�~�����[�V�����B
	m_mouseState = MouseGesture;
	SetINPUT(&in[0], &from, mk, true);	// �ŏ��̈ʒu�ŃN���b�N
	SetINPUT(&in[1], &to, 0, false);	// ���̈ʒu�ֈړ�
	::SendInput(2, in, sizeof(INPUT));
	PumpMessage();
}

void Thread::mouseUp(UINT mk, POINT pt) throw()
{
	INPUT in[2] = { 0 };

	// ���̈ʒu�ŗ����B����ɂ�郁�b�Z�[�W�̓]����}������B
	m_mouseState = MouseDisabled;
	SetINPUT(&in[0], null, mk, false);
	::SendInput(1, in, sizeof(INPUT));
	PumpMessage();
	// �N���b�N�̃G�~�����[�V�����B
	m_mouseState = MouseGesture;
	SetINPUT(&in[0], &pt, mk, true);	// �{�^��������
	SetINPUT(&in[1], null, mk, false);	// �{�^���𗣂�
	::SendInput(2, in, sizeof(INPUT));
	PumpMessage();
}

void MouseDrag(UINT mk, POINT from, POINT to) throw()
{
	theThread.mouseDrag(mk, from, to);
}

void MouseUp(UINT mk, POINT pt) throw()
{
	theThread.mouseUp(mk, pt);
}

HWND GetWindow() throw()
{
	return theThread.m_hwnd;
}

//==========================================================================================================================

namespace
{
	const DWORD SHCNE_GENERIC_CREATE = SHCNE_CREATE | SHCNE_MKDIR | SHCNE_DRIVEADD | SHCNE_MEDIAINSERTED;
	const DWORD SHCNE_GENERIC_DELETE = SHCNE_DELETE | SHCNE_RMDIR | SHCNE_DRIVEREMOVED | SHCNE_MEDIAREMOVED;
	const DWORD SHCNE_GENERIC_RENAME = SHCNE_RENAMEFOLDER | SHCNE_RENAMEITEM;
	const DWORD SHCNE_GENERIC_EVENTS = SHCNE_GENERIC_CREATE | SHCNE_GENERIC_DELETE | SHCNE_GENERIC_RENAME;

	struct ILEvent
	{
		const ITEMIDLIST*	deleted;
		const ITEMIDLIST*	created;

		ILEvent clone() const throw()
		{
			ILEvent e;
			e.deleted = (deleted ? ILClone(deleted) : null);
			e.created = (created ? ILClone(created) : null);
			return e;
		}

		void free() throw()
		{
			if (deleted)
			{
				ILFree(const_cast<ITEMIDLIST*>(deleted));
				deleted = null;
			}
			if (created)
			{
				ILFree(const_cast<ITEMIDLIST*>(created));
				created = null;
			}
		}

		friend bool operator == (const ILEvent& lhs, const ILEvent& rhs) throw()
		{
			return ILEquals(lhs.deleted, rhs.deleted) &&
				   ILEquals(lhs.created, rhs.created);
		}

		friend bool operator < (const ILEvent& lhs, const ILEvent& rhs) throw()
		{
			int cmp = ILCompare(lhs.deleted, rhs.deleted);
			if (cmp < 0)
				return true;
			else if (cmp > 0)
				return false;
			else
				return ILCompare(lhs.created, rhs.created) < 0;
		}
	};

	typedef std::set<ILEvent>	ILEventMap;

	const UINT ILEVENT_DELAY		= 200;	// ms

	static ULONG		theILEventToken;
	static ILEventMap	theILEvents;
	static UINT_PTR		theILEventTimer;

	static void CALLBACK SHNotifyEvent(HWND, UINT, UINT_PTR, DWORD)
	{
		::KillTimer(null, theILEventTimer);
		theILEventTimer = 0;

		for (ILEventMap::iterator i = theILEvents.begin(); i != theILEvents.end(); ++i)
		{
			WCHAR	path1[MAX_PATH];
			WCHAR	path2[MAX_PATH];

			if (i->deleted && i->created)
			{
				ILPath(i->deleted, path1);
				ILPath(i->created, path2);
				TRACE(L"RENAME: %s -> %s", path1, path2);
			}
			else if (i->deleted)
			{
				ILPath(i->deleted, path1);
				TRACE(L"DELETE: %s", path1);
			}
			else if (i->created)
			{
				ILPath(i->created, path2);
				TRACE(L"CREATE: %s", path2);
			}

			i->free();
		}
		theILEvents.clear();
	}

	static void OnILEvent(const ITEMIDLIST* deleted, const ITEMIDLIST* created)
	{
		ILEvent e = { deleted, created };
		ILEventMap::iterator i = theILEvents.find(e);
		if (i == theILEvents.end())
		{
			theILEvents.insert(e.clone());
			theILEventTimer = ::SetTimer(null, theILEventTimer, ILEVENT_DELAY, SHNotifyEvent);
		}
		// FIXME: ����ł��܂� CREATE �C�x���g��2��}������Ă��܂����Ƃ������B
	}
}

void SHNotifyBegin(HWND hwnd, UINT msg)
{
	ASSERT(theILEventToken == 0);
	ILPtr desktop;
	if SUCCEEDED(::SHGetSpecialFolderLocation(hwnd, CSIDL_DESKTOP, &desktop))
	{
		const DWORD SHCNRF_SOURCE = SHCNRF_NewDelivery | SHCNRF_ShellLevel;
		SHChangeNotifyEntry entry = { desktop, true };
		theILEventToken = ::SHChangeNotifyRegister(hwnd, SHCNRF_SOURCE, SHCNE_GENERIC_EVENTS, msg, 1, &entry);
	}
}

void SHNotifyEnd()
{
	if (theILEventToken)
	{
		::SHChangeNotifyDeregister(theILEventToken);
		theILEventToken = 0;
	}
}

HRESULT SHNotifyMessage(WPARAM wParam, LPARAM lParam)
{
	LONG what = 0;
	ITEMIDLIST** pidls = null;
	HANDLE hLock = ::SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &pidls, &what);
	if (!hLock)
		return E_FAIL;
	__try
	{
		if (what & SHCNE_GENERIC_CREATE)
			OnILEvent(null, pidls[0]);
		if (what & SHCNE_GENERIC_DELETE)
			OnILEvent(pidls[0], null);
		if (what & SHCNE_GENERIC_RENAME)
			OnILEvent(pidls[0], pidls[1]);
	}
	__finally
	{
		::SHChangeNotification_Unlock(hLock);
	}
	return S_OK;
}

//=============================================================================
// ���������AThread �� Window �����ڂɊ֌W����̂ŁA�����Œ�`���Ă���

void Window::update() throw()
{
	m_updated = true;
	theThread.markUpdate();
}

void Window::run() throw()
{
	ASSERT(IsWindow());

	if (!::IsWindowVisible(m_hwnd))
		::ShowWindow(m_hwnd, SW_SHOW);

	theThread.run(this);
}

void Thread::run(Window* form)
{
	HWND	hwnd = form->Handle;
	HWND	hwndPrev = m_hwnd;

	m_hwnd = hwnd;

	if (!hwndPrev)
	{
		ASSERT(!m_hookMSG);
		ASSERT(!m_hookCBT);
		HINSTANCE module = ::GetModuleHandle(null);
		DWORD tid = ::GetCurrentThreadId();
		m_hookMSG = ::SetWindowsHookEx(WH_GETMESSAGE, onMessage, module, tid);
		m_hookCBT = ::SetWindowsHookEx(WH_CBT, onCBT, module, tid);
		RegisterHotKeys();
		::RegisterDragDrop(hwnd, this);
#if 0
		SHNotifyBegin(hwnd, WM_SHNOTIFY);
#endif
	}

	__try
	{
		for (;;)
		{
			PumpMessage();
			if (m_needsUpdate)
				doUpdate();
			else if (!::IsWindow(hwnd))
			{	// PumpMessage�セ�̂܂�WaitMessage���Ă͂Ȃ�Ȃ��B
				// hwnd �͊��ɔj���ς݂ł���\��������B
				break;
			}
			else
			{
				::WaitMessage();
			}
		}
	}
	__finally
	{
		m_hwnd = hwndPrev;
		if (!m_hwnd)
		{
#if 0
			SHNotifyEnd();
#endif
			::RevokeDragDrop(hwnd);
			UnregisterHotKeys();
			::UnhookWindowsHookEx(m_hookMSG);
			m_hookMSG = null;
			::UnhookWindowsHookEx(m_hookCBT);
			m_hookCBT = null;
		}
	}
}

//=============================================================================
// DragDrop

// Drop() �ł̓}�E�X�̃{�^����������Ă���A���E�h���b�O�̋�ʂ����Ȃ��B
// ���̂��߁A�ǂ̃{�^���Ńh���b�O����Ă��邩��ۑ����Ă����B
// �����̃h���b�O�������ɔ������邱�Ƃ͖����̂ŃO���[�o���ϐ��ł��܂�Ȃ��B
static DWORD	DragButtons = 0;

STDMETHODIMP Thread::DragEnter(IDataObject* data, DWORD keys, POINTL pt, DWORD* effect)
{
	ASSERT(!m_dropData);

	m_dropTarget = null;
	m_dropData = data;
	SetRectEmpty(&m_dropBounds);
	if (!m_dropHelper)
		m_dropHelper.newobj(CLSID_DragDropHelper);
	m_dropHelper->DragEnter(m_hwnd, data, (POINT*)&pt, *effect);
	DragButtons = (keys & MK_BUTTONS);
	return S_OK;
}

STDMETHODIMP Thread::DragOver(DWORD keys, POINTL pt, DWORD* effect)
{
	ASSERT(m_dropData);

	if (!m_dropData)
	{
		if (effect)
			*effect = DROPEFFECT_NONE;
		return E_UNEXPECTED;
	}

	m_dropHelper->DragOver((POINT*)&pt, *effect);

	for (HWND hwnd = HwndFromPoint((POINT&)pt); hwnd; hwnd = ::GetAncestor(hwnd, GA_PARENT))
	{
		if (Window* target = Window::from(hwnd))
		{
			STATIC_ASSERT( sizeof(POINT) == sizeof(POINTL) );
			m_dropTarget = target->onDragOver(keys, (POINT&)pt, effect);
			return S_OK;
		}
	}

	if (effect)
		*effect = DROPEFFECT_NONE;
	return S_OK;
}

STDMETHODIMP Thread::DragLeave()
{
	if (m_dropTarget)
	{
		m_dropTarget->onDragLeave();
		m_dropTarget = null;
	}
	m_dropData = null;
	m_dropHelper->DragLeave();
	return S_OK;
}

STDMETHODIMP Thread::Drop(IDataObject* data, DWORD keys, POINTL pt, DWORD* effect)
{
	if (m_dropTarget)
	{
		m_dropTarget->onDragDrop(data, keys | DragButtons, (POINT&)pt, effect);
		m_dropTarget = null;
	}
	else if (effect)
		*effect = DROPEFFECT_NONE;
	m_dropData = null;
	m_dropHelper->Drop(data, (POINT*)&pt, *effect);
	return S_OK;
}
