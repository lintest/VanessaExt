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
		[&](VH data) {  this->result = this->provider->ParseText(data); }
	);

	AddFunction(u"ParseText", u"ПрочитатьТекст",
		[&](VH data) {  this->result = this->provider->ParseText(data); }
	);

	AddFunction(u"ParseFolder", u"ПрочитатьПапку",
		[&](VH filepath, VH filter) {  this->result = this->provider->ParseFolder(filepath, filter); }, { {1, u""} }
	);

	AddFunction(u"ParseFile", u"ПрочитатьФайл",
		[&](VH filepath) {  this->result = this->provider->ParseFile(filepath); }
	);

	AddProcedure(u"Exit", u"ЗавершитьРаботуСистемы",
		[&](VH status) { this->ExitCurrentProcess(status); }, { {0, (int64_t)0 } }
	);

#ifdef _WINDOWS
	CreateProgressMonitor();

	AddProcedure(u"ScanFolder", u"СканироватьПапку",
		[&](VH filepath, VH filter) {  this->ScanFolder(filepath, filter); }, { {1, u""} }
	);

	AddProcedure(u"AbortScan", u"ПрерватьСканирование",
		[&]() {  this->AbortScan(); }
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
	std::string step;
public:
	GherkinProgress(Gherkin::GherkinProvider& provider, const std::wstring& path, const std::string& filter, HWND hWnd)
		: provider(provider), path(path), filter(filter), hWnd(hWnd) {}
	virtual void Start(size_t max, const std::string& step) override {
		this->pos = 0;
		this->max = max;
		this->step = step;
	}
	virtual void Step(const boost::filesystem::path& path) override {
		Send(JSON({
			{ "pos", ++pos},
			{ "max", max },
			{ "step", step },
			{ "path", WC2MB(path.wstring()) },
			{ "name", WC2MB(path.filename().wstring()) } }
		).dump());
	}
	virtual void Send(const std::string& msg) override {
		auto data = (LPARAM)new std::string(msg);
		::SendMessageW(hWnd, WM_PARSING_PROGRESS, 0, data);
	}
	const Gherkin::GherkinProvider& provider;
	const std::wstring path;
	const std::string filter;
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
	auto text = provider.ParseFolder(path, filter, this);
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

void GherkinParser::ScanFolder(const std::wstring& path, const std::string& filter)
{
	auto progress = new GherkinProgress(*provider, path, filter, hWndMonitor);
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
