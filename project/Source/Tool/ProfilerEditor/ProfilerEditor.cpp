#include "ProfilerEditor.h"
#include "Engine.h"

void Detail::ProfilerEditor::DrawUI()
{
#ifdef DEVELOPMENT

    // ImGuiのウィンドウを作成
    if (ImGui::Begin("プロファイルエディタ"))
    {
        // FPSの表示
        ImGui::Text("アプリケーション 平均 %.3f ms/フレーム (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    ImGui::End();

#endif
}