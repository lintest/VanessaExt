#ifdef _WINDOWS

#include "WinUIAuto.h"

class UIString {
private:
	BSTR name = nullptr;
public:
	UIString() {}
	virtual ~UIString() { SysFreeString(name); }
	BSTR* operator &() { return &name; }
	operator std::wstring() {
		return std::wstring(name, SysStringLen(name));
	}
};

#include <sstream>
#include <atlcomcli.h>
#include <UIAutomationClient.h>

static CONTROLTYPEID str2type(const std::string& type) {
	if (type.empty()) return 0;
	static const std::map<std::string, CONTROLTYPEID> map{
		{"button", UIA_ButtonControlTypeId},
		{"calendar", UIA_CalendarControlTypeId},
		{"checkbox", UIA_CheckBoxControlTypeId},
		{"combobox", UIA_ComboBoxControlTypeId},
		{"edit", UIA_EditControlTypeId},
		{"hyperlink", UIA_HyperlinkControlTypeId},
		{"image", UIA_ImageControlTypeId},
		{"listitem", UIA_ListItemControlTypeId},
		{"list", UIA_ListControlTypeId},
		{"menu", UIA_MenuControlTypeId},
		{"menubar", UIA_MenuBarControlTypeId},
		{"menuitem", UIA_MenuItemControlTypeId},
		{"progressbar", UIA_ProgressBarControlTypeId},
		{"radiobutton", UIA_RadioButtonControlTypeId},
		{"scrollbar", UIA_ScrollBarControlTypeId},
		{"slider", UIA_SliderControlTypeId},
		{"spinner", UIA_SpinnerControlTypeId},
		{"statusbar", UIA_StatusBarControlTypeId},
		{"tab", UIA_TabControlTypeId},
		{"tabitem", UIA_TabItemControlTypeId},
		{"text", UIA_TextControlTypeId},
		{"toolbar", UIA_ToolBarControlTypeId},
		{"tooltip", UIA_ToolTipControlTypeId},
		{"tree", UIA_TreeControlTypeId},
		{"treeitem", UIA_TreeItemControlTypeId},
		{"custom", UIA_CustomControlTypeId},
		{"group", UIA_GroupControlTypeId},
		{"thumb", UIA_ThumbControlTypeId},
		{"datagrid", UIA_DataGridControlTypeId},
		{"dataitem", UIA_DataItemControlTypeId},
		{"document", UIA_DocumentControlTypeId},
		{"splitbutton", UIA_SplitButtonControlTypeId},
		{"window", UIA_WindowControlTypeId},
		{"pane", UIA_PaneControlTypeId},
		{"header", UIA_HeaderControlTypeId},
		{"headeritem", UIA_HeaderItemControlTypeId},
		{"table", UIA_TableControlTypeId},
		{"titlebar", UIA_TitleBarControlTypeId},
		{"separator", UIA_SeparatorControlTypeId},
		{"semanticzoom", UIA_SemanticZoomControlTypeId},
		{"appbar", UIA_AppBarControlTypeId},
	};
	std::string text = type;
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
	auto it = map.find(text);
	return it == map.end() ? 0 : it->second;
}

static std::string type2str(CONTROLTYPEID typeId) {
	switch (typeId) {
	case UIA_ButtonControlTypeId: return "Button";
	case UIA_CalendarControlTypeId: return "Calendar";
	case UIA_CheckBoxControlTypeId: return "CheckBox";
	case UIA_ComboBoxControlTypeId: return "ComboBox";
	case UIA_EditControlTypeId: return "Edit";
	case UIA_HyperlinkControlTypeId: return "Hyperlink";
	case UIA_ImageControlTypeId: return "Image";
	case UIA_ListItemControlTypeId: return "ListItem";
	case UIA_ListControlTypeId: return "List";
	case UIA_MenuControlTypeId: return "Menu";
	case UIA_MenuBarControlTypeId: return "MenuBar";
	case UIA_MenuItemControlTypeId: return "MenuItem";
	case UIA_ProgressBarControlTypeId: return "ProgressBar";
	case UIA_RadioButtonControlTypeId: return "RadioButton";
	case UIA_ScrollBarControlTypeId: return "ScrollBar";
	case UIA_SliderControlTypeId: return "Slider";
	case UIA_SpinnerControlTypeId: return "Spinner";
	case UIA_StatusBarControlTypeId: return "StatusBar";
	case UIA_TabControlTypeId: return "Tab";
	case UIA_TabItemControlTypeId: return "TabItem";
	case UIA_TextControlTypeId: return "Text";
	case UIA_ToolBarControlTypeId: return "ToolBar";
	case UIA_ToolTipControlTypeId: return "ToolTip";
	case UIA_TreeControlTypeId: return "Tree";
	case UIA_TreeItemControlTypeId: return "TreeItem";
	case UIA_CustomControlTypeId: return "Custom";
	case UIA_GroupControlTypeId: return "Group";
	case UIA_ThumbControlTypeId: return "Thumb";
	case UIA_DataGridControlTypeId: return "DataGrid";
	case UIA_DataItemControlTypeId: return "DataItem";
	case UIA_DocumentControlTypeId: return "Document";
	case UIA_SplitButtonControlTypeId: return "SplitButton";
	case UIA_WindowControlTypeId: return "Window";
	case UIA_PaneControlTypeId: return "Pane";
	case UIA_HeaderControlTypeId: return "Header";
	case UIA_HeaderItemControlTypeId: return "HeaderItem";
	case UIA_TableControlTypeId: return "Table";
	case UIA_TitleBarControlTypeId: return "TitleBar";
	case UIA_SeparatorControlTypeId: return "Separator";
	case UIA_SemanticZoomControlTypeId: return "SemanticZoom";
	case UIA_AppBarControlTypeId: return "AppBar";
	default: return {};
	}
}

