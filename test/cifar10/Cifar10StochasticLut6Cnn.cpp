// --------------------------------------------------------------------------
//  BinaryBrain  -- binary network evaluation platform
//   CIFAR-10 sample
//
//                                Copyright (C) 2018-2019 by Ryuji Fuchikami
// --------------------------------------------------------------------------


#include <iostream>
#include <fstream>
#include <numeric>
#include <random>
#include <chrono>

#include "bb/RealToBinary.h"
#include "bb/BinaryToReal.h"
#include "bb/StochasticLut6.h"
#include "bb/BinaryLutN.h"
#include "bb/LoweringConvolution.h"
#include "bb/BatchNormalization.h"
#include "bb/ReLU.h"
#include "bb/MaxPooling.h"
#include "bb/LossSoftmaxCrossEntropy.h"
#include "bb/MetricsCategoricalAccuracy.h"
#include "bb/OptimizerAdam.h"
#include "bb/OptimizerSgd.h"
#include "bb/LoadCifar10.h"
#include "bb/ShuffleSet.h"
#include "bb/Utility.h"
#include "bb/Sequential.h"
#include "bb/Runner.h"
#include "bb/ExportVerilog.h"
#include "bb/Reduce.h"



#if 0

// CNN with LUT networks
void Cifar10StochasticLut6Cnn(int epoch_size, int mini_batch_size, int max_run_size, int lut_frame_mux_size, bool binary_mode, bool file_read)
{
    std::string net_name = "Cifar10StochasticLut6Cnn";

  // load cifar-10 data
#ifdef _DEBUG
    auto td = bb::LoadCifar10<>::Load(1);
    std::cout << "!!! debug mode !!!" << std::endl;
#else
    auto td = bb::LoadCifar10<>::Load();
#endif

    // create network
    auto layer_cnv0_sl0 = bb::StochasticLut6<>::Create(192);
    auto layer_cnv0_sl1 = bb::StochasticLut6<>::Create(32);
    auto layer_cnv1_sl0 = bb::StochasticLut6<>::Create(192);
    auto layer_cnv1_sl1 = bb::StochasticLut6<>::Create(32);
    auto layer_cnv2_sl0 = bb::StochasticLut6<>::Create(192);
    auto layer_cnv2_sl1 = bb::StochasticLut6<>::Create(32);
    auto layer_cnv3_sl0 = bb::StochasticLut6<>::Create(192);
    auto layer_cnv3_sl1 = bb::StochasticLut6<>::Create(32);
    auto layer_sl4 = bb::StochasticLut6<>::Create(420);
    auto layer_sl5 = bb::StochasticLut6<>::Create(70);

    {
        auto cnv0_sub = bb::Sequential::Create();
        cnv0_sub->Add(layer_cnv0_sl0);
        cnv0_sub->Add(layer_cnv0_sl1);

        auto cnv1_sub = bb::Sequential::Create();
        cnv1_sub->Add(layer_cnv1_sl0);
        cnv1_sub->Add(layer_cnv1_sl1);

        auto cnv2_sub = bb::Sequential::Create();
        cnv2_sub->Add(layer_cnv2_sl0);
        cnv2_sub->Add(layer_cnv2_sl1);

        auto cnv3_sub = bb::Sequential::Create();
        cnv3_sub->Add(layer_cnv3_sl0);
        cnv3_sub->Add(layer_cnv3_sl1);
        
        auto net = bb::Sequential::Create();
        net->Add(bb::LoweringConvolution<>::Create(cnv0_sub, 3, 3));
        net->Add(bb::LoweringConvolution<>::Create(cnv1_sub, 3, 3));
        net->Add(bb::MaxPooling<>::Create(2, 2));
        net->Add(bb::LoweringConvolution<>::Create(cnv2_sub, 3, 3));
        net->Add(bb::LoweringConvolution<>::Create(cnv3_sub, 3, 3));
        net->Add(bb::MaxPooling<>::Create(2, 2));
        net->Add(layer_sl4);
        net->Add(layer_sl5);
        net->Add(bb::Reduce<>::Create(td.t_shape));
        net->SetInputShape(td.x_shape);

        if ( binary_mode ) {
            std::cout << "binary mode" << std::endl;
            net->SendCommand("binary true");
        }

        // print model information
        net->PrintInfo();

        // run fitting
        bb::Runner<float>::create_t runner_create;
        runner_create.name               = net_name;
        runner_create.net                = net;
        runner_create.lossFunc           = bb::LossSoftmaxCrossEntropy<float>::Create();
        runner_create.metricsFunc        = bb::MetricsCategoricalAccuracy<float>::Create();
        runner_create.optimizer          = bb::OptimizerAdam<float>::Create();
        runner_create.max_run_size       = max_run_size;    // 実際の1回の実行サイズ
        runner_create.file_read          = file_read;       // 前の計算結果があれば読み込んで再開するか
        runner_create.file_write         = true;            // 計算結果をファイルに保存するか
        runner_create.print_progress     = true;            // 途中結果を表示
        runner_create.initial_evaluation = file_read;       // ファイルを読んだ場合は最初に評価しておく
        auto runner = bb::Runner<float>::Create(runner_create);
        runner->Fitting(td, epoch_size, mini_batch_size);
    }


    {
        // LUT-network
        int const   frame_mux_size = 15;

        auto layer_cnv0_lut0 = bb::BinaryLutN<>::Create(layer_cnv0_sl0->GetOutputShape());
        auto layer_cnv0_lut1 = bb::BinaryLutN<>::Create(layer_cnv0_sl1->GetOutputShape());
        auto layer_cnv1_lut0 = bb::BinaryLutN<>::Create(layer_cnv1_sl0->GetOutputShape());
        auto layer_cnv1_lut1 = bb::BinaryLutN<>::Create(layer_cnv1_sl1->GetOutputShape());
        auto layer_cnv2_lut0 = bb::BinaryLutN<>::Create(layer_cnv2_sl0->GetOutputShape());
        auto layer_cnv2_lut1 = bb::BinaryLutN<>::Create(layer_cnv2_sl1->GetOutputShape());
        auto layer_cnv3_lut0 = bb::BinaryLutN<>::Create(layer_cnv3_sl0->GetOutputShape());
        auto layer_cnv3_lut1 = bb::BinaryLutN<>::Create(layer_cnv3_sl1->GetOutputShape());
        auto layer_lut4      = bb::BinaryLutN<>::Create(layer_sl4->GetOutputShape());
        auto layer_lut5      = bb::BinaryLutN<>::Create(layer_sl5->GetOutputShape());
//        auto layer_lut6      = bb::BinaryLutN<>::Create(layer_sl6->GetOutputShape());

        auto cnv0_sub = bb::Sequential::Create();
        cnv0_sub->Add(layer_cnv0_lut0);
        cnv0_sub->Add(layer_cnv0_lut1);

        auto cnv1_sub = bb::Sequential::Create();
        cnv1_sub->Add(layer_cnv1_lut0);
        cnv1_sub->Add(layer_cnv1_lut1);

        auto cnv2_sub = bb::Sequential::Create();
        cnv2_sub->Add(layer_cnv2_lut0);
        cnv2_sub->Add(layer_cnv2_lut1);

        auto cnv3_sub = bb::Sequential::Create();
        cnv3_sub->Add(layer_cnv3_lut0);
        cnv3_sub->Add(layer_cnv3_lut1);

        auto cnv4_sub = bb::Sequential::Create();
        cnv4_sub->Add(layer_lut4);
        cnv4_sub->Add(layer_lut5);
//        cnv4_sub->Add(layer_lut6);

        auto cnv0 = bb::LoweringConvolution<bb::Bit>::Create(cnv0_sub, 3, 3);
        auto cnv1 = bb::LoweringConvolution<bb::Bit>::Create(cnv1_sub, 3, 3);
        auto pol0 = bb::MaxPooling<bb::Bit>::Create(2, 2);

        auto cnv2 = bb::LoweringConvolution<bb::Bit>::Create(cnv2_sub, 3, 3);
        auto cnv3 = bb::LoweringConvolution<bb::Bit>::Create(cnv3_sub, 3, 3);
        auto pol1 = bb::MaxPooling<bb::Bit>::Create(2, 2);

        // 32x32 以外も入力できるように最終段も畳み込みに変換
        auto cnv4 = bb::LoweringConvolution<bb::Bit>::Create(cnv4_sub, 5, 5);

        auto lut_net = bb::Sequential::Create();
        lut_net->Add(bb::RealToBinary<float, bb::Bit>::Create(frame_mux_size));
        lut_net->Add(cnv0);
        lut_net->Add(cnv1);
        lut_net->Add(pol0);
        lut_net->Add(cnv2);
        lut_net->Add(cnv3);
        lut_net->Add(pol1);
        lut_net->Add(cnv4);
        lut_net->Add(bb::BinaryToReal<bb::Bit, float>::Create(td.t_shape, frame_mux_size));
        lut_net->SetInputShape(td.x_shape);


        // テーブル化して取り込み(現状まだSetInputShape後の取り込みが必要)
        layer_cnv0_lut0->ImportLayer<float, float>(layer_cnv0_sl0);
        layer_cnv0_lut1->ImportLayer<float, float>(layer_cnv0_sl1);
        layer_cnv1_lut0->ImportLayer<float, float>(layer_cnv1_sl0);
        layer_cnv1_lut1->ImportLayer<float, float>(layer_cnv1_sl1);
        layer_cnv2_lut0->ImportLayer<float, float>(layer_cnv2_sl0);
        layer_cnv2_lut1->ImportLayer<float, float>(layer_cnv2_sl1);
        layer_cnv3_lut0->ImportLayer<float, float>(layer_cnv3_sl0);
        layer_cnv3_lut1->ImportLayer<float, float>(layer_cnv3_sl1);
        layer_lut4     ->ImportLayer<float, float>(layer_sl4);
        layer_lut5     ->ImportLayer<float, float>(layer_sl5);
//        layer_lut6     ->ImportLayer<float, float>(layer_sl6);

        // 評価
        if ( 1 ) {
            bb::Runner<float>::create_t lut_runner_create;
            lut_runner_create.name        = "Lut_" + net_name;
            lut_runner_create.net         = lut_net;
            lut_runner_create.lossFunc    = bb::LossSoftmaxCrossEntropy<float>::Create();
            lut_runner_create.metricsFunc = bb::MetricsCategoricalAccuracy<float>::Create();
            lut_runner_create.optimizer   = bb::OptimizerAdam<float>::Create();
            lut_runner_create.initial_evaluation = false;
            lut_runner_create.print_progress = true;    // 途中結果を出力
            auto lut_runner = bb::Runner<float>::Create(lut_runner_create);
            auto lut_accuracy = lut_runner->Evaluation(td, mini_batch_size);
            std::cout << "lut_accuracy : " << lut_accuracy << std::endl;
        }

        {
            // Verilog 出力
            std::vector< std::shared_ptr< bb::Filter2d<bb::Bit> > >  vec_cnv0;
            std::vector< std::shared_ptr< bb::Filter2d<bb::Bit> > >  vec_cnv1;
            std::vector< std::shared_ptr< bb::Filter2d<bb::Bit> > >  vec_cnv2;

            vec_cnv0.push_back(cnv0);
            vec_cnv0.push_back(cnv1);
            vec_cnv0.push_back(pol0);
            vec_cnv1.push_back(cnv2);
            vec_cnv1.push_back(cnv3);
            vec_cnv1.push_back(pol1);
            vec_cnv2.push_back(cnv4);

            std::string filename = "verilog/" + net_name + ".v";
            std::ofstream ofs(filename);
            ofs << "`timescale 1ns / 1ps\n\n";
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, net_name + "Cnv0", vec_cnv0);
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, net_name + "Cnv1", vec_cnv1);
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, net_name + "Cnv2", vec_cnv2);
            std::cout << "export : " << filename << "\n" << std::endl;
            
            // write test image
            bb::WriteTestDataImage<float>("verilog/cifar10_test_160x120.ppm", 160, 120, td);
            bb::WriteTestDataImage<float>("verilog/cifar10_test_640x480.ppm", 640, 480, td);
        }
    }
}

