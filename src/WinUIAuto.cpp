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

std::string type2str(CONTROLTYPEID typeId) {
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

JSON WinUIAuto::info(IUIAutomationElement* element)
{
	if (element == nullptr) return {};
	JSON json;

	UIString name;
	if (SUCCEEDED(element->get_CurrentName(&name))) {
		json["name"] = WC2MB(name);
	}

	CONTROLTYPEID typeId;
	if (SUCCEEDED(element->get_CurrentControlType(&typeId))) {
		json["type"] = type2str(typeId);
	}

	UIString typeName;
	if (SUCCEEDED(element->get_CurrentLocalizedControlType(&typeName))) {
		json["info"] = WC2MB(typeName);
	}

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
			{"heigth", rect.bottom - rect.top },
		};
	}

	CComVariant value;
	if (SUCCEEDED(element->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &value)))
		if (auto length = SysStringLen(value.bstrVal))
			json["value"] = WC2MB(std::wstring(value.bstrVal, length));

	IUIAutomationTreeWalker* walker;
	pAutomation->get_ControlViewWalker(&walker);
	IUIAutomationElement* ptr = nullptr;
	walker->GetFirstChildElement(element, &ptr);
	UIAutoUniquePtr<IUIAutomationElement> child(ptr);
	while (child) {
		json["tree"].push_back(info(child.get()));
		walker->GetNextSiblingElement(child.get(), &ptr);
		child.reset(ptr);
	}

	return json;
}

std::string WinUIAuto::GetElements(int64_t pid)
{
	if (pAutomation == nullptr) {
		IUIAutomation* p;
		if (FAILED(CoInitialize(NULL))) return {};
		if (FAILED(CoCreateInstance(CLSID_CUIAutomation, NULL,
			CLSCTX_INPROC_SERVER, IID_IUIAutomation,
			reinterpret_cast<void**>(&p)))) return {};
		pAutomation.reset(p);
	}

	UIAutoUniquePtr<IUIAutomationElement> parent;
	if (pid == 0) {
		IUIAutomationElement* p = nullptr;
		auto hWnd = ::GetActiveWindow();
		if (FAILED(pAutomation->ElementFromHandle(hWnd, &p))) return {};
		parent.reset(p);
	}
	else {
		IUIAutomationElement* ptr = nullptr;
		UIAutoUniquePtr<IUIAutomationElement> root;
		if (FAILED(pAutomation->GetRootElement(&ptr))) return {};
		root.reset(ptr);
		VARIANT variant = { 0 };
		V_INT(&variant) = pid;
		V_VT(&variant) = VT_INT;
		IUIAutomationCondition* pCond;
		pAutomation->CreatePropertyCondition(UIA_ProcessIdPropertyId, variant, &pCond);
		UIAutoUniquePtr<IUIAutomationCondition> condition(pCond);
		root->FindFirst(TreeScope_Children, condition.get(), &ptr);
		parent.reset(ptr);
	}
	return info(parent.get()).dump();
}

#endif//_WINDOWS