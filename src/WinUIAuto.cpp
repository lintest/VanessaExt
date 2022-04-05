#ifdef _WINDOWS

#include "WinUIAuto.h"
#include "AddInNative.h"

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

const long UIA_ParentPropertyId = UIA_RuntimeIdPropertyId - 1;

std::map<std::string, int> properties = {
	{ std::string("Parent"), UIA_ParentPropertyId },
	{ std::string("RuntimeId"), UIA_RuntimeIdPropertyId },
	{ std::string("BoundingRectangle"), UIA_BoundingRectanglePropertyId },
	{ std::string("ProcessId"), UIA_ProcessIdPropertyId },
	{ std::string("ControlType"), UIA_ControlTypePropertyId },
	{ std::string("LocalizedControlType"), UIA_LocalizedControlTypePropertyId },
	{ std::string("Name"), UIA_NamePropertyId },
	{ std::string("AcceleratorKey"), UIA_AcceleratorKeyPropertyId },
	{ std::string("AccessKey"), UIA_AccessKeyPropertyId },
	{ std::string("HasKeyboardFocus"), UIA_HasKeyboardFocusPropertyId },
	{ std::string("IsKeyboardFocusable"), UIA_IsKeyboardFocusablePropertyId },
	{ std::string("IsEnabled"), UIA_IsEnabledPropertyId },
	{ std::string("AutomationId"), UIA_AutomationIdPropertyId },
	{ std::string("ClassName"), UIA_ClassNamePropertyId },
	{ std::string("HelpText"), UIA_HelpTextPropertyId },
	{ std::string("ClickablePoint"), UIA_ClickablePointPropertyId },
	{ std::string("Culture"), UIA_CulturePropertyId },
	{ std::string("IsControlElement"), UIA_IsControlElementPropertyId },
	{ std::string("IsContentElement"), UIA_IsContentElementPropertyId },
	{ std::string("LabeledBy"), UIA_LabeledByPropertyId },
	{ std::string("IsPassword"), UIA_IsPasswordPropertyId },
	{ std::string("NativeWindowHandle"), UIA_NativeWindowHandlePropertyId },
	{ std::string("ItemType"), UIA_ItemTypePropertyId },
	{ std::string("IsOffscreen"), UIA_IsOffscreenPropertyId },
	{ std::string("Orientation"), UIA_OrientationPropertyId },
	{ std::string("FrameworkId"), UIA_FrameworkIdPropertyId },
	{ std::string("IsRequiredForForm"), UIA_IsRequiredForFormPropertyId },
	{ std::string("ItemStatus"), UIA_ItemStatusPropertyId },
	{ std::string("IsDockPatternAvailable"), UIA_IsDockPatternAvailablePropertyId },
	{ std::string("IsExpandCollapsePatternAvailable"), UIA_IsExpandCollapsePatternAvailablePropertyId },
	{ std::string("IsGridItemPatternAvailable"), UIA_IsGridItemPatternAvailablePropertyId },
	{ std::string("IsGridPatternAvailable"), UIA_IsGridPatternAvailablePropertyId },
	{ std::string("IsInvokePatternAvailable"), UIA_IsInvokePatternAvailablePropertyId },
	{ std::string("IsMultipleViewPatternAvailable"), UIA_IsMultipleViewPatternAvailablePropertyId },
	{ std::string("IsRangeValuePatternAvailable"), UIA_IsRangeValuePatternAvailablePropertyId },
	{ std::string("IsScrollPatternAvailable"), UIA_IsScrollPatternAvailablePropertyId },
	{ std::string("IsScrollItemPatternAvailable"), UIA_IsScrollItemPatternAvailablePropertyId },
	{ std::string("IsSelectionItemPatternAvailable"), UIA_IsSelectionItemPatternAvailablePropertyId },
	{ std::string("IsSelectionPatternAvailable"), UIA_IsSelectionPatternAvailablePropertyId },
	{ std::string("IsTablePatternAvailable"), UIA_IsTablePatternAvailablePropertyId },
	{ std::string("IsTableItemPatternAvailable"), UIA_IsTableItemPatternAvailablePropertyId },
	{ std::string("IsTextPatternAvailable"), UIA_IsTextPatternAvailablePropertyId },
	{ std::string("IsTogglePatternAvailable"), UIA_IsTogglePatternAvailablePropertyId },
	{ std::string("IsTransformPatternAvailable"), UIA_IsTransformPatternAvailablePropertyId },
	{ std::string("IsValuePatternAvailable"), UIA_IsValuePatternAvailablePropertyId },
	{ std::string("IsWindowPatternAvailable"), UIA_IsWindowPatternAvailablePropertyId },
	{ std::string("ValueValue"), UIA_ValueValuePropertyId },
	{ std::string("ValueIsReadOnly"), UIA_ValueIsReadOnlyPropertyId },
	{ std::string("RangeValueValue"), UIA_RangeValueValuePropertyId },
	{ std::string("RangeValueIsReadOnly"), UIA_RangeValueIsReadOnlyPropertyId },
	{ std::string("RangeValueMinimum"), UIA_RangeValueMinimumPropertyId },
	{ std::string("RangeValueMaximum"), UIA_RangeValueMaximumPropertyId },
	{ std::string("RangeValueLargeChange"), UIA_RangeValueLargeChangePropertyId },
	{ std::string("RangeValueSmallChange"), UIA_RangeValueSmallChangePropertyId },
	{ std::string("ScrollHorizontalScrollPercent"), UIA_ScrollHorizontalScrollPercentPropertyId },
	{ std::string("ScrollHorizontalViewSize"), UIA_ScrollHorizontalViewSizePropertyId },
	{ std::string("ScrollVerticalScrollPercent"), UIA_ScrollVerticalScrollPercentPropertyId },
	{ std::string("ScrollVerticalViewSize"), UIA_ScrollVerticalViewSizePropertyId },
	{ std::string("ScrollHorizontallyScrollable"), UIA_ScrollHorizontallyScrollablePropertyId },
	{ std::string("ScrollVerticallyScrollable"), UIA_ScrollVerticallyScrollablePropertyId },
	{ std::string("SelectionSelection"), UIA_SelectionSelectionPropertyId },
	{ std::string("SelectionCanSelectMultiple"), UIA_SelectionCanSelectMultiplePropertyId },
	{ std::string("SelectionIsSelectionRequired"), UIA_SelectionIsSelectionRequiredPropertyId },
	{ std::string("GridRowCount"), UIA_GridRowCountPropertyId },
	{ std::string("GridColumnCount"), UIA_GridColumnCountPropertyId },
	{ std::string("GridItemRow"), UIA_GridItemRowPropertyId },
	{ std::string("GridItemColumn"), UIA_GridItemColumnPropertyId },
	{ std::string("GridItemRowSpan"), UIA_GridItemRowSpanPropertyId },
	{ std::string("GridItemColumnSpan"), UIA_GridItemColumnSpanPropertyId },
	{ std::string("GridItemContainingGrid"), UIA_GridItemContainingGridPropertyId },
	{ std::string("DockDockPosition"), UIA_DockDockPositionPropertyId },
	{ std::string("ExpandCollapseExpandCollapseState"), UIA_ExpandCollapseExpandCollapseStatePropertyId },
	{ std::string("MultipleViewCurrentView"), UIA_MultipleViewCurrentViewPropertyId },
	{ std::string("MultipleViewSupportedViews"), UIA_MultipleViewSupportedViewsPropertyId },
	{ std::string("WindowCanMaximize"), UIA_WindowCanMaximizePropertyId },
	{ std::string("WindowCanMinimize"), UIA_WindowCanMinimizePropertyId },
	{ std::string("WindowWindowVisualState"), UIA_WindowWindowVisualStatePropertyId },
	{ std::string("WindowWindowInteractionState"), UIA_WindowWindowInteractionStatePropertyId },
	{ std::string("WindowIsModal"), UIA_WindowIsModalPropertyId },
	{ std::string("WindowIsTopmost"), UIA_WindowIsTopmostPropertyId },
	{ std::string("SelectionItemIsSelected"), UIA_SelectionItemIsSelectedPropertyId },
	{ std::string("SelectionItemSelectionContainer"), UIA_SelectionItemSelectionContainerPropertyId },
	{ std::string("TableRowHeaders"), UIA_TableRowHeadersPropertyId },
	{ std::string("TableColumnHeaders"), UIA_TableColumnHeadersPropertyId },
	{ std::string("TableRowOrColumnMajor"), UIA_TableRowOrColumnMajorPropertyId },
	{ std::string("TableItemRowHeaderItems"), UIA_TableItemRowHeaderItemsPropertyId },
	{ std::string("TableItemColumnHeaderItems"), UIA_TableItemColumnHeaderItemsPropertyId },
	{ std::string("ToggleToggleState"), UIA_ToggleToggleStatePropertyId },
	{ std::string("TransformCanMove"), UIA_TransformCanMovePropertyId },
	{ std::string("TransformCanResize"), UIA_TransformCanResizePropertyId },
	{ std::string("TransformCanRotate"), UIA_TransformCanRotatePropertyId },
	{ std::string("IsLegacyIAccessiblePatternAvailable"), UIA_IsLegacyIAccessiblePatternAvailablePropertyId },
	{ std::string("LegacyIAccessibleChildId"), UIA_LegacyIAccessibleChildIdPropertyId },
	{ std::string("LegacyIAccessibleName"), UIA_LegacyIAccessibleNamePropertyId },
	{ std::string("LegacyIAccessibleValue"), UIA_LegacyIAccessibleValuePropertyId },
	{ std::string("LegacyIAccessibleDescription"), UIA_LegacyIAccessibleDescriptionPropertyId },
	{ std::string("LegacyIAccessibleRole"), UIA_LegacyIAccessibleRolePropertyId },
	{ std::string("LegacyIAccessibleState"), UIA_LegacyIAccessibleStatePropertyId },
	{ std::string("LegacyIAccessibleHelp"), UIA_LegacyIAccessibleHelpPropertyId },
	{ std::string("LegacyIAccessibleKeyboardShortcut"), UIA_LegacyIAccessibleKeyboardShortcutPropertyId },
	{ std::string("LegacyIAccessibleSelection"), UIA_LegacyIAccessibleSelectionPropertyId },
	{ std::string("LegacyIAccessibleDefaultAction"), UIA_LegacyIAccessibleDefaultActionPropertyId },
	{ std::string("AriaRole"), UIA_AriaRolePropertyId },
	{ std::string("AriaProperties"), UIA_AriaPropertiesPropertyId },
	{ std::string("IsDataValidForForm"), UIA_IsDataValidForFormPropertyId },
	{ std::string("ControllerFor"), UIA_ControllerForPropertyId },
	{ std::string("DescribedBy"), UIA_DescribedByPropertyId },
	{ std::string("FlowsTo"), UIA_FlowsToPropertyId },
	{ std::string("ProviderDescription"), UIA_ProviderDescriptionPropertyId },
	{ std::string("IsItemContainerPatternAvailable"), UIA_IsItemContainerPatternAvailablePropertyId },
	{ std::string("IsVirtualizedItemPatternAvailable"), UIA_IsVirtualizedItemPatternAvailablePropertyId },
	{ std::string("IsSynchronizedInputPatternAvailable"), UIA_IsSynchronizedInputPatternAvailablePropertyId },
	{ std::string("OptimizeForVisualContent"), UIA_OptimizeForVisualContentPropertyId },
	{ std::string("IsObjectModelPatternAvailable"), UIA_IsObjectModelPatternAvailablePropertyId },
	{ std::string("AnnotationAnnotationTypeId"), UIA_AnnotationAnnotationTypeIdPropertyId },
	{ std::string("AnnotationAnnotationTypeName"), UIA_AnnotationAnnotationTypeNamePropertyId },
	{ std::string("AnnotationAuthor"), UIA_AnnotationAuthorPropertyId },
	{ std::string("AnnotationDateTime"), UIA_AnnotationDateTimePropertyId },
	{ std::string("AnnotationTarget"), UIA_AnnotationTargetPropertyId },
	{ std::string("IsAnnotationPatternAvailable"), UIA_IsAnnotationPatternAvailablePropertyId },
	{ std::string("IsTextPattern2Available"), UIA_IsTextPattern2AvailablePropertyId },
	{ std::string("StylesStyleId"), UIA_StylesStyleIdPropertyId },
	{ std::string("StylesStyleName"), UIA_StylesStyleNamePropertyId },
	{ std::string("StylesFillColor"), UIA_StylesFillColorPropertyId },
	{ std::string("StylesFillPatternStyle"), UIA_StylesFillPatternStylePropertyId },
	{ std::string("StylesShape"), UIA_StylesShapePropertyId },
	{ std::string("StylesFillPatternColor"), UIA_StylesFillPatternColorPropertyId },
	{ std::string("StylesExtendedProperties"), UIA_StylesExtendedPropertiesPropertyId },
	{ std::string("IsStylesPatternAvailable"), UIA_IsStylesPatternAvailablePropertyId },
	{ std::string("IsSpreadsheetPatternAvailable"), UIA_IsSpreadsheetPatternAvailablePropertyId },
	{ std::string("SpreadsheetItemFormula"), UIA_SpreadsheetItemFormulaPropertyId },
	{ std::string("SpreadsheetItemAnnotationObjects"), UIA_SpreadsheetItemAnnotationObjectsPropertyId },
	{ std::string("SpreadsheetItemAnnotationTypes"), UIA_SpreadsheetItemAnnotationTypesPropertyId },
	{ std::string("IsSpreadsheetItemPatternAvailable"), UIA_IsSpreadsheetItemPatternAvailablePropertyId },
	{ std::string("Transform2CanZoom"), UIA_Transform2CanZoomPropertyId },
	{ std::string("IsTransformPattern2Available"), UIA_IsTransformPattern2AvailablePropertyId },
	{ std::string("LiveSetting"), UIA_LiveSettingPropertyId },
	{ std::string("IsTextChildPatternAvailable"), UIA_IsTextChildPatternAvailablePropertyId },
	{ std::string("IsDragPatternAvailable"), UIA_IsDragPatternAvailablePropertyId },
	{ std::string("DragIsGrabbed"), UIA_DragIsGrabbedPropertyId },
	{ std::string("DragDropEffect"), UIA_DragDropEffectPropertyId },
	{ std::string("DragDropEffects"), UIA_DragDropEffectsPropertyId },
	{ std::string("IsDropTargetPatternAvailable"), UIA_IsDropTargetPatternAvailablePropertyId },
	{ std::string("DropTargetDropTargetEffect"), UIA_DropTargetDropTargetEffectPropertyId },
	{ std::string("DropTargetDropTargetEffects"), UIA_DropTargetDropTargetEffectsPropertyId },
	{ std::string("DragGrabbedItems"), UIA_DragGrabbedItemsPropertyId },
	{ std::string("Transform2ZoomLevel"), UIA_Transform2ZoomLevelPropertyId },
	{ std::string("Transform2ZoomMinimum"), UIA_Transform2ZoomMinimumPropertyId },
	{ std::string("Transform2ZoomMaximum"), UIA_Transform2ZoomMaximumPropertyId },
	{ std::string("FlowsFrom"), UIA_FlowsFromPropertyId },
	{ std::string("IsTextEditPatternAvailable"), UIA_IsTextEditPatternAvailablePropertyId },
	{ std::string("IsPeripheral"), UIA_IsPeripheralPropertyId },
	{ std::string("IsCustomNavigationPatternAvailable"), UIA_IsCustomNavigationPatternAvailablePropertyId },
	{ std::string("PositionInSet"), UIA_PositionInSetPropertyId },
	{ std::string("SizeOfSet"), UIA_SizeOfSetPropertyId },
	{ std::string("Level"), UIA_LevelPropertyId },
	{ std::string("AnnotationTypes"), UIA_AnnotationTypesPropertyId },
	{ std::string("AnnotationObjects"), UIA_AnnotationObjectsPropertyId },
	{ std::string("LandmarkType"), UIA_LandmarkTypePropertyId },
	{ std::string("LocalizedLandmarkType"), UIA_LocalizedLandmarkTypePropertyId },
	{ std::string("FullDescription"), UIA_FullDescriptionPropertyId },
	{ std::string("FillColor"), UIA_FillColorPropertyId },
	{ std::string("OutlineColor"), UIA_OutlineColorPropertyId },
	{ std::string("FillType"), UIA_FillTypePropertyId },
	{ std::string("VisualEffects"), UIA_VisualEffectsPropertyId },
	{ std::string("OutlineThickness"), UIA_OutlineThicknessPropertyId },
	{ std::string("CenterPoint"), UIA_CenterPointPropertyId },
	{ std::string("Rotation"), UIA_RotationPropertyId },
	{ std::string("Size"), UIA_SizePropertyId },
	{ std::string("IsSelectionPattern2Available"), UIA_IsSelectionPattern2AvailablePropertyId },
	{ std::string("Selection2FirstSelectedItem"), UIA_Selection2FirstSelectedItemPropertyId },
	{ std::string("Selection2LastSelectedItem"), UIA_Selection2LastSelectedItemPropertyId },
	{ std::string("Selection2CurrentSelectedItem"), UIA_Selection2CurrentSelectedItemPropertyId },
	{ std::string("Selection2ItemCount"), UIA_Selection2ItemCountPropertyId },
	{ std::string("HeadingLevel"), UIA_HeadingLevelPropertyId },
	{ std::string("IsDialog"), UIA_IsDialogPropertyId },
};

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