#else

// CNN with LUT networks
void Cifar10StochasticLut6Cnn(int epoch_size, int mini_batch_size, int max_run_size, int lut_frame_mux_size, bool binary_mode, bool file_read)
{
    std::string net_name = "Cifar10StochasticLut6Cnn";

  // load cifar-10 data
#ifdef _DEBUG
    auto td = bb::LoadCifar10<>::Load(1);
    std::cout << "!!! debug mode !!!" << std::endl;
#else
    auto td = bb::LoadCifar10<>::Load();
#endif

    // create network
    auto layer_cnv0_sl0 = bb::StochasticLut6<>::Create(256);
    auto layer_cnv0_sl1 = bb::StochasticLut6<>::Create(192);
    auto layer_cnv0_sl2 = bb::StochasticLut6<>::Create(32);

    auto layer_cnv1d_sl0 = bb::StochasticLut6<>::Create({1, 8, 32},  "depthwise");
//  auto layer_cnv1d_sl1 = bb::StochasticLut6<>::Create({1, 1, 32},  "depthwise");

    auto layer_cnv1p_sl1 = bb::StochasticLut6<>::Create({1, 1, 192}, "pointwise");
    auto layer_cnv1p_sl2 = bb::StochasticLut6<>::Create({1, 1, 32},  "pointwise");

    auto layer_cnv2_sl0 = bb::StochasticLut6<>::Create({1, 16, 32}, "depthwise");
    auto layer_cnv2_sl1 = bb::StochasticLut6<>::Create(384);
    auto layer_cnv2_sl2 = bb::StochasticLut6<>::Create(64);

    auto layer_cnv3_sl0 = bb::StochasticLut6<>::Create(512);
    auto layer_cnv3_sl1 = bb::StochasticLut6<>::Create(384);
    auto layer_cnv3_sl2 = bb::StochasticLut6<>::Create(64);
#if 0
    auto layer_sl4 = bb::StochasticLut6<>::Create(2048);
    auto layer_sl5 = bb::StochasticLut6<>::Create(1024);
    auto layer_sl6 = bb::StochasticLut6<>::Create(512);
    auto layer_sl7 = bb::StochasticLut6<>::Create(360);
    auto layer_sl8 = bb::StochasticLut6<>::Create(60);
    auto layer_sl9 = bb::StochasticLut6<>::Create(10);
#else
    auto layer_sl4 = bb::StochasticLut6<>::Create(2048);
    auto layer_sl5 = bb::StochasticLut6<>::Create(1024);
    auto layer_sl6 = bb::StochasticLut6<>::Create(420);
    auto layer_sl7 = bb::StochasticLut6<>::Create(70);
#endif

    {
        auto cnv0_sub = bb::Sequential::Create();
        cnv0_sub->Add(layer_cnv0_sl0);
        cnv0_sub->Add(layer_cnv0_sl1);
        cnv0_sub->Add(layer_cnv0_sl2);

        auto cnv1d_sub = bb::Sequential::Create();
        cnv1d_sub->Add(layer_cnv1d_sl0);
 //     cnv1d_sub->Add(layer_cnv1d_sl1);

        auto cnv1p_sub = bb::Sequential::Create();
        cnv1p_sub->Add(layer_cnv1p_sl1);
        cnv1p_sub->Add(layer_cnv1p_sl2);

        auto cnv2_sub = bb::Sequential::Create();
        cnv2_sub->Add(layer_cnv2_sl0);
        auto cnv2p_sub = bb::Sequential::Create();
        cnv2p_sub->Add(layer_cnv2_sl1);
        cnv2p_sub->Add(layer_cnv2_sl2);

        auto cnv3_sub = bb::Sequential::Create();
        cnv3_sub->Add(layer_cnv3_sl0);
        cnv3_sub->Add(layer_cnv3_sl1);
        cnv3_sub->Add(layer_cnv3_sl2);
        
        auto net = bb::Sequential::Create();
        net->Add(bb::LoweringConvolution<>::Create(cnv0_sub, 3, 3));
        net->Add(bb::LoweringConvolution<>::Create(cnv1d_sub, 3, 3));
        net->Add(bb::LoweringConvolution<>::Create(cnv1p_sub, 1, 1));
        net->Add(bb::MaxPooling<>::Create(2, 2));
        net->Add(bb::LoweringConvolution<>::Create(cnv2_sub, 3, 3));
        net->Add(bb::LoweringConvolution<>::Create(cnv2p_sub, 1, 1));
        net->Add(bb::LoweringConvolution<>::Create(cnv3_sub, 3, 3));
        net->Add(bb::MaxPooling<>::Create(2, 2));
        net->Add(layer_sl4);
        net->Add(layer_sl5);
        net->Add(layer_sl6);
        net->Add(layer_sl7);
//        net->Add(layer_sl8);
//        net->Add(layer_sl9);
        net->Add(bb::Reduce<>::Create(td.t_shape));
        net->SetInputShape(td.x_shape);

        if ( binary_mode ) {
            std::cout << "binary mode" << std::endl;
            net->SendCommand("binary true");
        }

        // print model information
        net->PrintInfo();

        // run fitting
        bb::Runner<float>::create_t runner_create;
        runner_create.name               = net_name;
        runner_create.net                = net;
        runner_create.lossFunc           = bb::LossSoftmaxCrossEntropy<float>::Create();
        runner_create.metricsFunc        = bb::MetricsCategoricalAccuracy<float>::Create();
        runner_create.optimizer          = bb::OptimizerAdam<float>::Create();
        runner_create.max_run_size       = max_run_size;    // 実際の1回の実行サイズ
        runner_create.file_read          = file_read;       // 前の計算結果があれば読み込んで再開するか
        runner_create.file_write         = true;            // 計算結果をファイルに保存するか
        runner_create.print_progress     = true;            // 途中結果を表示
        runner_create.initial_evaluation = file_read;       // ファイルを読んだ場合は最初に評価しておく
        auto runner = bb::Runner<float>::Create(runner_create);
        runner->Fitting(td, epoch_size, mini_batch_size);
    }

#if 0
    {
        // LUT-network
        int const   frame_mux_size = 15;

        auto layer_cnv0_lut0 = bb::BinaryLutN<>::Create(layer_cnv0_sl0->GetOutputShape());
        auto layer_cnv0_lut1 = bb::BinaryLutN<>::Create(layer_cnv0_sl1->GetOutputShape());
        auto layer_cnv0_lut2 = bb::BinaryLutN<>::Create(layer_cnv0_sl2->GetOutputShape());
        auto layer_cnv1_lut0 = bb::BinaryLutN<>::Create(layer_cnv1_sl0->GetOutputShape());
        auto layer_cnv1_lut1 = bb::BinaryLutN<>::Create(layer_cnv1_sl1->GetOutputShape());
        auto layer_cnv1_lut2 = bb::BinaryLutN<>::Create(layer_cnv1_sl2->GetOutputShape());
        auto layer_cnv2_lut0 = bb::BinaryLutN<>::Create(layer_cnv2_sl0->GetOutputShape());
        auto layer_cnv2_lut1 = bb::BinaryLutN<>::Create(layer_cnv2_sl1->GetOutputShape());
        auto layer_cnv2_lut2 = bb::BinaryLutN<>::Create(layer_cnv2_sl2->GetOutputShape());
        auto layer_cnv3_lut0 = bb::BinaryLutN<>::Create(layer_cnv3_sl0->GetOutputShape());
        auto layer_cnv3_lut1 = bb::BinaryLutN<>::Create(layer_cnv3_sl1->GetOutputShape());
        auto layer_cnv3_lut2 = bb::BinaryLutN<>::Create(layer_cnv3_sl2->GetOutputShape());
        auto layer_lut4      = bb::BinaryLutN<>::Create(layer_sl4->GetOutputShape());
        auto layer_lut5      = bb::BinaryLutN<>::Create(layer_sl5->GetOutputShape());
        auto layer_lut6      = bb::BinaryLutN<>::Create(layer_sl6->GetOutputShape());
        auto layer_lut7      = bb::BinaryLutN<>::Create(layer_sl7->GetOutputShape());
//        auto layer_lut8      = bb::BinaryLutN<>::Create(layer_sl8->GetOutputShape());
//        auto layer_lut9      = bb::BinaryLutN<>::Create(layer_sl9->GetOutputShape());

        auto cnv0_sub = bb::Sequential::Create();
        cnv0_sub->Add(layer_cnv0_lut0);
        cnv0_sub->Add(layer_cnv0_lut1);
        cnv0_sub->Add(layer_cnv0_lut2);

        auto cnv1_sub = bb::Sequential::Create();
        cnv1_sub->Add(layer_cnv1_lut0);
        cnv1_sub->Add(layer_cnv1_lut1);
        cnv1_sub->Add(layer_cnv1_lut2);

        auto cnv2_sub = bb::Sequential::Create();
        cnv2_sub->Add(layer_cnv2_lut0);
        cnv2_sub->Add(layer_cnv2_lut1);
        cnv2_sub->Add(layer_cnv2_lut2);

        auto cnv3_sub = bb::Sequential::Create();
        cnv3_sub->Add(layer_cnv3_lut0);
        cnv3_sub->Add(layer_cnv3_lut1);
        cnv3_sub->Add(layer_cnv3_lut2);

        auto cnv4_sub = bb::Sequential::Create();
        cnv4_sub->Add(layer_lut4);
        cnv4_sub->Add(layer_lut5);
        cnv4_sub->Add(layer_lut6);
        cnv4_sub->Add(layer_lut7);
//        cnv4_sub->Add(layer_lut8);
//        cnv4_sub->Add(layer_lut9);

        auto cnv0 = bb::LoweringConvolution<bb::Bit>::Create(cnv0_sub, 3, 3);
        auto cnv1 = bb::LoweringConvolution<bb::Bit>::Create(cnv1_sub, 3, 3);
        auto pol0 = bb::MaxPooling<bb::Bit>::Create(2, 2);

        auto cnv2 = bb::LoweringConvolution<bb::Bit>::Create(cnv2_sub, 3, 3);
        auto cnv3 = bb::LoweringConvolution<bb::Bit>::Create(cnv3_sub, 3, 3);
        auto pol1 = bb::MaxPooling<bb::Bit>::Create(2, 2);

        // 32x32 以外も入力できるように最終段も畳み込みに変換
        auto cnv4 = bb::LoweringConvolution<bb::Bit>::Create(cnv4_sub, 5, 5);

        auto lut_net = bb::Sequential::Create();
        lut_net->Add(bb::RealToBinary<float, bb::Bit>::Create(lut_frame_mux_size));
        lut_net->Add(cnv0);
        lut_net->Add(cnv1);
        lut_net->Add(pol0);
        lut_net->Add(cnv2);
        lut_net->Add(cnv3);
        lut_net->Add(pol1);
        lut_net->Add(cnv4);
        lut_net->Add(bb::BinaryToReal<bb::Bit, float>::Create(td.t_shape, lut_frame_mux_size));
        lut_net->SetInputShape(td.x_shape);


        // テーブル化して取り込み(現状まだSetInputShape後の取り込みが必要)
        std::cout << "parameter copy to LUT-Network" << std::endl;
        layer_cnv0_lut0->ImportLayer<float, float>(layer_cnv0_sl0);
        layer_cnv0_lut1->ImportLayer<float, float>(layer_cnv0_sl1);
        layer_cnv0_lut2->ImportLayer<float, float>(layer_cnv0_sl2);
        layer_cnv1_lut0->ImportLayer<float, float>(layer_cnv1_sl0);
        layer_cnv1_lut1->ImportLayer<float, float>(layer_cnv1_sl1);
        layer_cnv1_lut2->ImportLayer<float, float>(layer_cnv1_sl2);
        layer_cnv2_lut0->ImportLayer<float, float>(layer_cnv2_sl0);
        layer_cnv2_lut1->ImportLayer<float, float>(layer_cnv2_sl1);
        layer_cnv2_lut2->ImportLayer<float, float>(layer_cnv2_sl2);
        layer_cnv3_lut0->ImportLayer<float, float>(layer_cnv3_sl0);
        layer_cnv3_lut1->ImportLayer<float, float>(layer_cnv3_sl1);
        layer_cnv3_lut2->ImportLayer<float, float>(layer_cnv3_sl2);
        layer_lut4     ->ImportLayer<float, float>(layer_sl4);
        layer_lut5     ->ImportLayer<float, float>(layer_sl5);
        layer_lut6     ->ImportLayer<float, float>(layer_sl6);
        layer_lut7     ->ImportLayer<float, float>(layer_sl7);
//        layer_lut8     ->ImportLayer<float, float>(layer_sl8);
//        layer_lut9     ->ImportLayer<float, float>(layer_sl9);

        // 評価
        if ( 1 ) {
            bb::Runner<float>::create_t lut_runner_create;
            lut_runner_create.name        = "Lut_" + net_name;
            lut_runner_create.net         = lut_net;
            lut_runner_create.lossFunc    = bb::LossSoftmaxCrossEntropy<float>::Create();
            lut_runner_create.metricsFunc = bb::MetricsCategoricalAccuracy<float>::Create();
            lut_runner_create.optimizer   = bb::OptimizerAdam<float>::Create();
            lut_runner_create.initial_evaluation = false;
            lut_runner_create.print_progress = true;    // 途中結果を出力
            auto lut_runner = bb::Runner<float>::Create(lut_runner_create);
            auto lut_accuracy = lut_runner->Evaluation(td, mini_batch_size);
            std::cout << "lut_accuracy : " << lut_accuracy << std::endl;
        }

        {
            // Verilog 出力
            std::vector< std::shared_ptr< bb::Filter2d<bb::Bit> > >  vec_cnv0;
            std::vector< std::shared_ptr< bb::Filter2d<bb::Bit> > >  vec_cnv1;
            std::vector< std::shared_ptr< bb::Filter2d<bb::Bit> > >  vec_cnv2;

            vec_cnv0.push_back(cnv0);
            vec_cnv0.push_back(cnv1);
            vec_cnv0.push_back(pol0);
            vec_cnv1.push_back(cnv2);
            vec_cnv1.push_back(cnv3);
            vec_cnv1.push_back(pol1);
            vec_cnv2.push_back(cnv4);

            std::string filename = "verilog/" + net_name + ".v";
            std::ofstream ofs(filename);
            ofs << "`timescale 1ns / 1ps\n\n";
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, net_name + "Cnv0", vec_cnv0);
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, net_name + "Cnv1", vec_cnv1);
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, net_name + "Cnv2", vec_cnv2);
            std::cout << "export : " << filename << "\n" << std::endl;
            
            // write test image
            bb::WriteTestDataImage<float>("verilog/cifar10_test_160x120.ppm", 160, 120, td);
            bb::WriteTestDataImage<float>("verilog/cifar10_test_640x480.ppm", 640, 480, td);
        }
    }
#endif
}

#endif



// end of file
