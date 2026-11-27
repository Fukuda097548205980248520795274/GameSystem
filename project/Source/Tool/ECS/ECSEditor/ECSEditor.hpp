#pragma once
#include "ECS/RegistryECS/RegistryECS.hpp"
#include "ECS/ComponentECS/ComponentECS.hpp"
#include "AssetsWindow/AssetsWindow.hpp"
#include "RenderContext/ImGuiRender/ImGuiRender.hpp"

namespace Detail
{
    // @brief シーンマネージャー
    class SceneManager;

	/// @brief ECSエディタ
	class ECSEditor
	{
	public:

		/// @brief コンストラクタ
		ECSEditor() { Initialize(); }

		/// @brief デストラクタ
		~ECSEditor() = default;

		/// @brief 更新
		/// @param registry 
		void Update(SceneManager* sceneManager);

		/// @brief ファイルがドロップされたときの処理
		/// @param filePath 
		void OnTextureDropped(const std::string& filePath){ assetsWindow_->OnFileDropped(filePath); }


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief ヒエラルキーの描画 
		/// @param registry 
		void DrawHierarchy();

		/// @brief インスペクターの描画
		/// @param registry 
		void DrawInspector();

		/// @brief アセットウィンドウの描画
		void DrawAssetsWindow();

		/// @brief エンティティコンポーネントUIの描画
		/// @param entity
		void DrawEntityComponents(Entity entity);

		/// @brief プレイコントロールの描画
		void DrawPlayControls(SceneManager* sceneManager);


    private:

        /// @brief エンティティにスクリプトを割り当てる
        /// @param entity 
        /// @param scriptHandle 
        void AssignScriptToEntity(Entity entity, ScriptHandle scriptHandle);

        /// @brief ヒエラルキー用の表示ラベルを生成する
        /// @param entity 
        /// @return 
        std::string GetEntityDisplayName(Entity entity) const;


        /// @brief コンポーネントのUI描画と右クリック削除メニューを共通化する
        /// @tparam T 
        /// @tparam UIFunc 
        /// @param entity 
        /// @param label 
        /// @param uiFunc 
        /// @param canRemove 
        template<typename T, typename UIFunc>
        void DrawComponent(Entity entity, const char* label, UIFunc uiFunc, bool canRemove = true)
        {
            if (!registry_->HasComponent<T>(entity)) return;

            bool isOpen = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
            bool isRemoved = false;

            // 直前のアイテム（CollapsingHeader）に対する右クリックメニュー
            if (canRemove && ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("コンポーネントを削除"))
                {
                    isRemoved = true;
                }
                ImGui::EndPopup();
            }

            // 削除されていない場合のみ中身を描画する
            if (isOpen && !isRemoved)
            {
                if (auto component = registry_->GetComponent<T>(entity))
                {
                    uiFunc(component);
                }
            }

            // 描画の後に削除を実行する
            if (isRemoved)
            {
                registry_->RemoveComponent<T>(entity);
            }
        }


	private:

		/// @brief 選択中のエンティティ
		Entity selectedEntity_ = kNullEntity;

		/// @brief レジストリ
		RegistryECS* registry_ = nullptr;

		// 状態保存用のレジストリ
		std::unique_ptr<RegistryECS> backupRegistry_ = nullptr;

		// アセットウィンドウ
		std::unique_ptr<AssetsWindow> assetsWindow_ = nullptr;
	};
}