UICacheRequest::UICacheRequest(WinUIAuto& owner)
{
	owner.getAutomation()->CreateCacheRequest(UI(cache));
	cache->AddProperty(UIA_RuntimeIdPropertyId);
	cache->AddProperty(UIA_BoundingRectanglePropertyId);
	cache->AddProperty(UIA_ProcessIdPropertyId);
	cache->AddProperty(UIA_ControlTypePropertyId);
	cache->AddProperty(UIA_HasKeyboardFocusPropertyId);
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
			}
			UIA_HWND hWnd;
			if (SUCCEEDED(element->get_CurrentNativeWindowHandle(&hWnd))) {
				json["Handle"] = (int64_t)hWnd;
			}
			return true;
		}
	}
	return false;
}

#define SET_JSON(key, method) { UIString name; if (SUCCEEDED(element->method(&name))) json[key] = WC2MB(name); }

JSON WinUIAuto::info(IUIAutomationElement* element, UICacheRequest& cache, int64_t level)
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

	BOOL focus;
	if (SUCCEEDED(element->get_CachedHasKeyboardFocus(&focus))) {
		json["Focus"] = (bool)focus;
	}

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

	UIAutoUniquePtr<IUIAutomationTreeWalker> walker;
	pAutomation->get_ControlViewWalker(UI(walker));
	if (level > 0) {
		int64_t sublevel = level - 1;
		UIAutoUniquePtr<IUIAutomationElement> child;
		walker->GetFirstChildElementBuildCache(element, cache, UI(child));
		while (child) {
			json["Tree"].push_back(info(child.get(), cache, sublevel));
			walker->GetNextSiblingElementBuildCache(child.get(), cache, UI(child));
		}
	}
	else if (level < 0) {
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
		json.push_back(info(element.get(), cache));
	}
	return json;
}

