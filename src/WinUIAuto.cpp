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

bool WinUIAuto::isWindow(IUIAutomationElement* element, JSON& json)
{
	CONTROLTYPEID typeId;
	if (SUCCEEDED(element->get_CurrentControlType(&typeId))) {
		if (typeId == UIA_WindowControlTypeId) {
			UIString name;
			if (SUCCEEDED(element->get_CurrentName(&name))) {
				json["window"] = WC2MB(name);
				return true;
			}
		}
	}
	return false;
}

#define SET_JSON(key, method) { UIString name; if (SUCCEEDED(element->method(&name))) json[key] = WC2MB(name); }

JSON WinUIAuto::info(IUIAutomationElement* element, bool subtree)
{
	if (element == nullptr) return {};
	JSON json;

	CONTROLTYPEID typeId;
	if (SUCCEEDED(element->get_CurrentControlType(&typeId))) {
		json["type"] = type2str(typeId);
	}

	SET_JSON("name", get_CurrentName);
	SET_JSON("help", get_CurrentHelpText);
	SET_JSON("info", get_CurrentLocalizedControlType);

	std::stringstream ss;
	SAFEARRAY* id = nullptr;
	if (SUCCEEDED(element->GetRuntimeId(&id))) {
		void* pVoid = 0;
		::SafeArrayAccessData(id, &pVoid);
		const long* pLongs = reinterpret_cast<long*>(pVoid);
		for (ULONG i = 0; i < id->rgsabound[0].cElements; ++i) {
			const long val = pLongs[i];
			if (i) ss << ".";
			ss << std::hex << val;
		}
		::SafeArrayUnaccessData(id);
	}
	json["id"] = ss.str();

	RECT rect;
	if (SUCCEEDED(element->get_CurrentBoundingRectangle(&rect))) {
		json["size"] = {
			{"left", rect.left },
			{"top", rect.top },
			{"right", rect.right },
			{"bottom", rect.bottom },
			{"width", rect.right - rect.left },
			{"height", rect.bottom - rect.top },
		};
	}

	CComVariant value;
	if (SUCCEEDED(element->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &value)))
		if (auto length = SysStringLen(value.bstrVal))
			json["value"] = WC2MB(std::wstring(value.bstrVal, length));

	IUIAutomationTreeWalker* walker;
	pAutomation->get_ControlViewWalker(&walker);
	if (subtree) {
		UIAutoUniquePtr<IUIAutomationElement> child;
		walker->GetFirstChildElement(element, UI(child));
		while (child) {
			json["tree"].push_back(info(child.get(), true));
			walker->GetNextSiblingElement(child.get(), UI(child));
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

JSON WinUIAuto::info(IUIAutomationElementArray* elements) {
	JSON json;
	int count = 0;
	if (elements) elements->get_Length(&count);
	for (int i = 0; i < count; ++i) {
		UIAutoUniquePtr<IUIAutomationElement> element;
		elements->GetElement(i, UI(element));
		json.push_back(info(element.get(), false));
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
	InitAutomation();

	UIAutoUniquePtr<IUIAutomationElement> element;
	if (FAILED(pAutomation->GetFocusedElement(UI(element)))) return {};
	return info(element.get(), false).dump();
}

std::string WinUIAuto::GetElements(DWORD pid)
{
	InitAutomation();

	UIAutoUniquePtr<IUIAutomationElement> parent;
	if (pid == 0) {
		auto hWnd = ::GetActiveWindow();
		if (FAILED(pAutomation->ElementFromHandle(hWnd, UI(parent)))) return {};
	}
	else {
		UIAutoUniquePtr<IUIAutomationElement> root;
		if (FAILED(pAutomation->GetRootElement(UI(root)))) return {};

		UIAutoUniquePtr<IUIAutomationCondition> cond;
		pAutomation->CreatePropertyCondition(UIA_ProcessIdPropertyId, CComVariant((int)pid, VT_INT), UI(cond));
		root->FindFirst(TreeScope_Children, cond.get(), UI(parent));
	}
	return info(parent.get(), true).dump();
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

std::string WinUIAuto::FindElements(DWORD pid, const std::wstring& name, const std::string& type, const std::string& parent)
{
	InitAutomation();

	if (pid == 0) ::GetWindowThreadProcessId(::GetActiveWindow(), &pid);

	std::vector<IUIAutomationCondition*> conditions;
	UIAutoUniquePtr<IUIAutomationCondition> cProc, cName, cName1, cName2, cType;
	pAutomation->CreatePropertyCondition(UIA_ProcessIdPropertyId, CComVariant((int)pid, VT_INT), UI(cProc));
	pAutomation->CreatePropertyConditionEx(UIA_NamePropertyId, CComVariant(name.c_str()), PropertyConditionFlags_IgnoreCase, UI(cName1));
	pAutomation->CreatePropertyConditionEx(UIA_NamePropertyId, CComVariant((name + L":").c_str()), PropertyConditionFlags_IgnoreCase, UI(cName2));
	pAutomation->CreateOrCondition(cName1.get(), cName2.get(), UI(cName));
	conditions.push_back(cName.get());

	if (auto iType = str2type(type)) {
		pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, CComVariant((int)iType, VT_INT), UI(cType));
		conditions.push_back(cType.get());
	}

	UIAutoUniquePtr<IUIAutomationElement> root, owner;
	if (FAILED(pAutomation->GetRootElement(UI(root)))) return {};
	if (root) root->FindFirst(TreeScope_Children, cProc.get(), UI(owner));

	if (!parent.empty()) {
		auto v = str2id(parent);
		SAFEARRAY* sa;
		UIAutoUniquePtr<IUIAutomationCondition> cond;
		pAutomation->IntNativeArrayToSafeArray(v.data(), (int)v.size(), &sa);
		pAutomation->CreatePropertyCondition(UIA_RuntimeIdPropertyId, CComVariant(sa), UI(cond));
		if (owner) owner->FindFirst(TreeScope_Subtree, cond.get(), UI(owner));
		SafeArrayDestroy(sa);
	}

	UIAutoUniquePtr<IUIAutomationCondition> cond;
	UIAutoUniquePtr<IUIAutomationElementArray> elements;
	pAutomation->CreateAndConditionFromNativeArray(conditions.data(), (int)conditions.size(), UI(cond));
	if (owner) owner->FindAll(TreeScope_Subtree, cond.get(), UI(elements));
	return info(elements.get()).dump();
}

#endif//_WINDOWS