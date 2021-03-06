﻿
#include <stdio.h>
#include <iostream>

#include "gtest/gtest.h"
#include "bb/DepthwiseDenseAffine.h"


TEST(DepthwiseDenseAffineTest, testAffine)
{
    auto affine = bb::DepthwiseDenseAffine<>::Create(3);
    
    affine->SetInputShape({2, 3});

    // forward
    bb::FrameBuffer x_buf(1, {2, 3}, BB_TYPE_FP32);

    x_buf.SetFP32(0, 0, 1);
    x_buf.SetFP32(0, 1, 2);
    x_buf.SetFP32(0, 2, 3);
    x_buf.SetFP32(0, 3, 4);
    x_buf.SetFP32(0, 4, 5);
    x_buf.SetFP32(0, 5, 6);
    
    EXPECT_EQ(1, x_buf.GetFP32(0, 0));
    EXPECT_EQ(2, x_buf.GetFP32(0, 1));
    EXPECT_EQ(3, x_buf.GetFP32(0, 2));
    EXPECT_EQ(4, x_buf.GetFP32(0, 3));
    EXPECT_EQ(5, x_buf.GetFP32(0, 4));
    EXPECT_EQ(6, x_buf.GetFP32(0, 5));

    {
        auto W = affine->lock_W();
        auto b = affine->lock_b();
        W(0, 0) = 1;
        W(0, 1) = 2;
        W(1, 0) = 10;
        W(1, 1) = 20;
        W(2, 0) = 100;
        W(2, 1) = 200;
        b(0) = 1000;
        b(1) = 2000;
        b(2) = 3000;
    }

    auto y_buf = affine->Forward(x_buf);

    EXPECT_EQ(1 *   1 + 2 *   2 + 1000, y_buf.GetFP32(0, 0));
    EXPECT_EQ(3 *  10 + 4 *  20 + 2000, y_buf.GetFP32(0, 1));
    EXPECT_EQ(5 * 100 + 6 * 200 + 3000, y_buf.GetFP32(0, 2));

    
    // backward
    bb::FrameBuffer dy_buf(1, {3}, BB_TYPE_FP32);

    dy_buf.SetFP32(0, 0, 123);
    dy_buf.SetFP32(0, 1, 456);
    dy_buf.SetFP32(0, 2, 789);

    auto dx_buf = affine->Backward(dy_buf);

    EXPECT_EQ(123 * 1, dx_buf.GetFP32(0, 0));
    EXPECT_EQ(123 * 2, dx_buf.GetFP32(0, 1));
    EXPECT_EQ(456 * 10, dx_buf.GetFP32(0, 2));
    EXPECT_EQ(456 * 20, dx_buf.GetFP32(0, 3));
    EXPECT_EQ(789 * 100, dx_buf.GetFP32(0, 4));
    EXPECT_EQ(789 * 200, dx_buf.GetFP32(0, 5));

    {
        auto db = affine->lock_db_const();

        EXPECT_EQ(123, db(0));
        EXPECT_EQ(456, db(1));
        EXPECT_EQ(789, db(2));
    }

    {
        auto dW = affine->lock_dW_const();

        EXPECT_EQ(1 * 123, dW(0, 0));
        EXPECT_EQ(2 * 123, dW(0, 1));
        EXPECT_EQ(3 * 456, dW(1, 0));
        EXPECT_EQ(4 * 456, dW(1, 1));
        EXPECT_EQ(5 * 789, dW(2, 0));
        EXPECT_EQ(6 * 789, dW(2, 1));
    }
}

#if 0

