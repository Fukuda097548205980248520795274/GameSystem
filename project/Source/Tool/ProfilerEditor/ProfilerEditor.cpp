#include "ProfilerEditor.hpp"
#include "Engine.hpp"

//// @brief UIを描画する
void Detail::ProfilerEditor::DrawUI()
{
#ifdef DEVELOPMENT

    // ImGuiのウィンドウを作成
    if (ImGui::Begin("プロファイルエディタ"))
    {
        // 基本フレームレート表示
        float fps = ImGui::GetIO().Framerate;
        float frameTime = 1000.0f / (fps > 0.0f ? fps : 60.0f);
        ImGui::Text("FPS : %.1f (Total: %.3f ms)", fps, frameTime);
        ImGui::Separator();

		auto renderContext = Engine::GetInstance()->GetRenderContext();
		float cpuTimeMs = renderContext->GetCpuTimeMs();
		float gpuTimeMs = renderContext->GetGpuTimeMs();

        // CPU / GPU 時間のテキスト表示
        ImGui::Text("CPU Time : %.3f ms", cpuTimeMs);
        ImGui::Text("GPU Time : %.3f ms", gpuTimeMs);

        // 履歴リングバッファの更新
        static float cpuHistory[120] = { 0 };
        static float gpuHistory[120] = { 0 };
        static int offset = 0;

        cpuHistory[offset] = cpuTimeMs;
        gpuHistory[offset] = gpuTimeMs;
        offset = (offset + 1) % 120;

        // グラフ表示
        ImGui::PlotLines("CPU (ms)", cpuHistory, 120, offset, nullptr, 0.0f, 33.3f, ImVec2(0, 50));
        ImGui::PlotLines("GPU (ms)", gpuHistory, 120, offset, nullptr, 0.0f, 33.3f, ImVec2(0, 50));
    }

    ImGui::End();

#endif
}