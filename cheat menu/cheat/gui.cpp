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

	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowTitleAlign = { 0.5f, 0.5f };
	style->WindowPadding = { 15, 15 };
	style->ChildRounding = 2.f;
	style->WindowRounding = 0.f;
	style->ScrollbarRounding = 1.f;
	style->FrameRounding = 2.f;
	style->ItemSpacing = { 8, 8 };
	style->ScrollbarSize = 3.f;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.96f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.32f, 0.32f, 0.58f, 0.30f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.17f, 0.00f, 0.52f, 0.26f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.27f, 0.38f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.29f, 0.37f, 0.62f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.33f, 0.33f, 0.67f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.33f, 0.33f, 0.68f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.33f, 0.33f, 0.67f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.81f, 0.66f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.35f, 0.37f, 0.48f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.35f, 0.49f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.33f, 0.33f, 0.67f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.42f, 0.32f, 0.67f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.41f, 0.73f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.33f, 0.33f, 0.67f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.58f, 0.50f, 1.00f, 0.35f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.29f, 0.84f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.24f, 0.80f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", 17.f);

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
bool show_exit_button = true;
static auto current_tab = 0;

static bool values[] = { true, true, true, true, true, true, true, true };
size_t value_index = 0;
bool my_checkbox_state = false;
constexpr auto butn_tall = 48;

int buttonPressCount = 0;

int status = system("tasklist | find /i \"csgo.exe\" > nul");//del


void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"ShooterZ Menu", //spaces so it goes in the middle
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);
	

	if (ImGui::BeginChild(
		1,
		{ ImGui::GetContentRegionAvail().x * 0.25f, ImGui::GetContentRegionAvail().y },
		true)) {
		constexpr auto button_height = 48;
		if (ImGui::Button("Home", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 9; }
		if (ImGui::Button("ESP", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 0; }
		if (ImGui::Button("Thirdperson", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 1; }
		if (ImGui::Button("Misc", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 2; }
		if (ImGui::Button("Other", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 3; }
		if (ImGui::Button("Configs", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 4; }
		if (ImGui::Button("About", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 5; }
		if (ImGui::Button("Thank you", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 6; }
		if (ImGui::Button("csgo Status", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 8; }
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Only works is application started before hand");
		if (ImGui::Button(" Creator:\nShooterZ", { ImGui::GetContentRegionAvail().x, button_height })) { current_tab = 7; }

		ImGui::EndChild();
	}

	ImGui::SameLine();

	if (ImGui::BeginChild(
		2,
		ImGui::GetContentRegionAvail(),
		true)) {

		switch (current_tab) {
		case 9:
			ImGui::BeginChild("Home", ImVec2(150, 0), true);
			{
				ImGui::Text("Hello, Welcome to \nShooterZ menu. \nThis is just a \nprototype and is \nstill a work in \nprogress. If \nanything doesnt \nwork make sure to\ncontact me \nvia my email\non my GitHub page.");
				// Draw left section contents
			}
			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("REMEMBER", ImVec2(0, 0), true);
			{
				ImGui::Text("Check out my Youtube");
				
				if (ImGui::Button("Youtube", { ImGui::GetContentRegionAvail().x, butn_tall })) {
					ShellExecute(0, 0, "https://www.youtube.com/channel/UCkP2YjZfvZIfArYbAUyRLsg", 0, 0, SW_SHOW);
					
				}
				
			}
			ImGui::EndChild();
			break;

		case 0:
			ImGui::Toggle("ESP", &globals::glow);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("See players through walls");

			ImGui::ColorEdit4("ESP colour", globals::glowColor);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Choose colours");

			ImGui::Toggle("Radar hack", &globals::radar);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("See players on radar");

			ImGui::Toggle("Chams", &globals::chams);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("See players better");

			break;

		case 1:
			ImGui::Toggle("Thirdperson", &globals::thirdperson);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("A buggy thirdperson");

			ImGui::Toggle("Thirdperson FOV Changer", &globals::FOV);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Changes FOV");

			ImGui::Toggle("Normal thirdperson FOV", &globals::norFOV);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Changes FOV back to normal");

			break;

		case 2:
			ImGui::Toggle("bhop", &globals::bhop);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Makes movement easier");

			ImGui::Toggle("Anti-Flash", &globals::flashDur);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Cant be flashed");

			ImGui::Toggle("FOV Changer", &globals::FOV);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Changes FOV");

			ImGui::Toggle("Normal FOV", &globals::norFOV);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Changes FOV back to normal");

			break;

		case 3:
			void renderUI();
			{
				if (show_exit_button) {
					if (ImGui::Button("PANIC BUTTON", { ImGui::GetContentRegionAvail().x, butn_tall })) {
						exit(0);
					}
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Exits program");
			}

			if (ImGui::Button("My GitHub", { ImGui::GetContentRegionAvail().x, butn_tall }))
				ShellExecute(0, 0, "https://github.com/MavenCoding157", 0, 0, SW_SHOW);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("My GitHub");

			if (ImGui::Button("My Youtube", { ImGui::GetContentRegionAvail().x, butn_tall }))
				ShellExecute(0, 0, "https://www.youtube.com/channel/UCkP2YjZfvZIfArYbAUyRLsg", 0, 0, SW_SHOW);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("My Youtube");

			if (ImGui::Button("FREE GAMES", { ImGui::GetContentRegionAvail().x, butn_tall }))
				ShellExecute(0, 0, "https://drive.google.com/drive/folders/1myezzlndx8HAv9wtVErD2hoIAP_5Q4g-", 0, 0, SW_SHOW);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("FREE GAMES");

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			break;

		case 4:
			if (ImGui::Button("Legit config", { ImGui::GetContentRegionAvail().x, butn_tall })) {
				globals::glow = true;

				globals::radar = true;

				globals::bhop = true;

				globals::flashDur = true;
			}

			if (ImGui::Button("Crazy config", { ImGui::GetContentRegionAvail().x, butn_tall })) {
				globals::glow = true;

				globals::radar = true;

				globals::chams = true;

				globals::bhop = true;

				globals::flashDur = true;

				globals::FOV = true;
			}

			break;

		case 5:
			ImGui::Text("This is the first C++ hack from the\ncreator of this, me 'ShooterZ'\n(or my GitHub name 'MavenCoding157'). \nthis was originally meant to be a \nC# hack but the imgui design appealed more. \nAlso i have mainly used C# in\nthe past but i have decided to\n use C++ for this and the ImGui\nframework as it appeals to me more.");
			break;

		case 6:
			ImGui::Text("Thank you");

			ImGui::Text("Thank you for downloading\nthis tool and please\ncheck out some of\nmy other tools aswell");

			if (ImGui::Button("Press this if you love 'ShooterZ' tool")) {
				buttonPressCount++;
			}

			ImGui::Text("Button press count: %d", buttonPressCount);
			break;

		case 8:
			ImGui::Begin("Proccess status");

			if (status == 0) {
				ImGui::Text("csgo is running");
			}
			else {
				
				ImGui::Text("csgo is not running");
			}

			ImGui::PopStyleColor();

			ImGui::End();
			break;

		case 7:
			break;

		}

		ImGui::EndChild();
	}

	ImGui::End();
}


	