WinUIAuto::UICacheRequest::UICacheRequest(WinUIAuto& owner)
{
	owner.InitAutomation();
	owner.pAutomation->CreateCacheRequest(UI(cache));
	cache->AddProperty(UIA_RuntimeIdPropertyId);
	cache->AddProperty(UIA_BoundingRectanglePropertyId);
	cache->AddProperty(UIA_ProcessIdPropertyId);
	cache->AddProperty(UIA_ControlTypePropertyId);
	cache->AddProperty(UIA_LocalizedControlTypePropertyId);
	cache->AddProperty(UIA_NamePropertyId);
	cache->AddProperty(UIA_AutomationIdPropertyId);
	cache->AddProperty(UIA_HelpTextPropertyId);
	cache->AddProperty(UIA_ClickablePointPropertyId);
	cache->AddProperty(UIA_ValueValuePropertyId);
	cache->put_TreeScope(TreeScope_Subtree);
}

bool WinUIAuto::isWindow(IUIAutomationElement* element, JSON& json)
{
	CONTROLTYPEID typeId;
	if (SUCCEEDED(element->get_CurrentControlType(&typeId))) {
		if (typeId == UIA_WindowControlTypeId) {
			UIString name;
			if (SUCCEEDED(element->get_CurrentName(&name))) {
				json["Window"] = WC2MB(name);
				return true;
			}
		}
	}
	return false;
}

#define SET_JSON(key, method) { UIString name; if (SUCCEEDED(element->method(&name))) json[key] = WC2MB(name); }

JSON WinUIAuto::info(IUIAutomationElement* element, UICacheRequest& cache, bool subtree)
{
	if (element == nullptr) return {};
	JSON json;

	CONTROLTYPEID typeId;
	if (SUCCEEDED(element->get_CachedControlType(&typeId))) {
		json["Type"] = type2str(typeId);
	}

	SET_JSON("Name", get_CachedName);
	SET_JSON("HelpText", get_CachedHelpText);
	SET_JSON("AutomationId", get_CachedAutomationId);
	SET_JSON("LocalizedControlType", get_CachedLocalizedControlType);

	std::stringstream ss;
	std::unique_ptr<SAFEARRAY, SafeArrayDeleter> id;
	if (SUCCEEDED(element->GetRuntimeId(UI(id)))) {
		void* pVoid = 0;
		::SafeArrayAccessData(id.get(), &pVoid);
		const long* pLongs = reinterpret_cast<long*>(pVoid);
		for (ULONG i = 0; i < id->rgsabound[0].cElements; ++i) {
			const long val = pLongs[i];
			if (i) ss << ".";
			ss << std::hex << val;
		}
		::SafeArrayUnaccessData(id.get());
	}
	json["Id"] = ss.str();

	RECT rect;
	if (SUCCEEDED(element->get_CurrentBoundingRectangle(&rect))) {
		json["Size"] = {
			{"Left", rect.left },
			{"Top", rect.top },
			{"Right", rect.right },
			{"Bottom", rect.bottom },
			{"Width", rect.right - rect.left },
			{"Height", rect.bottom - rect.top },
		};
	}

	POINT point = { 0, 0 }; BOOL gotClickable = false;
	if (SUCCEEDED(element->GetClickablePoint(&point, &gotClickable)))
		if (gotClickable) { json["x"] = point.x; json["y"] = point.y; }

	CComVariant value;
	if (SUCCEEDED(element->GetCachedPropertyValue(UIA_ValueValuePropertyId, &value)))
		if (auto length = SysStringLen(value.bstrVal))
			json["Value"] = WC2MB(std::wstring(value.bstrVal, length));

	IUIAutomationTreeWalker* walker;
	pAutomation->get_ControlViewWalker(&walker);
	if (subtree) {
		UIAutoUniquePtr<IUIAutomationElement> child;
		walker->GetFirstChildElementBuildCache(element, cache, UI(child));
		while (child) {
			json["Tree"].push_back(info(child.get(), cache, true));
			walker->GetNextSiblingElementBuildCache(child.get(), cache, UI(child));
		}
	}
	else {
		if (!isWindow(element, json)) {
			UIAutoUniquePtr<IUIAutomationElement> parent;
			walker->GetParentElement(element, UI(parent));
			while (parent) {
				if (isWindow(parent.get(), json)) break;
				walker->GetParentElement(parent.get(), UI(parent));
			}
		}
	}

	return json;
}

