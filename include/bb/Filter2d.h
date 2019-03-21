﻿// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                     Copyright (C) 2018 by Ryuji Fuchikami
//                                     https://github.com/ryuz
//                                     ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------



#pragma once


#include "bb/Model.h"


namespace bb {

// 二次元フィルタの基本クラス
template <typename FT = float, typename BT = float>
class Filter2d : public Model
{
public:
    virtual index_t GetFilterHeight(void) = 0;
    virtual index_t GetFilterWidth(void)  = 0;

    index_t GetInputChannels(void)
    {
        auto shape = this->GetInputShape();
        BB_ASSERT(shape.size() == 3);
        return shape[2];
    }

    index_t GetInputHeight(void)
    {
        auto shape = this->GetInputShape();
        BB_ASSERT(shape.size() == 3);
        return shape[1];
    }

    index_t GetInputWidth(void)
    {
        auto shape = this->GetInputShape();
        BB_ASSERT(shape.size() == 3);
        return shape[0];
    }

    index_t GetOutputChannels(void)
    {
        auto shape = this->GetOutputShape();
        BB_ASSERT(shape.size() == 3);
        return shape[2];
    }

    index_t GetOutputHeight(void)
    {
        auto shape = this->GetOutputShape();
        BB_ASSERT(shape.size() == 3);
        return shape[1];
    }

    index_t GetOutputWidth(void)
    {
        auto shape = this->GetOutputShape();
        BB_ASSERT(shape.size() == 3);
        return shape[0];
    }
};


}