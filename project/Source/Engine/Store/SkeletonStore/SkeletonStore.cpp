#include "SkeletonStore.hpp"

#include "Engine.hpp"
#include "Func/SkeletonFunc/SkeletonFunc.hpp"

/// @brief 更新処理
/// @param maxBufferCount 
/// @param frameIndex 
void Detail::SkeletonStore::Update(uint32_t maxBufferCount, uint32_t frameIndex)
{
	// 破棄を遅延処理する
	ProcessDeferredDeletions(maxBufferCount, frameIndex);
}

/// @brief 読み込み
/// @param directory 
/// @param fileName 
/// @param device 
/// @return 
Detail::SkeletonHandle Detail::SkeletonStore::Load(const std::string& filePath)
{
	// キャッシュチェック
	auto it = cache_.find(filePath);
	if (it != cache_.end())
	{
		// 既存のハンドルが有効か確認
		if (GetSkeletonData(it->second) != nullptr)
		{
			return it->second;
		}
	}

	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();
	if (!engine) return Detail::SkeletonHandle::Invalid();

	// モデルストアを取得
	auto  modelStore = engine->GetRenderContext()->GetModelStore();
	if (!modelStore)return Detail::SkeletonHandle::Invalid();


	// モデルをロードして、モデルハンドルを取得する
	ModelHandle modelHandle = engine->LoadModel(filePath);
	if (!modelHandle.IsValid())return Detail::SkeletonHandle::Invalid();

	Detail::SkeletonData skeletonData;

	// モデルデータを取得する
	auto modelData = modelStore->GetModelData(modelHandle);
	if (!modelData) return Detail::SkeletonHandle::Invalid();

	// スケルトンを作成する
	skeletonData.skeleton = Detail::CreateSkeleton(modelData->nodes);
	if (skeletonData.skeleton.joints.empty())
		return Detail::SkeletonHandle::Invalid();

	// ボーンを読み込む
	skeletonData.boneData = Detail::LoadBone(filePath, skeletonData.skeleton);
	if (skeletonData.boneData.meshes.empty() && skeletonData.boneData.meshIndices.empty())
		return Detail::SkeletonHandle::Invalid();

	// ファイルパスを保存
	skeletonData.filePath = filePath;


	// スロットの割り当て
	uint32_t slotIndex = 0;
	if (!freeSlots_.empty())
	{
		// 空いているスロットを再利用
		slotIndex = freeSlots_.front();
		freeSlots_.pop();
	}
	else
	{
		// 新規スロット作成
		slotIndex = static_cast<uint32_t>(slots_.size());
		slots_.emplace_back();
	}

	// スロットにスケルトンデータを格納
	Slot& slot = slots_[slotIndex];
	slot.data = std::make_unique<SkeletonData>(std::move(skeletonData));
	slot.inUse = true;

	// キャッシュに登録
	SkeletonHandle handle{ slotIndex, slot.generation };
	cache_[filePath] = handle;

	return handle;
}

/// @brief アニメーションの破棄
/// @param handle 
/// @param frameIndex 
void Detail::SkeletonStore::Unload(SkeletonHandle handle, uint32_t frameIndex)
{
	if (!handle.IsValid() || handle.index >= slots_.size()) return;

	Slot& slot = slots_[handle.index];

	if (slot.inUse && slot.generation == handle.generation)
	{
		if (slot.data)
		{
			cache_.erase(slot.data->filePath);

			// 即座に破棄せず、所有権を破棄キューに移動する
			deferredDeletionQueue_.push({ std::move(slot.data), frameIndex });
		}

		// スロットは空いたものとして即座に再利用可能にする
		slot.inUse = false;
		slot.generation++;
		freeSlots_.push(handle.index);
	}
}

/// @brief アニメーションデータを取得する
/// @param handle 
/// @return 
Detail::SkeletonData* Detail::SkeletonStore::GetSkeletonData(SkeletonHandle handle)
{
	// ハンドルが無効、またはインデックスが範囲外の場合はnullptrを返す
	if (!handle.IsValid() || handle.index >= slots_.size())
		return nullptr;

	// スロットを取得する
	Slot& slot = slots_[handle.index];

	// スロットが使用中でない、または世代番号が一致しない場合は無効なハンドルとみなす
	if (!slot.inUse || slot.generation != handle.generation)
		return nullptr;

	return slot.data.get();
}

/// @brief インデックスでハンドルを取得する
/// @param index 
/// @return 
Detail::SkeletonHandle Detail::SkeletonStore::GetHandleByIndex(uint32_t index) const
{
	// インデックスが範囲外の場合は無効なハンドルを返す
	if (index >= slots_.size())
		return SkeletonHandle{};

	// スロットを取得する
	const Slot& slot = slots_[index];

	// スロットが使用中でない場合は無効なハンドルを返す
	if (!slot.inUse)
		return SkeletonHandle{};

	return SkeletonHandle{ index, slot.generation };
}

/// @brief 破棄を遅延処理する
/// @param maxBufferCount 
/// @param frameIndex 
void Detail::SkeletonStore::ProcessDeferredDeletions(uint32_t maxBufferCount, uint32_t frameIndex)
{
	while (!deferredDeletionQueue_.empty())
	{
		// キュー先頭の破棄要求フレームを取得
		int32_t requestedFrame = deferredDeletionQueue_.front().requestedFrameIndex;

		// 破棄要求からGPUが十分に処理を進めたか判定
		if (frameIndex - requestedFrame >= maxBufferCount)
		{
			// キューからポップすることで、std::unique_ptrが破棄されリソースが安全に解放される
			deferredDeletionQueue_.pop();
		}
		else
		{
			// 先頭要素がまだ破棄できないなら、後続の要素もまだ破棄できないのでループを抜ける
			break;
		}
	}
}