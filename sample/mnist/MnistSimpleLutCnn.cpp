// --------------------------------------------------------------------------
//  BinaryBrain  -- binary network evaluation platform
//   MNIST sample
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
#include "bb/MicroMlp.h"
#include "bb/BinaryLutN.h"
#include "bb/LoweringConvolution.h"
#include "bb/BatchNormalization.h"
#include "bb/ReLU.h"
#include "bb/MaxPooling.h"
#include "bb/LossSoftmaxCrossEntropy.h"
#include "bb/AccuracyCategoricalClassification.h"
#include "bb/OptimizerAdam.h"
#include "bb/OptimizerSgd.h"
#include "bb/LoadMnist.h"
#include "bb/ShuffleSet.h"
#include "bb/Utility.h"
#include "bb/Sequential.h"
#include "bb/Runner.h"
#include "bb/ExportVerilog.h"


static void WriteTestImage(std::string filename, int w, int h);

// MNIST CNN with LUT networks
void MnistSimpleLutCnn(int epoch_size, size_t mini_batch_size, bool binary_mode)
{
    // write test image
    WriteTestImage("verilog/mnist_test_160x120.ppm", 160, 120);
    WriteTestImage("verilog/mnist_test_640x480.ppm", 640, 480);

  // load MNIST data
#ifdef _DEBUG
	auto td = bb::LoadMnist<>::Load(10, 512, 128);
    std::cout << "!!! debug mode !!!" << std::endl;
#else
    auto td = bb::LoadMnist<>::Load(10);
#endif

    // create network
    auto layer_cnv0_mm0 = bb::MicroMlp<>::Create(192);
    auto layer_cnv0_mm1 = bb::MicroMlp<>::Create(32);
    auto layer_cnv1_mm0 = bb::MicroMlp<>::Create(192);
    auto layer_cnv1_mm1 = bb::MicroMlp<>::Create(32);
    auto layer_cnv2_mm0 = bb::MicroMlp<>::Create(192);
    auto layer_cnv2_mm1 = bb::MicroMlp<>::Create(32);
    auto layer_cnv3_mm0 = bb::MicroMlp<>::Create(192);
    auto layer_cnv3_mm1 = bb::MicroMlp<>::Create(32);
    auto layer_mm4 = bb::MicroMlp<>::Create(480);
    auto layer_mm5 = bb::MicroMlp<>::Create(80);

    {
        auto cnv0_sub = bb::Sequential::Create();
        cnv0_sub->Add(layer_cnv0_mm0);
        cnv0_sub->Add(layer_cnv0_mm1);

        auto cnv1_sub = bb::Sequential::Create();
        cnv1_sub->Add(layer_cnv1_mm0);
        cnv1_sub->Add(layer_cnv1_mm1);

        auto cnv2_sub = bb::Sequential::Create();
        cnv2_sub->Add(layer_cnv2_mm0);
        cnv2_sub->Add(layer_cnv2_mm1);

        auto cnv3_sub = bb::Sequential::Create();
        cnv3_sub->Add(layer_cnv3_mm0);
        cnv3_sub->Add(layer_cnv3_mm1);


        auto net = bb::Sequential::Create();
        net->Add(bb::RealToBinary<>::Create(1));
        net->Add(bb::LoweringConvolution<>::Create(cnv0_sub, 3, 3));
        net->Add(bb::LoweringConvolution<>::Create(cnv1_sub, 3, 3));
        net->Add(bb::MaxPooling<>::Create(2, 2));
        net->Add(bb::LoweringConvolution<>::Create(cnv2_sub, 3, 3));
        net->Add(bb::LoweringConvolution<>::Create(cnv3_sub, 3, 3));
        net->Add(bb::MaxPooling<>::Create(2, 2));
        net->Add(layer_mm4);
        net->Add(layer_mm5);
        net->Add(bb::BinaryToReal<>::Create({ 10 }, 1));
        net->SetInputShape({28, 28, 1});

        if ( binary_mode ) {
            std::cout << "binary mode" << std::endl;
            net->SendCommand("binary true");
        }

        // print model information
        net->PrintInfo(2);

        // run fitting
        bb::Runner<float>::create_t runner_create;
        runner_create.name      = "MnistSimpleLutCnn";
        runner_create.net       = net;
        runner_create.lossFunc  = bb::LossSoftmaxCrossEntropy<float>::Create();
        runner_create.accFunc   = bb::AccuracyCategoricalClassification<float>::Create(10);
        runner_create.optimizer = bb::OptimizerAdam<float>::Create();
        runner_create.file_read  = true;        // 前の計算結果があれば読み込んで再開
        runner_create.file_write = true;        // 計算結果をファイルに保存する
        runner_create.print_progress = true;    // 途中結果を出力
        auto runner = bb::Runner<float>::Create(runner_create);
        runner->Fitting(td, epoch_size, mini_batch_size);
    }


    {
        // LUT-network
        auto layer_cnv0_lut0 = bb::BinaryLutN<>::Create(layer_cnv0_mm0->GetOutputShape());
        auto layer_cnv0_lut1 = bb::BinaryLutN<>::Create(layer_cnv0_mm1->GetOutputShape());
        auto layer_cnv1_lut0 = bb::BinaryLutN<>::Create(layer_cnv1_mm0->GetOutputShape());
        auto layer_cnv1_lut1 = bb::BinaryLutN<>::Create(layer_cnv1_mm1->GetOutputShape());
        auto layer_cnv2_lut0 = bb::BinaryLutN<>::Create(layer_cnv2_mm0->GetOutputShape());
        auto layer_cnv2_lut1 = bb::BinaryLutN<>::Create(layer_cnv2_mm1->GetOutputShape());
        auto layer_cnv3_lut0 = bb::BinaryLutN<>::Create(layer_cnv3_mm0->GetOutputShape());
        auto layer_cnv3_lut1 = bb::BinaryLutN<>::Create(layer_cnv3_mm1->GetOutputShape());
        auto layer_lut4      = bb::BinaryLutN<>::Create(layer_mm4->GetOutputShape());
        auto layer_lut5      = bb::BinaryLutN<>::Create(layer_mm5->GetOutputShape());

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

        auto cnv0 = bb::LoweringConvolution<bb::Bit>::Create(cnv0_sub, 3, 3);
        auto cnv1 = bb::LoweringConvolution<bb::Bit>::Create(cnv1_sub, 3, 3);
        auto pol0 = bb::MaxPooling<bb::Bit>::Create(2, 2);

        auto cnv2 = bb::LoweringConvolution<bb::Bit>::Create(cnv2_sub, 3, 3);
        auto cnv3 = bb::LoweringConvolution<bb::Bit>::Create(cnv3_sub, 3, 3);
        auto pol1 = bb::MaxPooling<bb::Bit>::Create(2, 2);

        // 28x28 以外も入力できるように最終段も畳み込みに変換
        auto cnv4 = bb::LoweringConvolution<bb::Bit>::Create(cnv4_sub, 4, 4);

        auto lut_net = bb::Sequential::Create();
        lut_net->Add(bb::RealToBinary<float, bb::Bit>::Create(1));
        lut_net->Add(cnv0);
        lut_net->Add(cnv1);
        lut_net->Add(pol0);
        lut_net->Add(cnv2);
        lut_net->Add(cnv3);
        lut_net->Add(pol1);
        lut_net->Add(cnv4);
        lut_net->Add(bb::BinaryToReal<bb::Bit, float>::Create({ 10 }, 1));
        lut_net->SetInputShape({28, 28, 1});


        // テーブル化して取り込み(現状まだSetInputShape後の取り込みが必要)
        layer_cnv0_lut0->ImportLayer(*layer_cnv0_mm0);
        layer_cnv0_lut1->ImportLayer(*layer_cnv0_mm1);
        layer_cnv1_lut0->ImportLayer(*layer_cnv1_mm0);
        layer_cnv1_lut1->ImportLayer(*layer_cnv1_mm1);
        layer_cnv2_lut0->ImportLayer(*layer_cnv2_mm0);
        layer_cnv2_lut1->ImportLayer(*layer_cnv2_mm1);
        layer_cnv3_lut0->ImportLayer(*layer_cnv3_mm0);
        layer_cnv3_lut1->ImportLayer(*layer_cnv3_mm1);
        layer_lut4     ->ImportLayer(*layer_mm4);
        layer_lut5     ->ImportLayer(*layer_mm5);

        // 評価
        if ( 0 ) {
            bb::Runner<float>::create_t lut_runner_create;
            lut_runner_create.name      = "Lut_MnistSimpleLutCnn";
            lut_runner_create.net       = lut_net;
            lut_runner_create.lossFunc  = bb::LossSoftmaxCrossEntropy<float>::Create();
            lut_runner_create.accFunc   = bb::AccuracyCategoricalClassification<float>::Create(10);
            lut_runner_create.optimizer = bb::OptimizerAdam<float>::Create();
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

            std::string filename = "verilog/MnistSimpleLutCnn.v";
            std::ofstream ofs(filename);
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, "MnistSimpleLutCnnCnv0", vec_cnv0);
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, "MnistSimpleLutCnnCnv1", vec_cnv1);
            bb::ExportVerilog_LutCnnLayersAxi4s(ofs, "MnistSimpleLutCnnCnv2", vec_cnv2);
            std::cout << "export : " << filename << "\n" << std::endl;
        }
    }
}


// RTL simulation 用データの出力
static void WriteTestImage(std::string filename, const int w, const int h)
{
	// load MNIST data
	auto td = bb::LoadMnist<>::Load();

	unsigned char* img = new unsigned char[h * w];
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int idx = (y / 28) * (w / 28) + (x / 28);
			int xx = x % 28;
			int yy = y % 28;
			img[y*w+x] = (unsigned char)(td.x_test[idx][yy * 28 + xx] * 255.0f);
		}
	}

	if ( 0 ) {
		std::ofstream ofs(filename);
		ofs << "P2" << std::endl;
		ofs << w << " " << h << std::endl;
		ofs << "255" << std::endl;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				ofs << (int)img[y*w+x] << std::endl;
			}
		}
	}

	if ( 1 ) {
		std::ofstream ofs(filename);
		ofs << "P3" << std::endl;
		ofs << w << " " << h << std::endl;
		ofs << "255" << std::endl;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				ofs << (int)img[y*w+x] << " " << (int)img[y*w+x] << " " << (int)img[y*w+x] << std::endl;
			}
		}
	}

    delete[] img;
}



// end of file
