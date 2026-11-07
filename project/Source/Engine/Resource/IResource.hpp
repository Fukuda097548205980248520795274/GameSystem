#pragma once
#include "Store/ResourceStore/BufferAllocator/BufferAllocator.hpp"

namespace Detail
{
    class IResource
    {
    public:

        /// @brief コンストラクタ
        IResource() = default;

        /// @brief デストラクタ
        virtual ~IResource() = default;
    };
}