WinUIAuto::WinUIAuto()
{
	hInitialize = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

	if (FAILED(hInitialize))
		throw std::runtime_error("CoInitialize error");

	if (FAILED(CoCreateInstance(CLSID_CUIAutomation,
		NULL, CLSCTX_INPROC_SERVER, IID_IUIAutomation,
		reinterpret_cast<void**>(&UI(pAutomation)))))
		throw std::runtime_error("CoInitialize error");
}

WinUIAuto::~WinUIAuto()
{
	if (SUCCEEDED(hInitialize)) CoUninitialize();
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

std::string WinUIAuto::GetElements(DWORD pid, int64_t level)
{
	UICacheRequest cache(*this);
	cache->put_TreeScope(TreeScope_Subtree);
	UIAutoUniquePtr<IUIAutomationElement> owner;
	find(pid, cache, UI(owner));
	if (owner.get() == nullptr) return {};
	return info(owner.get(), cache, level).dump();
}

std::string WinUIAuto::GetElements(const std::string& id, int64_t level)
{
	UICacheRequest cache(*this);
	cache->put_TreeScope(TreeScope_Subtree);
	UIAutoUniquePtr<IUIAutomationElement> owner;
	find(id, cache, UI(owner));
	if (owner.get() == nullptr) return {};
	return info(owner.get(), cache, level).dump();
}

std::string WinUIAuto::ElementById(const std::string& id)
{
	UICacheRequest cache(*this);
	cache->put_TreeScope(TreeScope_Element);
	UIAutoUniquePtr<IUIAutomationElement> element;
	find(id, cache, UI(element));
	if (element.get() == nullptr) return {};
	return info(element.get(), cache).dump();
}

std::string WinUIAuto::ElementFromPoint(int x, int y)
{
	POINT p{ x, y };
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> element;
	if (FAILED(pAutomation->ElementFromPointBuildCache(p, cache, UI(element)))) return {};
	return info(element.get(), cache).dump();
}

static bool empty(UIAutoUniquePtr<IUIAutomationElementArray>& elements)
{
	if (elements.get() == nullptr) return true;
	int count = 0;
	elements->get_Length(&count);
	return count == 0;
}

std::vector<HWND> GetProcessWindows(DWORD pid)
{
	using EnumParam = std::pair<DWORD, std::vector<HWND>>;
	EnumParam param{ (DWORD)pid, {} };
	bool bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			if (::IsWindow(hWnd) && ::IsWindowVisible(hWnd)) {
				DWORD dwProcessId;
				auto p = (EnumParam*)lParam;
				::GetWindowThreadProcessId(hWnd, &dwProcessId);
				if (p->first == dwProcessId)
					p->second.push_back(hWnd);
			}
			return TRUE;
		}, (LPARAM)&param);
	return param.second;
}

