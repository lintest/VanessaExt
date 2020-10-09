#include "stdafx.h"
#include "ClipboardControl.h"
#include "ClipboardManager.h"

std::vector<std::u16string> ClipboardControl::names = {
	AddComponent(u"ClipboardControl", []() { return new ClipboardControl; }),
};

ClipboardControl::ClipboardControl()
{
	AddProperty(u"Text", u"Текст",
		[&](VH var) { var = ClipboardManager().GetText(); },
		[&](VH var) { ClipboardManager().SetText(var); }
	);
	AddProperty(u"Files", u"Файлы", 
		[&](VH var) { var = ClipboardManager().GetFiles(); },
		[&](VH var) { ClipboardManager().SetFiles(var); }
	);
	AddProperty(u"Image", u"Картинка", 
		[&](VH var) { ClipboardManager().GetImage(var); },
		[&](VH var) { ClipboardManager().SetImage(var); }
	);
	AddProperty(u"Format", u"Формат",
		[&](VH var) { var = ClipboardManager().GetFormat(); }
	);
	AddProperty(u"Version", u"Версия", 
		[&](VH var) { var = this->version(); }
	);

	AddFunction(u"Empty", u"Очистить", [&]() { this->result = ClipboardManager().Empty(); });
	AddFunction(u"SetText", u"ЗаписатьТекст", [&](VH var) { this->result = ClipboardManager().SetText(var); });
	AddFunction(u"SetFiles", u"ЗаписатьФайлы", [&](VH var) { this->result = ClipboardManager().SetFiles(var); });
	AddFunction(u"SetImage", u"ЗаписатьКартинку", [&](VH var) { this->result = ClipboardManager().SetImage(var); });
}
