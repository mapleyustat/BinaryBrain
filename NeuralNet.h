


#pragma once


#include <vector>


// NeuralNetの抽象クラス
template <typename T=float, typename ET = float, typename INDEX=size_t>
class NeuralNet
{
public:
	virtual ~NeuralNet() {}								// デストラクタ
	
	// 基本部分
	virtual INDEX GetInputFrameSize(void) const = 0;	// 入力のフレーム数
	virtual INDEX GetInputNodeSize(void) const = 0;		// 入力のノード数
	virtual INDEX GetOutputFrameSize(void) const = 0;	// 出力のフレーム数
	virtual INDEX GetOutputNodeSize(void) const = 0;	// 出力のノード数
	
	virtual void  SetInputValue(INDEX frame, INDEX node, T value) = 0;
	virtual T     GetInputValue(INDEX frame, INDEX node) = 0;
	virtual void  SetInputError(INDEX frame, INDEX node, ET error) = 0;
	virtual ET    GetInputError(INDEX frame, INDEX node) = 0;
	
	virtual void  SetOutput(INDEX frame, INDEX node, T value) = 0;
	virtual T     GetOutput(INDEX frame, INDEX node) = 0;
	virtual void  SetOutputError(INDEX frame, INDEX node, ET error) = 0;
	virtual ET    GetOutputError(INDEX frame, INDEX node) = 0;
	
	virtual	void  Forward(void) = 0;
	virtual	void  Backward(void) = 0;
	
	virtual INDEX  GetLayerSize(void) const = 0;			// 
	virtual INDEX  GetNodeSize(int layer) const = 0;
	virtual INDEX  GetEdgeSize(int layer, int node) const = 0;

	virtual void   SetConnection(int layer, int node, int input_num, int input_node) = 0;
	virtual int    GetConnection(int layer, int node, int input_num) const = 0;
	
	virtual void CalcForward(int layer = 0) = 0;

	virtual bool GetValue(int layer, int node) const = 0;
	virtual void SetValue(int layer, int node, bool value) = 0;

	virtual bool GetInputValue(int layer, int node, int index) const = 0;

	virtual bool GetLutBit(int layer, int node, int bit) const = 0;
	virtual void SetLutBit(int layer, int node, int bit, bool value) = 0;

	virtual void InvertLut(int layer, int node)
	{
		int n = GetInputNum(layer, node);
		int size = (1 << n);
		for (int i = 0; i < size; i++) {
			SetLutBit(layer, node, i, !GetLutBit(layer, node, i));
		}
		SetValue(layer, node, !GetValue(layer, node));
	}

	void SetInput(std::vector<bool> input_vector)
	{
		for ( int i = 0; i < (int)input_vector.size(); i++ ) {
			SetValue(0, i, input_vector[i]);
		}
	}

	std::vector<bool> GetOutput(void)
	{
		int layer = GetLayerNum() - 1;
		std::vector<bool> output_vector(GetNodeNum(layer));
		for (int i = 0; i < (int)output_vector.size(); i++) {
			output_vector[i] = GetValue(layer, i);
		}
		return output_vector;
	}

	int  GetInputLutIndex(int layer, int node) const
	{
		int num = GetInputNum(layer, node);
		
		int idx = 0;
		int bit = 1;
		for (int i = 0; i < num; i++) {
			idx |= GetInputValue(layer, node, i) ? bit : 0;
			bit <<= 1;
		}

		return idx;
	}
	

	// データエクスポート
	BinaryNetData ExportData(void) {
		BinaryNetData	bnd;
		for (int layer = 0; layer < GetLayerNum(); layer++) {
			if (layer == 0) {
				bnd.input_num = GetNodeNum(layer);
			}
			else {
				int node_num = GetNodeNum(layer);
				BinaryLayerData bld;
				bld.node.reserve(node_num);
				for (int node = 0; node < node_num; node++) {
					LutData ld;
					int num = GetInputNum(layer, node);
					ld.connect.reserve(num);
					for (int i = 0; i < num; i++) {
						ld.connect.push_back(GetConnection(layer, node, i));
					}
					ld.lut.reserve((size_t)1<<num);
					for (int i = 0; i < (1<<num); i++) {
						ld.lut.push_back(GetLutBit(layer, node, i));
					}
					bld.node.push_back(ld);
				}
				bnd.layer.push_back(bld);
			}
		}

		return bnd;
	}
};

