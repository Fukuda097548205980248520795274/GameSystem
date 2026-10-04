#include "ImGuiRender.h"
#include "Engine.h"
#include <cassert>
#include <filesystem>

/// @brief デストラクタ
Detail::ImGuiRender::~ImGuiRender()
{
	// ImNodesの破棄
	ImNodes::DestroyContext();

	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

/// @brief 初期化
/// @param device 
/// @param winApp 
/// @param heap 
/// @param swapChain 
void Detail::ImGuiRender::Initialize(ID3D12Device* device, WinApp* winApp, DXHeap* heap, DXSwapChain* swapChain)
{
	// nullptrチェック
	assert(device);
	assert(winApp);
	assert(heap);
	assert(swapChain);

	// 引数を受け取る
	winApp_ = winApp;

	// SRVハンドルを取得する
	srvHandle_ = heap->GetSrvDescriptorHandle();

	// サイズを取得
	screenWidth_ = static_cast<float>(swapChain->GetSwapChainDesc().Width);
	screenHeight_ = static_cast<float>(swapChain->GetSwapChainDesc().Height);


	// ImGuiを初期化する
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// ImNodesの初期化
	ImNodes::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	namespace fs = std::filesystem;


	ImFontConfig config = {};
	config.SizePixels = 12.0f;

	const char* fontPath = "C:/Windows/Fonts/YuGothB.ttc";

	if (fs::exists(fontPath))
	{
		ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, config.SizePixels, &config, io.Fonts->GetGlyphRangesJapanese());

		if (font)
		{
			io.FontDefault = font;
			io.FontGlobalScale = 1.0f;
			io.Fonts->Build();
		}
	}
	else
	{
		OutputDebugStringA("フォントファイルが存在しません: YuGothB.ttc\n");
	}

	// ドッキング機能を有効にする
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// imgui.iniの読み込み
	std::fstream f("imgui.ini");

	// ファイルが存在すれば読み込む、なければデフォルトのレイアウトを読み込む
	if (f.is_open())
	{
		f.close();
	}
	else
	{
		// デフォルトのレイアウトを読み込む
		//ImGui::LoadIniSettingsFromMemory(defaultImguiIni);
	}

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp_->GetHwnd());
	ImGui_ImplDX12_Init(device, swapChain->GetSwapChainDesc().BufferCount,
		swapChain->GetRtvDesc().Format, heap->GetSrvDescriptorHeap(), srvHandle_.cpuHandle, srvHandle_.gpuHandle);
}

/// @brief リサイズ
/// @param width 
/// @param height 
void Detail::ImGuiRender::Resize(int32_t width, int32_t height)
{
	screenWidth_ = static_cast<float>(width);
	screenHeight_ = static_cast<float>(height);
}

/// @brief フレーム開始
void Detail::ImGuiRender::FrameStart()
{
	// フレームの開始をImGuiに伝える
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();

	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(true);
}

/// @brief Dockスペースを作成する
void Detail::ImGuiRender::CreateDockSpace()
{
	static bool opt_fullscreen = true;
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// パディングを0に（メインDockSpaceの余白をなくす）
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("DockSpace", nullptr, window_flags);

	ImGui::PopStyleVar(3); // WindowPadding, Rounding, BorderSizeを戻す

	// DockSpace作成（バーなし、背景のみ）
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	ImGui::End();
}

/// @brief ImGuiスクリーンを描画する
/// @param resource 
/// @param gpuHandle 
/// @param commandList 
void Detail::ImGuiRender::DrawImGuiScreen(OffscreenResource* currentOffscreen, ID3D12GraphicsCommandList* commandList, int32_t frameIndex)
{
	// nullptrチェック
	if (currentOffscreen == nullptr)return;

	ImGui::Begin("View");

	ImTextureID texId = (ImTextureID)(currentOffscreen->GetSrvHandle(frameIndex).gpuHandle.ptr);

	ImVec2 availSize = ImGui::GetContentRegionAvail(); // ウィンドウ内の空きサイズ

	float aspectRatio = screenWidth_ / screenHeight_;

	// アスペクト比を保ちつつ、ウィンドウサイズ内に最大表示
	ImVec2 imageSize;

	float availAspect = availSize.x / availSize.y;
	if (availAspect > aspectRatio) {
		// 横に余裕あり → 高さに合わせる
		imageSize.y = availSize.y;
		imageSize.x = availSize.y * aspectRatio;
	}
	else {
		imageSize.x = availSize.x;
		imageSize.y = availSize.x / aspectRatio;
	}

	// 中央寄せ（X方向、Y方向両方）
	ImVec2 cursorPos = ImGui::GetCursorPos();
	ImVec2 newCursorPos = ImVec2(
		cursorPos.x + (availSize.x - imageSize.x) * 0.5f,
		cursorPos.y + (availSize.y - imageSize.y) * 0.5f
	);

	// 画像が描かれている領域をそのままギズモの Rect にする
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 gizmoPos = ImVec2(windowPos.x + newCursorPos.x,
		windowPos.y + newCursorPos.y);


	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

	ImGuizmo::SetRect(gizmoPos.x, gizmoPos.y, imageSize.x, imageSize.y);

	ImGui::SetCursorPos(newCursorPos);

	ImGui::Image(texId, imageSize);


	// Imageの矩形取得
	ImVec2 imageMin = ImGui::GetItemRectMin();

	// マウス
	ImVec2 mousePos = ImGui::GetMousePos();

	float renderWidth = static_cast<float>(winApp_->GetClientWidth());
	float renderHeight = static_cast<float>(winApp_->GetClientHeight());

	viewWindowCursorPos_.x = (mousePos.x - imageMin.x) * (renderWidth / imageSize.x);
	viewWindowCursorPos_.y = (mousePos.y - imageMin.y) * (renderHeight / imageSize.y);

	// ウィンドウ内をホバーしているかどうか
	isViewWindowHover_ = ImGui::IsItemHovered();

	// ローカル座標
	viewWindowCursorPos_.x = std::clamp(viewWindowCursorPos_.x, 0.0f, renderWidth);
	viewWindowCursorPos_.y = std::clamp(viewWindowCursorPos_.y, 0.0f, renderHeight);


	ImGui::End();


	// ImGuiの内部コマンドを生成する
	ImGui::Render();

	// ImGuiを描画する
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

}