﻿// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                     Copyright (C) 2018 by Ryuji Fuchikami
//                                     https://github.com/ryuz
//                                     ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------


#pragma once

#include <vector>
#include <iostream>
#include <fstream>

#if BB_WITH_CEREAL
#include "cereal/types/array.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"
#endif

#include "bb/FrameBuffer.h"
#include "bb/Variables.h"


namespace bb {


//! model class
class Model
{
protected:
	std::string		m_name;

    /**
     * @brief  コマンドを処理
     * @detail レイヤーの動作をカスタマイズ
     *         そのうち特定のレイヤだけとか、活性化だけとか選べるのも作るかも
     *         文字列にしておけば何でも出来るかな？
     */
    virtual void CommandProc(std::vector<std::string> args) {}
    
public:
    /**
     * @brief  デストラクタ(仮想関数)
     * @detail デストラクタ(仮想関数)
     */
	virtual ~Model() {}

    /**
     * @brief  クラス名取得
     * @detail クラス名取得
     *         シリアライズ時などの便宜上、クラス名を返すようにする
     * @return クラス名
     */
	virtual std::string GetClassName(void) const = 0;

    /**
     * @brief  名前設定
     * @detail 名前設定
     *         インスタンスの名前を設定する
     */
	virtual void  SetName(const std::string name) {
		m_name = name;
	}
    
    /**
     * @brief  名前取得
     * @detail 名前取得
     *         インスタンスの名前を取得する
     * @return 名前を返す
     */
	virtual std::string GetName(void) const
	{
		if (m_name.empty()) {
			return GetClassName();
		}
		return m_name;
	}
	
    /**
     * @brief  コマンドを送信
     * @detail コマンドを送信
     *         そのうち特定のレイヤだけとか、活性化だけとか選べるのも作るかも
     */
    virtual void SendCommand(std::string command, std::string send_to = "all")
    {
        if ( send_to == "all" || send_to == GetClassName() || send_to == GetName() ) {
            CommandProc(SplitString(command));
        }
    }


    /**
     * @brief  パラメータ取得
     * @detail パラメータを取得する
     *         Optimizerでの利用を想定
     * @return パラメータを返す
     */
    virtual Variables GetParameters(void) { return Variables(); }

    /**
     * @brief  勾配取得
     * @detail 勾配を取得する
     *         Optimizerでの利用を想定
     * @return パラメータを返す
     */
    virtual Variables GetGradients(void) { return Variables(); }
    

    /**
     * @brief  入力形状設定
     * @detail 入力形状を設定する
     *         同一形状を指定しても内部変数は初期化されるものとする
     * @param  shape      1フレームのノードを構成するshape
     * @return 出力形状を返す
     */
    virtual indices_t SetInputShape(indices_t shape) { return shape; }


    /**
     * @brief  入力形状取得
     * @detail 入力形状を取得する
     * @return 入力形状を返す
     */
    virtual indices_t GetInputShape(void) const = 0;

    /**
     * @brief  入力ノード数取得
     * @detail 入力ノード数取得
     * @return 入力ノード数を返す
     */
    index_t GetInputNodeSize(void) const
    {
        return GetShapeSize(GetInputShape());
    }

    /**
     * @brief  出力形状取得
     * @detail 出力形状を取得する
     * @return 出力形状を返す
     */
    virtual indices_t GetOutputShape(void) const = 0;
    
    /**
     * @brief  出力ノード数取得
     * @detail 出力ノード数取得
     * @return 出力ノード数を返す
     */
    index_t GetOutputNodeSize(void) const
    {
        return GetShapeSize(GetOutputShape());
    }


   /**
     * @brief  forward演算
     * @detail forward演算を行う
     * @param  x     入力データ
     * @param  train 学習時にtrueを指定
     * @return forward演算結果
     */
    virtual	FrameBuffer Forward(FrameBuffer x, bool train=true) = 0;

   /**
     * @brief  forward演算(複数入力対応)
     * @detail forward演算を行う
     *         分岐や合流演算を可能とするために汎用版を定義しておく
     * @return forward演算結果
     */
    virtual	std::vector<FrameBuffer> Forward(std::vector<FrameBuffer> vx, bool train = true)
    {
        BB_ASSERT(vx.size() == 1);
        auto y = Forward(vx[0], train);
        return {y};
    }


   /**
     * @brief  backward演算
     * @detail backward演算を行う
     *         
     * @return backward演算結果
     */
	virtual	FrameBuffer Backward(FrameBuffer dy) = 0;

   /**
     * @brief  backward演算(複数入力対応)
     * @detail backward演算を行う
     *         分岐や合流演算を可能とするために汎用版を定義しておく
     * @return backward演算結果
     */
    virtual	std::vector<FrameBuffer> Backward(std::vector<FrameBuffer> vdy)
    {
        BB_ASSERT(vdy.size() == 1);
        auto dx = Backward(vdy[0]);
        return {dx};
    }
	
	
public:
	// Serialize
  	virtual void Save(std::ostream &os) const
	{
        int size = (int)m_name.size();
        os.write((char const *)&size, sizeof(size));
        os.write((char const *)&m_name[0], size);
	}

	virtual void Load(std::istream &is)
	{
        int size;
        is.read((char*)&size, sizeof(size));
        m_name.resize(size);
        is.read((char*)&m_name[0], size);
	}

    void SaveBinary(std::string filename) const
    {
        std::ofstream ofs(filename, std::ios::binary);
         Save(ofs);
    }

    void LoadBinary(std::string filename)
    {
        std::ifstream ifs(filename, std::ios::binary);
        Load(ifs);
    }


	// Serialize(CEREAL)
#if BB_WITH_CEREAL
	template <class Archive>
	void save(Archive& archive, std::uint32_t const version) const
	{
		archive(cereal::make_nvp("name", m_name));
	}

	template <class Archive>
	void load(Archive& archive, std::uint32_t const version)
	{
		archive(cereal::make_nvp("name", m_name));
	}

	virtual void Save(cereal::JSONOutputArchive& archive) const
	{
		archive(cereal::make_nvp("Model", *this));
	}

	virtual void Load(cereal::JSONInputArchive& archive)
	{
		archive(cereal::make_nvp("Model", *this));
	}

	void SaveJson(std::string filename) const
    {
        std::ofstream ofs(filename);
        cereal::JSONOutputArchive archive(ofs);
		Save(archive);
	}

	void LoadJson(std::string filename)
    {
        std::ifstream ifs(filename);
        cereal::JSONInputArchive archive(ifs);
		Load(archive);
	}
#endif


};



}