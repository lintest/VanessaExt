#include "GherkinParser.h"
#include "gherkin.h"

std::vector<std::u16string> GherkinParser::names = {
	AddComponent(u"GherkinParser", []() { return new GherkinParser; }),
};

GherkinParser::GherkinParser()
{
	provider.reset(new Gherkin::GherkinProvider);

	AddProperty(u"Keywords", u"КлючевыеСлова",
		[&](VH value) { value = this->provider->getKeywords(); },
		[&](VH value) { this->provider->setKeywords(value); }
	);

	AddProperty(u"PrimitiveEscaping", u"ПримитивноеЭкранирование",
		[&](VH value) { value = this->provider->primitiveEscaping; },
		[&](VH value) { this->provider->primitiveEscaping = value; }
	);

	AddFunction(u"Parse", u"Прочитать",
		[&](VH data) { this->result = this->provider->ParseText(data); }
	);

	AddFunction(u"ParseText", u"ПрочитатьТекст",
		[&](VH data) { this->result = this->provider->ParseText(data); }
	);

	AddFunction(u"ParseFolder", u"ПрочитатьПапку",
		[&](VH dirs, VH libs, VH filter) { this->result = this->provider->ParseFolder(dirs, libs, filter); }, 
		{ {1, u"[]"}, {2, u""} }
	);

	AddFunction(u"ParseFile", u"ПрочитатьФайл",
		[&](VH file, VH libs) { this->result = this->provider->ParseFile(file, libs); }, { {1, u"[]"} }
	);

	AddFunction(u"GetCashe", u"ПолучитьКэш",
		[&]() { this->result = this->provider->GetCashe(); }
	);

	AddProcedure(u"ClearCashe", u"ОчиститьКэш",
		[&](VH filename) { this->provider->ClearSnippets((std::wstring)filename); }, { {0, u""} }
	);

	AddProcedure(u"Exit", u"ЗавершитьРаботуСистемы",
		[&](VH status) { this->ExitCurrentProcess(status); }, { {0, (int64_t)0 } }
	);

#ifdef _WINDOWS
	CreateProgressMonitor();

	AddProcedure(u"ScanFolder", u"СканироватьПапку",
		[&](VH dirs, VH libs, VH filter) { this->ScanFolder(dirs, libs, filter); },
		{ {1, u"[]"}, { 2, u"" } }
	);

	AddProcedure(u"AbortScan", u"ПрерватьСканирование",
		[&]() { this->AbortScan(); }
	);
#endif//_WINDOWS
}

#ifdef _WINDOWS

class GherkinProgress
	: public Gherkin::AbstractProgress {
private:
	HWND hWnd;
	size_t max = 0;
	size_t pos = 0;
	std::string dir;
	std::string step;
public:
	GherkinProgress(Gherkin::GherkinProvider& provider, const std::string& dirs, const std::string& libs, const std::string& tags, HWND hWnd)
		: provider(provider), dirs(dirs), libs(libs), tags(tags), hWnd(hWnd) {}
	virtual void Start(const std::string& dir, size_t max, const std::string& step) override {
		this->dir = dir;
		this->max = max;
		this->pos = 0;
		this->step = step;
	}
	virtual void Step(const boost::filesystem::path& path) override {
		Send(JSON({
			{ "dir", dir },
			{ "max", max },
			{ "pos", ++pos },
			{ "step", step },
			{ "path", WC2MB(path.wstring()) },
			{ "name", WC2MB(path.filename().wstring()) } }
		).dump());
	}
	virtual void Send(const std::string& msg) override {
		auto data = (LPARAM)new std::string(msg);
		::SendMessageW(hWnd, WM_PARSING_PROGRESS, 0, data);
	}
	Gherkin::GherkinProvider& provider;
	const std::string dirs;
	const std::string libs;
	const std::string tags;
	void Scan();
};

GherkinParser::~GherkinParser()
{
	::DestroyWindow(hWndMonitor);
}

void GherkinParser::ExitCurrentProcess(int64_t status)
{
	ExitProcess((UINT)status);
}

static LRESULT CALLBACK MonitorWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PARSING_PROGRESS:
	case WM_PARSING_FINISHED: {
		auto component = (GherkinParser*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (component) component->OnProgress(message, *(std::string*)lParam);
		return 0;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void GherkinProgress::Scan() {
	auto text = provider.ParseFolder(dirs, libs, tags, this);
	auto data = (LPARAM)new std::string(text);
	::SendMessageW(hWnd, WM_PARSING_FINISHED, 0, data);
}

static DWORD WINAPI ParserThreadProc(LPVOID lpParam)
{
	std::unique_ptr<GherkinProgress> progress((GherkinProgress*)lpParam);
	progress->Scan();
	return 0;
}

void GherkinParser::CreateProgressMonitor()
{
	const LPCWSTR wsClassName = L"VanessaParserMonitor";

	WNDCLASS wndClass = {};
	wndClass.hInstance = hModule;
	wndClass.lpszClassName = wsClassName;
	wndClass.lpfnWndProc = MonitorWndProc;
	RegisterClass(&wndClass);

	hWndMonitor = CreateWindowW(wsClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
	SetWindowLongPtr(hWndMonitor, GWLP_USERDATA, (LONG_PTR)this);
}

void GherkinParser::OnProgress(UINT id, const std::string& data)
{
	std::u16string message = id == WM_PARSING_FINISHED ? u"PARSING_FINISHED" : u"PARSING_PROGRESS";
	ExternalEvent(message, MB2WCHAR(data));
}

void GherkinParser::ScanFolder(const std::string& dirs, const std::string& libs, const std::string& filter)
{
	auto progress = new GherkinProgress(*provider, dirs, libs, filter, hWndMonitor);
	CreateThread(0, NULL, ParserThreadProc, (LPVOID)progress, NULL, NULL);
}

void GherkinParser::AbortScan()
{
	provider->AbortScan();
}

#else//_WINDOWS

void GherkinParser::ExitCurrentProcess(int64_t status)
{
	exit((int)status);
}

#endif//_WINDOWS
