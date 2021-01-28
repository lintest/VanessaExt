#ifdef _WINDOWS

//  Copyright (c) 2015, 2016 Jari Pennanen
//  https://github.com/Ciantic/VirtualDesktopAccessor

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <inspectable.h>
#include <SDKDDKVer.h>
#include <ObjectArray.h>
#include "Win10Desktops.h"

#define VDA_VirtualDesktopCreated 5
#define VDA_VirtualDesktopDestroyBegin 4
#define VDA_VirtualDesktopDestroyFailed 3
#define VDA_VirtualDesktopDestroyed 2
#define VDA_ViewVirtualDesktopChanged 1
#define VDA_CurrentVirtualDesktopChanged 0

#define VDA_IS_NORMAL 1
#define VDA_IS_MINIMIZED 2
#define VDA_IS_MAXIMIZED 3

std::map<HWND, int> listeners;
IServiceProvider* pServiceProvider = nullptr;
IVirtualDesktopManagerInternal *pDesktopManagerInternal = nullptr;
IVirtualDesktopManager *pDesktopManager = nullptr;
IApplicationViewCollection *viewCollection = nullptr;
IVirtualDesktopPinnedApps *pinnedApps = nullptr;
IVirtualDesktopNotificationService* pDesktopNotificationService = nullptr;
BOOL registeredForNotifications = FALSE;

DWORD idNotificationService = 0;

void _RegisterService(BOOL force = FALSE) {
	if (force) {
		pServiceProvider = nullptr;
		pDesktopManagerInternal = nullptr;
		pDesktopManager = nullptr;
		viewCollection = nullptr;
		pinnedApps = nullptr;
		pDesktopNotificationService = nullptr;
		registeredForNotifications = FALSE;
	}

	if (pServiceProvider != nullptr) {
		return;
	}
	::CoInitialize(NULL);
	::CoCreateInstance(
		CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER,
		__uuidof(IServiceProvider), (PVOID*)&pServiceProvider);

	if (pServiceProvider == nullptr) {
		throw u"FATAL ERROR: pServiceProvider is null";
	}
	pServiceProvider->QueryService(__uuidof(IApplicationViewCollection), &viewCollection);

	pServiceProvider->QueryService(__uuidof(IVirtualDesktopManager), &pDesktopManager);

	pServiceProvider->QueryService(
		CLSID_VirtualDesktopPinnedApps,
		__uuidof(IVirtualDesktopPinnedApps), (PVOID*)&pinnedApps);

	pServiceProvider->QueryService(
		CLSID_VirtualDesktopManagerInternal,
		__uuidof(IVirtualDesktopManagerInternal), (PVOID*)&pDesktopManagerInternal);

	if (viewCollection == nullptr) {
		throw u"FATAL ERROR: viewCollection is null";
	}

	if (pDesktopManagerInternal == nullptr) {
		throw u"FATAL ERROR: pDesktopManagerInternal is null";
	}

	// Notification service
	HRESULT hrNotificationService = pServiceProvider->QueryService(
		CLSID_IVirtualNotificationService,
		__uuidof(IVirtualDesktopNotificationService),
		(PVOID*)&pDesktopNotificationService);
}

int GetDesktopCount()
{
	_RegisterService();

	IObjectArray *pObjectArray = nullptr;
	HRESULT hr = pDesktopManagerInternal->GetDesktops(&pObjectArray);

	if (SUCCEEDED(hr))
	{
		UINT count;
		hr = pObjectArray->GetCount(&count);
		pObjectArray->Release();
		return count;
	}

	return -1;
}

