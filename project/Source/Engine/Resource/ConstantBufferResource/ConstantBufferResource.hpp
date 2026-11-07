#pragma once
#include "IConstantBufferResource.hpp"

namespace Detail
{

	/// @brief 定数バッファリソース
	/// @tparam T 
	template<typename T>
	class ConstantBufferResource : public IConstantBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		ConstantBufferResource(BufferAllocator* allocator, uint32_t maxBufferCount = 1) { Initialize(allocator, maxBufferCount); }

        /// @brief デストラクタ
        ~ConstantBufferResource() override;

        /// @brief データを書き込むためのCPUポインタを取得する
        /// @param frameIndex 
        T* GetData(uint32_t frameIndex){return static_cast<T*>(regions_[frameIndex].cpuAddress);}

        /// @brief 型情報を取得する
        /// @return 
        std::type_index GetTypeIndex() const override{return std::type_index(typeid(ConstantBufferResource<T>));}


	private:

		/// @brief 初期化
		/// @param device 
		void Initialize(BufferAllocator* allocator, uint32_t maxBufferCount);
	};
}

/// @brief 初期化
/// @param device 
/// @param log 
template<typename T>
void Detail::ConstantBufferResource<T>::Initialize(BufferAllocator* allocator, uint32_t maxBufferCount)
{
	// nullptrチェック
	assert(allocator);

    // バッファアロケータと最大バッファ数を保存
    allocator_ = allocator;
    maxBufferCount_ = maxBufferCount > 0 ? maxBufferCount : 1;

    regions_.resize(maxBufferCount_);

    // 必要なバイト数を計算（256バイトアライメント）
    uint64_t size = (sizeof(T) + 255) & ~255;

    // フレームバッファの数だけ領域をサブアロケーションで確保
    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        regions_[i] = allocator_->Allocate(size, 256);
    }
}

/// @brief デストラクタ
/// @tparam T 
template<typename T>
Detail::ConstantBufferResource<T>::~ConstantBufferResource()
{
    if (allocator_)
    {
        for (auto& region : regions_)
        {
            allocator_->Free(region);
        }
    }
}
