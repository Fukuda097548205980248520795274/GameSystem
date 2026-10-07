#include "RenderPassSystem.h"
#include <cassert>
#include <algorithm>

#include "Engine.h"
#include "RenderContext/RenderSystem/RenderSystem.h"

/// @brief 初期化
/// @param renderTargetPool 
void Detail::RenderPassSystem::Initialize(RenderTargetPool* renderTargetPool)
{
	// Engineのインスタンスを取得
	auto engine = Engine::GetInstance();
	auto registry = engine->GetRegistryECS();

	// nullptrチェック
	assert(registry);
	assert(renderTargetPool);

	// メンバ変数に代入する
	registry_ = registry;
	renderTargetPool_ = renderTargetPool;
}

/// @brief レンダーパスを作成する
/// @param priority 
/// @param blendMode 
/// @param drawFunc 
/// @return 
Entity Detail::RenderPassSystem::CreatePass(int priority, BlendMode blendMode, std::function<void()> drawFunc)
{
	Entity entity = registry_->CreateEntity();

	// RenderPassComponentを追加する
	RenderPassComponent passComp;
	passComp.priority = priority;
	registry_->AddComponent(entity, passComp);

	// BlendComponentを追加する
	BlendComponent blendComp;
	blendComp.blendMode = blendMode;
	registry_->AddComponent(entity, blendComp);

	return entity;
}

/// @brief すべてのレンダーパスを実行する
/// @param commandList 
/// @param multiPass 
/// @param dsvHandle 
/// @param renderSystem 
void Detail::RenderPassSystem::ExecuteAll(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, MultiPass* multiPass, RenderSystem* renderSystem)
{
    auto* passArray = registry_->GetComponentArray<RenderPassComponent>();
    const auto& entities = passArray->GetEntities();
	int32_t frameIndex = Engine::GetInstance()->GetFrameIndex();

	// priority に基づいてソートするためのペアのベクターを作成
    std::vector<std::pair<Entity, int>> sortedPasses;
    for (Entity entity : entities)
    {
		// RenderPassComponent を取得して、isEnabled が true の場合のみ追加
        auto& passComp = registry_->GetComponent<RenderPassComponent>(entity);
        if (passComp.isEnabled)
        {
            sortedPasses.push_back({ entity, passComp.priority });
        }
    }

    // priority に基づいて昇順でソート（数値が小さいほど先に描画）
    std::sort(sortedPasses.begin(), sortedPasses.end(), [](const auto& a, const auto& b) {return a.second < b.second;});

    // 順次実行
    for (const auto& pair : sortedPasses)
    {
        Entity entity = pair.first;
        auto& blendComp = registry_->GetComponent<BlendComponent>(entity);

        // オフスクリーンリソースの貸出
        OffscreenResource* destinationResource = renderTargetPool_->Rent(commandList);
        if (!destinationResource) continue;

        // 返却処理のためにアクティブリソースリストに追加しておく
        activeResources_.push_back(destinationResource);

        // リソースバリアの設定（シェーダーリソース状態からレンダーターゲット状態へ遷移）
        destinationResource->Barrier(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, frameIndex);

        // レンダーターゲットのクリアと設定
        destinationResource->ClearRenderTarget(commandList, dsvHandle, frameIndex);

		// 描画処理の実行
		renderSystem->ExecuteAll(commandList, pair.second);

        // リソースバリアの設定（レンダーターゲット状態からシェーダーリソース状態へ遷移）
        destinationResource->Barrier(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, frameIndex);

        // MultiPass クラスへ今回の描画結果リソースを渡す
        multiPass->SetCurrentResource(destinationResource);
    }
}

/// @brief レンダーパスを返却する
void Detail::RenderPassSystem::Return()
{
    for (auto& renderPass : activeResources_)
    {
        renderTargetPool_->Return(renderPass);
    }

    activeResources_.clear();
}