int GetDesktopNumberById(const GUID &desktopId) {
	_RegisterService();

	IObjectArray *pObjectArray = nullptr;
	HRESULT hr = pDesktopManagerInternal->GetDesktops(&pObjectArray);
	int found = -1;

	if (SUCCEEDED(hr))
	{
		UINT count;
		hr = pObjectArray->GetCount(&count);

		if (SUCCEEDED(hr))
		{
			for (UINT i = 0; i < count; i++)
			{
				IVirtualDesktop *pDesktop = nullptr;

				if (FAILED(pObjectArray->GetAt(i, __uuidof(IVirtualDesktop), (void**)&pDesktop)))
					continue;

				GUID id = { 0 };
				if (SUCCEEDED(pDesktop->GetID(&id)) && id == desktopId)
				{
					found = i;
					pDesktop->Release();
					break;
				}

				pDesktop->Release();
			}
		}

		pObjectArray->Release();
	}

	return found;
}

IVirtualDesktop* _GetDesktopByNumber(int number) {
	_RegisterService();

	IObjectArray *pObjectArray = nullptr;
	HRESULT hr = pDesktopManagerInternal->GetDesktops(&pObjectArray);
	IVirtualDesktop* found = nullptr;

	if (SUCCEEDED(hr))
	{
		UINT count;
		hr = pObjectArray->GetCount(&count);
		pObjectArray->GetAt(number, __uuidof(IVirtualDesktop), (void**)&found);
		pObjectArray->Release();
	}

	return found;
}

GUID GetWindowDesktopId(HWND window) {
	_RegisterService();

	GUID pDesktopId = {};
	pDesktopManager->GetWindowDesktopId(window, &pDesktopId);

	return pDesktopId;
}

int GetWindowDesktopNumber(HWND window) {
	_RegisterService();

	GUID pDesktopId = {};
	if (SUCCEEDED(pDesktopManager->GetWindowDesktopId(window, &pDesktopId))) {
		return GetDesktopNumberById(pDesktopId);
	}

	return -1;
}

int IsWindowOnCurrentVirtualDesktop(HWND window) {
	_RegisterService();

	BOOL b;
	if (SUCCEEDED(pDesktopManager->IsWindowOnCurrentVirtualDesktop(window, &b))) {
		return b;
	}

	return -1;
}

GUID GetDesktopIdByNumber(int number) {
	GUID id;
	IVirtualDesktop* pDesktop = _GetDesktopByNumber(number);
	if (pDesktop != nullptr) {
		pDesktop->GetID(&id);
		pDesktop->Release();
	}
	return id;
}

int IsWindowOnDesktopNumber(HWND window, int number) {
	_RegisterService();
	IApplicationView* app = nullptr;
	if (window == 0) {
		return -1;
	}
	viewCollection->GetViewForHwnd(window, &app);
	GUID desktopId = { 0 };
	app->GetVirtualDesktopId(&desktopId);
	GUID desktopCheckId = GetDesktopIdByNumber(number);
	app->Release();
	if (desktopCheckId == GUID_NULL || desktopId == GUID_NULL) {
		return -1;
	}

	if (GetDesktopIdByNumber(number) == desktopId) {
		return 1;
	}
	else {
		return 0;
	}
	
	return -1;
}

BOOL MoveWindowToDesktopNumber(HWND window, int number) {
	_RegisterService();
	IVirtualDesktop* pDesktop = _GetDesktopByNumber(number);
	if (pDesktopManager == nullptr) {
		throw u"ARRGH?";
	}
	if (window == 0) {
		return false;
	}
	if (pDesktop != nullptr) {
		GUID id = { 0 };
		if (SUCCEEDED(pDesktop->GetID(&id))) {
			IApplicationView* app = nullptr;
			viewCollection->GetViewForHwnd(window, &app);
			if (app != nullptr) {
				pDesktopManagerInternal->MoveViewToDesktop(app, pDesktop);
				return true;
			}
		}
	}
	return false;
}

int GetDesktopNumber(IVirtualDesktop *pDesktop) {
	_RegisterService();

	if (pDesktop == nullptr) {
		return -1;
	}

	GUID guid;

	if (SUCCEEDED(pDesktop->GetID(&guid))) {
		return GetDesktopNumberById(guid);
	}

	return -1;
}