std::string WinUIAuto::FindElements(const std::string& arg)
{
	DWORD pid = 0;
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> owner;

	auto filter = JSON::parse(arg);
	std::vector<IUIAutomationCondition*> conditions;
	std::vector<UIAutoUniquePtr<IUIAutomationCondition>> deleter;
	for (auto& element : filter.items()) {
		auto& it = properties.find(element.key());
		if (it == properties.end()) {
			JSON error = { {"error", "Property not found: " + element.key()} };
			return error.dump();
		}
		PROPERTYID propertyId = it->second;
		switch (propertyId) {
		case UIA_ProcessIdPropertyId: {
			pid = (unsigned int)element.value();
		} break;
		case UIA_ParentPropertyId: {
			std::string id = element.value();
			find(id, cache, UI(owner));
		} break;
		case UIA_NamePropertyId: {
			std::wstring value = MB2WC(element.value());
			UIAutoUniquePtr<IUIAutomationCondition> cond, name1, name2;
			pAutomation->CreatePropertyConditionEx(propertyId, CComVariant(value.c_str()), PropertyConditionFlags_IgnoreCase, UI(name1));
			pAutomation->CreatePropertyConditionEx(propertyId, CComVariant((value + L":").c_str()), PropertyConditionFlags_IgnoreCase, UI(name2));
			pAutomation->CreateOrCondition(name1.get(), name2.get(), UI(cond));
			conditions.push_back(cond.get());
			deleter.emplace_back(cond.release());
		} break;
		case UIA_ControlTypePropertyId: {
			std::string type = element.value();
			if (auto iType = str2type(type)) {
				UIAutoUniquePtr<IUIAutomationCondition> cond;
				pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, CComVariant((int)iType, VT_INT), UI(cond));
				conditions.push_back(cond.get());
				deleter.emplace_back(cond.release());
			}
		} break;
		default:
			UIAutoUniquePtr<IUIAutomationCondition> cond;
			if (element.value().is_string()) {
				std::wstring value = MB2WC(element.value());
				pAutomation->CreatePropertyConditionEx(propertyId, CComVariant(value.c_str()), PropertyConditionFlags_IgnoreCase, UI(cond));
			}
			else if (element.value().is_number_integer()) {
				auto value = (int)element.value();
				pAutomation->CreatePropertyCondition(propertyId, CComVariant(value), UI(cond));
			}
			else if (element.value().is_boolean()) {
				auto value = (bool)element.value();
				pAutomation->CreatePropertyCondition(propertyId, CComVariant(value), UI(cond));
			}
			if (cond) {
				conditions.push_back(cond.get());
				deleter.emplace_back(cond.release());
			}
		}
	}

	UIAutoUniquePtr<IUIAutomationCondition> cond;
	UIAutoUniquePtr<IUIAutomationElementArray> elements;
	pAutomation->CreateAndConditionFromNativeArray(conditions.data(), (int)conditions.size(), UI(cond));

	if (pid) {
		std::map<std::string, JSON> controls;
		for (auto hWnd : GetProcessWindows(pid)) {
			int length = 0;
			UICacheRequest cache(*this);
			UIAutoUniquePtr<IUIAutomationElement> owner;
			if (FAILED(pAutomation->ElementFromHandleBuildCache(hWnd, cache, UI(owner)))) continue;
			if (FAILED(owner->FindAllBuildCache(TreeScope_Subtree, cond.get(), cache, UI(elements)))) continue;
			if (!elements || FAILED(elements->get_Length(&length))) continue;
			for (int i = 0; i < length; ++i) {
				UIAutoUniquePtr<IUIAutomationElement> element;
				if (FAILED(elements->GetElement(i, UI(element)))) continue;
				auto json = info(element.get(), cache);
				controls[(std::string)json["Id"]] = json;
			}
		}
		JSON json;
		for (auto& it : controls) {
			json.push_back(it.second);
		}
		return json.dump();
	}
	else if (owner) {
		owner->FindAllBuildCache(TreeScope_Subtree, cond.get(), cache, UI(elements));
		return info(elements.get(), cache).dump();
	}
	else {
		JSON error = { {"error", "Process ID condition not found"} };
		return error.dump();
	}
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
	cache->put_TreeScope(TreeScope_Parent);
	UIAutoUniquePtr<IUIAutomationElement> child;
	find(id, cache, UI(child));
	if (child.get() == nullptr) return {};

	UIAutoUniquePtr<IUIAutomationTreeWalker> walker;
	pAutomation->get_ControlViewWalker(UI(walker));
	UIAutoUniquePtr<IUIAutomationElement> parent;
	walker->GetParentElementBuildCache(child.get(), cache, UI(parent));
	walker->NormalizeElementBuildCache(parent.get(), cache, UI(parent));
	if (parent.get() == nullptr) return {};
	return info(parent.get(), cache).dump();;
}