JSON WinUIAuto::info(IUIAutomationElementArray* elements, UICacheRequest& cache) {
	JSON json;
	int count = 0;
	if (elements == nullptr) return json;
	elements->get_Length(&count);
	for (int i = 0; i < count; ++i) {
		UIAutoUniquePtr<IUIAutomationElement> element;
		elements->GetElement(i, UI(element));
		json.push_back(info(element.get(), cache, false));
	}
	return json;
}

void WinUIAuto::InitAutomation()
{
	if (pAutomation == nullptr) {
		if (FAILED(CoInitialize(NULL)))
			throw std::runtime_error("CoInitialize error");

		if (FAILED(CoCreateInstance(CLSID_CUIAutomation,
			NULL, CLSCTX_INPROC_SERVER, IID_IUIAutomation,
			reinterpret_cast<void**>(&UI(pAutomation)))))
			throw std::runtime_error("CoInitialize error");
	}
}

std::string WinUIAuto::GetFocusedElement()
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> element;
	if (FAILED(pAutomation->GetFocusedElementBuildCache(cache, UI(element)))) return {};
	if (element.get() == nullptr) return {};
	return info(element.get(), cache).dump();
}

static std::vector<int> str2id(const std::string& text) {
	std::vector<int> result;
	std::stringstream ss(text);
	while (ss.good()) {
		std::string sub;
		std::getline(ss, sub, '.');
		int i = std::stoul(sub, 0, 16);
		result.push_back(i);
	}
	return result;
}

std::string WinUIAuto::GetElements(DWORD pid)
{
	UICacheRequest cache(*this);
	cache->put_TreeScope(TreeScope_Subtree);
	UIAutoUniquePtr<IUIAutomationElement> owner;
	find(pid, cache, UI(owner));
	if (owner.get() == nullptr) return {};
	return info(owner.get(), cache, true).dump();
}

std::string WinUIAuto::GetElements(const std::string& id)
{
	UICacheRequest cache(*this);
	cache->put_TreeScope(TreeScope_Subtree);
	UIAutoUniquePtr<IUIAutomationElement> owner;
	find(id, cache, UI(owner));
	if (owner.get() == nullptr) return {};
	return info(owner.get(), cache, true).dump();
}

static HWND GetMainProcessWindow(DWORD pid)
{
	using EnumParam = std::pair<DWORD, HWND>;
	EnumParam p{ pid, 0 };
	bool bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			if (::IsWindow(hWnd) && ::IsWindowVisible(hWnd)
				&& ::GetWindow(hWnd, GW_OWNER) == (HWND)0) {
				auto p = (EnumParam*)lParam;
				DWORD dwProcessId = 0;
				::GetWindowThreadProcessId(hWnd, &dwProcessId);
				if (p->first == dwProcessId) {
					p->second = hWnd;
				}
			}
			return TRUE;
		}, (LPARAM)&p);
	return p.second;
}