IVirtualDesktop* GetCurrentDesktop() {
	_RegisterService();

	if (pDesktopManagerInternal == nullptr) {
		return nullptr;
	}
	IVirtualDesktop* found = nullptr;
	pDesktopManagerInternal->GetCurrentDesktop(&found);
	return found;
}

int GetCurrentDesktopNumber() {
	IVirtualDesktop* virtualDesktop = GetCurrentDesktop();
	int number = GetDesktopNumber(virtualDesktop);
	if (virtualDesktop) virtualDesktop->Release();
	return number;
}

IVirtualDesktop* CreateDesktopW() {
	_RegisterService();

	if (pDesktopManagerInternal == nullptr) {
		return nullptr;
	}
	IVirtualDesktop* found = nullptr;
	pDesktopManagerInternal->CreateDesktopW(&found);
	return found;
}

int CreateDesktopNumber() {
	IVirtualDesktop* virtualDesktop = CreateDesktopW();
	int number = GetDesktopNumber(virtualDesktop);
	if (virtualDesktop) virtualDesktop->Release();
	return number;
}

bool RemoveDesktopByNumber(int number, int fallback) {
	_RegisterService();

	if (pDesktopManagerInternal == nullptr) {
		return false;
	}

	bool ok = false;
	IVirtualDesktop* virtualDesktop = _GetDesktopByNumber(number);
	IVirtualDesktop* fallbackDesktop = _GetDesktopByNumber(fallback);
	if (virtualDesktop && fallbackDesktop) {
		HRESULT hr = pDesktopManagerInternal->RemoveDesktop(virtualDesktop, fallbackDesktop);
		ok = SUCCEEDED(hr);
	}
	if (fallbackDesktop) fallbackDesktop->Release();
	if (virtualDesktop) virtualDesktop->Release();
	return ok;
}

bool GoToDesktopNumber(int number) {
	_RegisterService();

	if (pDesktopManagerInternal == nullptr) {
		return false;
	}

	IVirtualDesktop* oldDesktop = GetCurrentDesktop();
	GUID oldId = { 0 };
	oldDesktop->GetID(&oldId);
	oldDesktop->Release();

	IObjectArray *pObjectArray = nullptr;
	HRESULT hr = pDesktopManagerInternal->GetDesktops(&pObjectArray);
	int found = -1;

	bool ok = false;
	if (SUCCEEDED(hr))
	{
		UINT count;
		hr = pObjectArray->GetCount(&count);

		if (SUCCEEDED(hr))
		{
			for (UINT i = 0; i < count; i++)
			{
				IVirtualDesktop *pDesktop = nullptr;

				if (FAILED(pObjectArray->GetAt(i, __uuidof(IVirtualDesktop), (void**)&pDesktop)))
					continue;

				GUID id = { 0 };
				pDesktop->GetID(&id);
				if (i == number) {
					hr = pDesktopManagerInternal->SwitchDesktop(pDesktop);
					ok = SUCCEEDED(hr);
				}

				pDesktop->Release();
			}
		}
		pObjectArray->Release();
	}
	return ok;
}

#include "DesktopManager.h"

int64_t DesktopManager::GetDesktopCount()
{
	return ::GetDesktopCount();
}

int64_t DesktopManager::CreateDesktopNumber()
{
	return ::CreateDesktopNumber();
}

int64_t DesktopManager::GetCurrentDesktopNumber()
{
	return ::GetCurrentDesktopNumber();
}

bool  DesktopManager::GoToDesktopNumber(int64_t number)
{
	return ::GoToDesktopNumber((int)number);
}

bool DesktopManager::RemoveDesktopByNumber(int64_t number, int64_t fallback)
{
	return ::RemoveDesktopByNumber((int)number, (int)fallback);
}

int64_t DesktopManager::GetWindowDesktopNumber(int64_t window)
{
	return ::GetWindowDesktopNumber((HWND)window);
}

bool DesktopManager::MoveWindowToDesktopNumber(int64_t window, int64_t number)
{
	return ::MoveWindowToDesktopNumber((HWND)window, (int)number);
}

#endif//_WINDOWS