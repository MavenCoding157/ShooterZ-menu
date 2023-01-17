#include "gui.h"
#include <cstdlib>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

#include "globals.h"
#include <corecrt_math.h>

#include <windows.h>
#include <shellapi.h>
#include "../imgui/imgui_toggle.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "class001";
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		0,
		"class001",
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 17.f);

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}
//bools
static bool fov = false;
static bool showing = true;
static float size = 450;
static bool my_bool;
bool show_circle_overlay = false;
HWND target_hwnd;
bool show_exit_button = true;//del

static bool values[] = { true, true, true, true, true, true, true, true };
size_t value_index = 0;

HWND GetApplicationWindowHandle(const char* windowName)
{
	return FindWindowA(NULL, windowName);
}

void DrawCircleOverlay(HWND hwnd)
{
	if (!show_circle_overlay) return;
	// Get the client rect of the window
	RECT rect;
	GetClientRect(hwnd, &rect);

	// Get the position of the window
	POINT point;
	point.x = rect.left;
	point.y = rect.top;
	ClientToScreen(hwnd, &point);

	ImGui::SetNextWindowPos(ImVec2((float)point.x, (float)point.y));
	ImGui::SetNextWindowSize(ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top)));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddCircle(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), 100, IM_COL32(255, 255, 255, 255));

	ImGui::PopStyleColor();
	ImGui::End();
}


void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"Ranos Cheat menu",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);

	void renderUI();
	{
		if (show_exit_button) {
			if (ImGui::Button("PANIC BUTTON")) {
				exit(0);
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Exits program");
	}

	ImGui::Checkbox("ESP", &globals::glow);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("See players through walls");

	ImGui::ColorEdit4("ESP colour", globals::glowColor);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Choose colours");

	ImGui::Checkbox("Radar hack", &globals::radar);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("See players on radar");

	ImGui::Checkbox("FOV Circle (coming soon)", &show_circle_overlay);
	target_hwnd = GetApplicationWindowHandle("Counter-Strike: Global Offensive");
	DrawCircleOverlay(target_hwnd);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Makes you have better aim");

	ImGui::Checkbox("Chams", &globals::chams);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("See players better");

	ImGui::Checkbox("bhop", &globals::bhop);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Makes movement easier");

	ImGui::Checkbox("Anti-Flash", &globals::flashDur);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Cant be flashed");

	if (ImGui::Button("My GitHub"))
		ShellExecute(0, 0, "https://github.com/MavenCoding157", 0, 0, SW_SHOW);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("My GitHub");

	if (ImGui::Button("My Youtube"))
		ShellExecute(0, 0, "https://www.youtube.com/channel/UCkP2YjZfvZIfArYbAUyRLsg", 0, 0, SW_SHOW);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("My Youtube");

	if (ImGui::Button("FREE GAMES"))
		ShellExecute(0, 0, "https://drive.google.com/drive/folders/1myezzlndx8HAv9wtVErD2hoIAP_5Q4g-", 0, 0, SW_SHOW);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("FREE GAMES");

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	float samples[100]{};
	for (int n = 0; n < 100; n++)
		samples[n] = sinf(n * 0.2f + ImGui::GetTime() * 1.5f);
	ImGui::PlotLines("", samples, 100);

	ImGui::End();
}


	