std::string WinUIAuto::FindElements(DWORD pid, const std::wstring& name, const std::string& type, const std::string& parent)
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> owner;
	std::vector<IUIAutomationCondition*> conditions;
	UIAutoUniquePtr<IUIAutomationCondition> cName, cName1, cName2, cType, cProc;

	if (parent.empty()) {
		find(pid, cache, UI(owner));
	}
	else {
		find(parent, cache, UI(owner));
	}
	if (owner.get() == nullptr) return {};

	pAutomation->CreatePropertyConditionEx(UIA_NamePropertyId, CComVariant(name.c_str()), PropertyConditionFlags_IgnoreCase, UI(cName1));
	pAutomation->CreatePropertyConditionEx(UIA_NamePropertyId, CComVariant((name + L":").c_str()), PropertyConditionFlags_IgnoreCase, UI(cName2));
	pAutomation->CreateOrCondition(cName1.get(), cName2.get(), UI(cName));
	conditions.push_back(cName.get());

	if (auto iType = str2type(type)) {
		pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, CComVariant((int)iType, VT_INT), UI(cType));
		conditions.push_back(cType.get());
	}

	UIAutoUniquePtr<IUIAutomationCondition> cond;
	UIAutoUniquePtr<IUIAutomationElementArray> elements;
	pAutomation->CreateAndConditionFromNativeArray(conditions.data(), (int)conditions.size(), UI(cond));
	owner->FindAllBuildCache(TreeScope_Subtree, cond.get(), cache, UI(elements));
	return info(elements.get(), cache).dump();
}

#define ASSERT(hr) if ((HRESULT)(hr) < 0) return hr;

HRESULT WinUIAuto::find(DWORD pid, UICacheRequest& cache, IUIAutomationElement** element)
{
	if (pid == 0) {
		ASSERT(pAutomation->ElementFromHandle(::GetActiveWindow(), element));
	}
	else {
		UIAutoUniquePtr<IUIAutomationElement> root;
		UIAutoUniquePtr<IUIAutomationCondition> cond;
		ASSERT(pAutomation->GetRootElement(UI(root)));
		ASSERT(pAutomation->CreatePropertyCondition(UIA_ProcessIdPropertyId, CComVariant((int)pid, VT_INT), UI(cond)));
		ASSERT(root->FindFirst(TreeScope_Children, cond.get(), element));
	}
	return 0;
}

HRESULT WinUIAuto::find(const std::string& id, UICacheRequest& cache, IUIAutomationElement** element)
{
	UIAutoUniquePtr<IUIAutomationElement> root;
	ASSERT(pAutomation->GetRootElement(UI(root)));
	auto v = str2id(id);
	std::unique_ptr<SAFEARRAY, SafeArrayDeleter> sa;
	UIAutoUniquePtr<IUIAutomationCondition> cond;
	ASSERT(pAutomation->IntNativeArrayToSafeArray(v.data(), (int)v.size(), UI(sa)));
	ASSERT(pAutomation->CreatePropertyCondition(UIA_RuntimeIdPropertyId, CComVariant(sa.get()), UI(cond)));
	ASSERT(root->FindFirstBuildCache(TreeScope_Subtree, cond.get(), cache, element));
	return 0;
}

bool WinUIAuto::InvokeElement(const std::string& id)
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> element;
	find(id, cache, UI(element));
	if (element.get() == nullptr) return false;
	UIAutoUniquePtr<IUIAutomationInvokePattern> pattern;
	if (FAILED(element->GetCurrentPattern(UIA_InvokePatternId, (IUnknown**)&UI(pattern)))) return false;
	return pattern && SUCCEEDED(pattern->Invoke());
}

bool WinUIAuto::FocusElement(const std::string& id)
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> element;
	find(id, cache, UI(element));
	if (element.get() == nullptr) return false;
	return SUCCEEDED(element->SetFocus());
}

std::string WinUIAuto::GetParentElement(const std::string& id)
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> child;
	find(id, cache, UI(child));
	if (child.get() == nullptr) return {};

	IUIAutomationTreeWalker* walker;
	pAutomation->get_ControlViewWalker(&walker);
	UIAutoUniquePtr<IUIAutomationElement> parent;
	walker->GetParentElementBuildCache(child.get(), cache, UI(parent));
	if (parent.get() == nullptr) return {};
	return info(parent.get(), cache).dump();
}

bool WinUIAuto::SetElementValue(const std::string& id, const std::wstring& value)
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> element;
	find(id, cache, UI(element));
	if (element.get() == nullptr) return false;
	UIAutoUniquePtr<IUIAutomationValuePattern> pattern;
	if (FAILED(element->GetCurrentPattern(UIA_ValuePatternId, (IUnknown**)&UI(pattern)))) return false;
	return pattern && SUCCEEDED(pattern->SetValue((BSTR)value.c_str()));
}

std::wstring WinUIAuto::GetElementValue(const std::string& id)
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> element;
	find(id, cache, UI(element));
	if (element.get() == nullptr) return {};

	CComVariant value;
	if (SUCCEEDED(element->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &value)))
		if (auto length = SysStringLen(value.bstrVal))
			return std::wstring(value.bstrVal, length);

	return {};
}

#endif//_WINDOWS