std::string WinUIAuto::GetNextElement(const std::string& id)
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> child;
	find(id, cache, UI(child));
	if (child.get() == nullptr) return {};

	UIAutoUniquePtr<IUIAutomationTreeWalker> walker;
	pAutomation->get_ControlViewWalker(UI(walker));
	UIAutoUniquePtr<IUIAutomationElement> parent;
	walker->GetNextSiblingElementBuildCache(child.get(), cache, UI(parent));
	if (parent.get() == nullptr) return {};
	return info(parent.get(), cache).dump();
}

std::string WinUIAuto::GetPreviousElement(const std::string& id)
{
	UICacheRequest cache(*this);
	UIAutoUniquePtr<IUIAutomationElement> child;
	find(id, cache, UI(child));
	if (child.get() == nullptr) return {};

	UIAutoUniquePtr<IUIAutomationTreeWalker> walker;
	pAutomation->get_ControlViewWalker(UI(walker));
	UIAutoUniquePtr<IUIAutomationElement> parent;
	walker->GetPreviousSiblingElementBuildCache(child.get(), cache, UI(parent));
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

void WinUIAuto::setMonitoringStatus(AddInNative* addin)
{
	if (addin) {
		pAutoHandler.reset(UIAutoHandler::CreateInstance(*this, addin));
	}
	else {
		pAutoHandler.reset();
	}
}

bool WinUIAuto::getMonitoringStatus()
{
	return pAutoHandler.get();
}

UIAutoHandler* UIAutoHandler::CreateInstance(WinUIAuto& owner, AddInNative* addin)
{
	return new UIAutoHandler(owner, addin);
}

UIAutoHandler::UIAutoHandler(WinUIAuto& owner, AddInNative* addin)
	: m_owner(owner), m_cache(owner), m_addin(addin)
{
	m_owner.getAutomation()->AddFocusChangedEventHandler(m_cache, this);
}

void UIAutoHandler::ResetHandler()
{
	m_owner.getAutomation()->RemoveFocusChangedEventHandler(this);
	m_addin = nullptr;
}

HRESULT UIAutoHandler::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;

	*ppvObj = NULL;
	if (riid == IID_IUnknown || riid == IID_IUIAutomationFocusChangedEventHandler)
	{
		// Increment the reference count and return the pointer.
		*ppvObj = (LPVOID)this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

ULONG UIAutoHandler::AddRef()
{
	InterlockedIncrement(&m_count);
	return m_count;
}

ULONG UIAutoHandler::Release()
{
	// Decrement the object's internal counter.
	ULONG ulRefCount = InterlockedDecrement(&m_count);
	if (0 == m_count)
	{
		delete this;
	}
	return ulRefCount;
}

HRESULT UIAutoHandler::HandleFocusChangedEvent(IUIAutomationElement* element)
{
	if (m_addin && element) {
		auto json = m_owner.info(element, m_cache);
		std::u16string text = MB2WCHAR(json.dump());
		m_addin->ExternalEvent(u"FOCUS_CHANGED", text);
	}
	return S_OK;
}

#endif//_WINDOWS