#ifdef BB_WITH_CUDA
TEST(DenseAffineTest, testAffine_cudaBlas1)
{
    cublasHandle_t handle;
    BB_CUBLAS_SAFE_CALL(cublasCreate(&handle));

    bb::FrameBuffer a(2, {3}, BB_TYPE_FP32);
    bb::FrameBuffer b(3, {2}, BB_TYPE_FP32);
    bb::FrameBuffer c(2, {2}, BB_TYPE_FP32);

    a.SetFP32(0, 0, 1);
    a.SetFP32(0, 1, 2);
    a.SetFP32(0, 2, 3);
    a.SetFP32(1, 0, 4);
    a.SetFP32(1, 1, 5);
    a.SetFP32(1, 2, 6);

    b.SetFP32(0, 0, 7);
    b.SetFP32(0, 1, 8);
    b.SetFP32(1, 0, 9);
    b.SetFP32(1, 1, 10);
    b.SetFP32(2, 0, 11);
    b.SetFP32(2, 1, 12);

    {
        auto a_ptr = a.LockDeviceMemoryConst();
        auto b_ptr = b.LockDeviceMemoryConst();
        auto c_ptr = c.LockDeviceMemory(true);

        float alpha = 1.0f;
        float beta = 0.0f;
        BB_CUBLAS_SAFE_CALL(cublasSgemm
            (
                handle,
                CUBLAS_OP_N,
                CUBLAS_OP_N,
                (int)a.GetFrameSize(),   // = c.GetFrameSize() = 2
                (int)b.GetNodeSize(),    // = c.GetNodeSize()  = 2
                (int)a.GetNodeSize(),    // = b.GetFrameSize() = 3
                &alpha,
                (const float *)a_ptr.GetAddr(),
                (int)(a.GetFrameStride() / sizeof(float)),
                (const float *)b_ptr.GetAddr(),
                (int)(b.GetFrameStride() / sizeof(float)),
                &beta,
                (float *)c_ptr.GetAddr(),
                (int)(c.GetFrameStride() / sizeof(float))
            ));
    }

    cublasDestroy(handle);

    EXPECT_FLOAT_EQ(1*7 + 2*9  + 3*11, c.GetFP32(0, 0));
    EXPECT_FLOAT_EQ(1*8 + 2*10 + 3*12, c.GetFP32(0, 1));
    EXPECT_FLOAT_EQ(4*7 + 5*9  + 6*11, c.GetFP32(1, 0));
    EXPECT_FLOAT_EQ(4*8 + 5*10 + 6*12, c.GetFP32(1, 1));
    
#if 0
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            std::cout << i << " " << j << " : " << c.GetFP32(i, j) << std::endl;
        }
    }
#endif
}


TEST(DenseAffineTest, testAffine_cudaBlas2)
{
    cublasHandle_t handle;
    BB_CUBLAS_SAFE_CALL(cublasCreate(&handle));

    bb::FrameBuffer a(2, {3}, BB_TYPE_FP32);
    bb::Tensor      b({3, 2}, BB_TYPE_FP32);
    bb::FrameBuffer c(2, {2}, BB_TYPE_FP32);

    a.SetFP32(0, 0, 1);
    a.SetFP32(0, 1, 2);
    a.SetFP32(0, 2, 3);
    a.SetFP32(1, 0, 4);
    a.SetFP32(1, 1, 5);
    a.SetFP32(1, 2, 6);

    {
        auto b_ptr = b.Lock<float>();
        b_ptr(0, 0) = 7;
        b_ptr(1, 0) = 8;
        b_ptr(0, 1) = 9;
        b_ptr(1, 1) = 10;
        b_ptr(0, 2) = 11;
        b_ptr(1, 2) = 12;
    }

    c.SetFP32(0, 0, 21);
    c.SetFP32(0, 1, 22);
    c.SetFP32(1, 0, 23);
    c.SetFP32(1, 1, 24);

    {
        auto a_ptr = a.LockDeviceMemoryConst();
        auto b_ptr = b.LockDeviceMemoryConst();
        auto c_ptr = c.LockDeviceMemory();

        float alpha = 1.0f;
        float beta = 1.0f;
        BB_CUBLAS_SAFE_CALL(cublasSgemm
            (
                handle,
                CUBLAS_OP_N,
                CUBLAS_OP_N,
                (int)c.GetFrameSize(),   // = a.GetFrameSize() = 2
                (int)c.GetNodeSize(),    // = 2
                (int)a.GetNodeSize(),    // = 3
                &alpha,
                (const float *)a_ptr.GetAddr(),
                (int)(a.GetFrameStride() / sizeof(float)),
                (const float *)b_ptr.GetAddr(),
                3,
                &beta,
                (float *)c_ptr.GetAddr(),
                (int)(c.GetFrameStride() / sizeof(float))
            ));
    }

    cublasDestroy(handle);

    EXPECT_FLOAT_EQ(21 + 1*7 + 2*9  + 3*11, c.GetFP32(0, 0));
    EXPECT_FLOAT_EQ(22 + 1*8 + 2*10 + 3*12, c.GetFP32(0, 1));
    EXPECT_FLOAT_EQ(23 + 4*7 + 5*9  + 6*11, c.GetFP32(1, 0));
    EXPECT_FLOAT_EQ(24 + 4*8 + 5*10 + 6*12, c.GetFP32(1, 1));
    
#if 0
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            std::cout << i << " " << j << " : " << c.GetFP32(i, j) << std::endl;
        }
    }
#endif
}

#endif

#endif

