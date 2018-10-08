﻿// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                     Copyright (C) 2018 by Ryuji Fuchikami
//                                     https://github.com/ryuz
//                                     ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------



#pragma once


#include <vector>
#include <random>

#include <immintrin.h>

#include "NeuralNetLayerBuf.h"
#include "NeuralNetOptimizerSgd.h"


namespace bb {


// Convolutionクラス
template <typename T = float, typename INDEX = size_t>
class NeuralNetConvolution : public NeuralNetLayerBuf<T, INDEX>
{
protected:
	INDEX			m_frame_size = 1;
	int				m_input_h_size;
	int				m_input_w_size;
	int				m_input_c_size;
	int				m_filter_h_size;
	int				m_filter_w_size;
	int				m_output_h_size;
	int				m_output_w_size;
	int				m_output_c_size;
	std::vector <T>	m_W;
	std::vector <T>	m_b;
	std::vector <T>	m_dW;
	std::vector <T>	m_db;
	std::unique_ptr< ParamOptimizer<T, INDEX> >	m_optimizer_W;
	std::unique_ptr< ParamOptimizer<T, INDEX> >	m_optimizer_b;

	bool			m_binary_mode = false;

public:
	NeuralNetConvolution() {}
	
	NeuralNetConvolution(INDEX input_c_size, INDEX input_h_size, INDEX input_w_size, INDEX output_c_size, INDEX filter_h_size, INDEX filter_w_size, std::uint64_t seed = 1,
		const NeuralNetOptimizer<T, INDEX>* optimizer = nullptr)
	{
		NeuralNetOptimizerSgd<T, INDEX> DefOptimizer;
		if (optimizer == nullptr) {
			optimizer = &DefOptimizer;
		}
		
		Resize(input_c_size, input_h_size, input_w_size, output_c_size, filter_h_size, filter_w_size, seed);
		SetOptimizer(optimizer);
	}
	
	~NeuralNetConvolution() {}		// デストラクタ

	std::string GetClassName(void) const { return "NeuralNetConvolution"; }

	void Resize(INDEX input_c_size, INDEX input_h_size, INDEX input_w_size, INDEX output_c_size, INDEX filter_h_size, INDEX filter_w_size, std::uint64_t seed = 1)
	{
		m_input_c_size = (int)input_c_size;
		m_input_h_size = (int)input_h_size;
		m_input_w_size = (int)input_w_size;
		m_filter_h_size = (int)filter_h_size;
		m_filter_w_size = (int)filter_w_size;
		m_output_c_size = (int)output_c_size;
		m_output_h_size = m_input_h_size - m_filter_h_size + 1;
		m_output_w_size = m_input_w_size - m_filter_w_size + 1;

		m_W.resize(m_output_c_size*m_input_c_size*m_filter_h_size*m_filter_w_size);
		m_b.resize(m_output_c_size);
		m_dW.resize(m_output_c_size*m_input_c_size*m_filter_h_size*m_filter_w_size);
		m_db.resize(m_output_c_size);

		std::mt19937_64 mt(seed);
//		std::uniform_real_distribution<T> real_dist((T)0, (T)1);
		std::normal_distribution<T>		real_dist((T)0.0, (T)1.0);
		for (auto& w : m_W) {
			w = real_dist(mt);
		}
		for (auto& b : m_b) {
			b = real_dist(mt);
		}
	}

	void  SetOptimizer(const NeuralNetOptimizer<T, INDEX>* optimizer)
	{
		m_optimizer_W.reset(optimizer->Create(m_output_c_size*m_input_c_size*m_filter_h_size*m_filter_w_size));
		m_optimizer_b.reset(optimizer->Create(m_output_c_size));
	}

	void  SetBinaryMode(bool enable){
		m_binary_mode = enable;
	}

	T& W(INDEX n, INDEX c, INDEX y, INDEX x) {
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		BB_ASSERT(c >= 0 && c < m_input_c_size);
		BB_ASSERT(y >= 0 && y < m_input_h_size);
		BB_ASSERT(x >= 0 && x < m_input_w_size);
		return m_W[((n*m_input_c_size + c)*m_filter_h_size + y)*m_filter_w_size + x];
	}

	T& b(INDEX n) {
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		return m_b[n];
	}

	T& dW(INDEX n, INDEX c, INDEX y, INDEX x) {
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		BB_ASSERT(c >= 0 && c < m_input_c_size);
		BB_ASSERT(y >= 0 && y < m_input_h_size);
		BB_ASSERT(x >= 0 && x < m_input_w_size);
		return m_dW[((n*m_input_c_size + c)*m_filter_h_size + y)*m_filter_w_size + x];
	}

	T& db(INDEX n) {
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		return m_db[n];
	}
	
	void SetBatchSize(INDEX batch_size) {
		m_frame_size = batch_size;
	}
	
	INDEX GetInputFrameSize(void) const { return m_frame_size; }
	INDEX GetInputNodeSize(void) const { return m_input_c_size * m_input_h_size * m_input_w_size; }
	INDEX GetOutputFrameSize(void) const { return m_frame_size; }
	INDEX GetOutputNodeSize(void) const { return m_output_c_size * m_output_h_size * m_output_w_size; }
	
	int   GetInputSignalDataType(void) const { return NeuralNetType<T>::type; }
	int   GetInputErrorDataType(void) const { return NeuralNetType<T>::type; }
	int   GetOutputSignalDataType(void) const { return NeuralNetType<T>::type; }
	int   GetOutputErrorDataType(void) const { return NeuralNetType<T>::type; }
	
protected:

	inline T* GetInputPtr(NeuralNetBuffer<T, INDEX>& buf, int c, int y, int x)
	{
		return (T*)buf.GetPtr((c*m_input_h_size + y)*m_input_w_size + x);
	}

	inline T* GetOutputPtr(NeuralNetBuffer<T, INDEX>& buf, int c, int y, int x)
	{
		return (T*)buf.GetPtr((c*m_output_h_size + y)*m_output_w_size + x);
	}

	inline T* GetOutputPtrWithRangeCheck(NeuralNetBuffer<T, INDEX>& buf, int c, int y, int x)
	{
		if (x < 0 || x >= m_output_w_size || y < 0 || y >= m_output_h_size) {
			return (T*)buf.GetZeroPtr();
		}

		return (T*)buf.GetPtr((c*m_output_h_size + y)*m_output_w_size + x);
	}

	inline T* GetWPtr(NeuralNetBuffer<T, INDEX>& buf, INDEX n, INDEX c, INDEX y, INDEX x)
	{
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		BB_ASSERT(c >= 0 && c < m_input_c_size);
		BB_ASSERT(y >= 0 && y < m_input_h_size);
		BB_ASSERT(x >= 0 && x < m_input_w_size);
		return (T*)buf.GetPtr(((n*m_input_c_size + c)*m_filter_h_size + y)*m_filter_w_size + x);
	}

	// 水平加算
	inline static __m256 my_mm256_hsum_ps(__m256 r)
	{
		r = _mm256_hadd_ps(r, r);
		r = _mm256_hadd_ps(r, r);
		__m256 tmp = _mm256_permute2f128_ps(r, r, 0x1);
		r = _mm256_unpacklo_ps(r, tmp);
		return _mm256_hadd_ps(r, r);
	}

	inline float my_mm256_sum_ps(__m256 r)
	{
		return _mm256_cvtss_f32(my_mm256_hsum_ps(r));
	}

	inline __m256	my_mm256_fmadd_ps(__m256 a, __m256 b, __m256 c) {
		return _mm256_fmadd_ps(a, b, c);
	}

public:
	void Forward(bool train = true)
	{
		if (typeid(T) == typeid(float)) {
			// float用実装
			int  m256_frame_size = (int)(((m_frame_size + 7) / 8) * 8);
			auto in_sig_buf = this->GetInputSignalBuffer();
			auto out_sig_buf = this->GetOutputSignalBuffer();

#pragma omp parallel for
			for (int n = 0; n < m_output_c_size; ++n) {
				for (int y = 0; y < m_output_h_size; ++y) {
					for (int x = 0; x < m_output_w_size; ++x) {
						float* out_ptr = GetOutputPtr(out_sig_buf, n, y, x);
						for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
							_mm256_store_ps(&out_ptr[frame], _mm256_set1_ps(b(n)));
						}
						for (int c = 0; c < m_input_c_size; ++c) {
							for (int fy = 0; fy < m_filter_h_size; ++fy) {
								for (int fx = 0; fx < m_filter_w_size; ++fx) {
									int ix = x + fx;
									int iy = y + fy;
									float* in_ptr = GetInputPtr(in_sig_buf, c, iy, ix);
									__m256 W_val = _mm256_set1_ps(W(n, c, fy, fx));
									for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
										__m256 out_sig = _mm256_load_ps(&out_ptr[frame]);
										__m256 in_sig = _mm256_load_ps(&in_ptr[frame]);
										out_sig = _mm256_fmadd_ps(W_val, in_sig, out_sig);
										_mm256_store_ps(&out_ptr[frame], out_sig);
									}
								}
							}
						}
					}
				}
			}
		}
		else if (typeid(T) == typeid(double)) {
			// double用実装
		}
		else {
			assert(0);
		}
	}
	
	void Backward(void)
	{
		if (typeid(T) == typeid(float)) {
			// float用実装
			int  m256_frame_size = (int)(((m_frame_size + 7) / 8) * 8);
			auto in_sig_buf = this->GetInputSignalBuffer();
			auto out_sig_buf = this->GetOutputSignalBuffer();
			auto in_err_buf = this->GetInputErrorBuffer();
			auto out_err_buf = this->GetOutputErrorBuffer();

			// パラメータdWの計算
#pragma omp parallel for
			for (int n = 0; n < m_output_c_size; ++n) {
				__m256 sum_db = _mm256_set1_ps(0);
				for (int c = 0; c < m_input_c_size; ++c) {
					for (int fy = 0; fy < m_filter_h_size; ++fy) {
						for (int fx = 0; fx < m_filter_w_size; ++fx) {
							__m256 sum_dW = _mm256_set1_ps(0);
							for (int y = 0; y < m_output_h_size; ++y) {
								for (int x = 0; x < m_output_w_size; ++x) {
									int ix = x + fx;
									int iy = y + fy;
									float* out_err_ptr = GetOutputPtr(out_err_buf, n, y, x);
									float* in_sig_ptr = GetInputPtr(in_sig_buf, c, iy, ix);
									for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
										__m256 out_err = _mm256_load_ps(&out_err_ptr[frame]);
										__m256 in_sig = _mm256_load_ps(&in_sig_ptr[frame]);
				//						__m256 mul_val = _mm256_mul_ps(in_sig, out_err);
				//						sum_dW = _mm256_add_ps(sum_dW, mul_val);
										sum_dW = my_mm256_fmadd_ps(in_sig, out_err, sum_dW);
									}
								}
							}
							dW(n, c, fy, fx) = my_mm256_sum_ps(sum_dW);
						}
					}
				}
			}

			// パラメータdbの計算
#pragma omp parallel for
			for (int n = 0; n < m_output_c_size; ++n) {
				__m256 sum_db = _mm256_set1_ps(0);
				for (int y = 0; y < m_output_h_size; ++y) {
					for (int x = 0; x < m_output_w_size; ++x) {
						float* out_err_ptr = GetOutputPtr(out_err_buf, n, y, x);
						for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
							__m256 out_err = _mm256_load_ps(&out_err_ptr[frame]);
							sum_db = _mm256_add_ps(sum_db, out_err);
						}
					}
				}
				db(n) = my_mm256_sum_ps(sum_db);
			}


			// 入力への逆伝播
			in_err_buf.Clear();
			for (int c = 0; c < m_input_c_size; ++c) {
#pragma omp parallel for
				for (int y = 0; y < m_input_h_size; ++y) {
					for (int x = 0; x < m_input_w_size; ++x) {
						float* in_err_ptr = GetInputPtr(in_err_buf, c, y, x);
						for (int n = 0; n < m_output_c_size; ++n) {
							for (int fy = 0; fy < m_filter_h_size; ++fy) {
								for (int fx = 0; fx < m_filter_w_size; ++fx) {
									int ox = x - fx;
									int oy = y - fy;
									__m256 W_val = _mm256_set1_ps(W(n, c, fy, fx));
									float* out_err_ptr = GetOutputPtrWithRangeCheck(out_err_buf, n, oy, ox);
									for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
										__m256 in_err = _mm256_load_ps(&in_err_ptr[frame]);
										__m256 out_err = _mm256_load_ps(&out_err_ptr[frame]);
										in_err = _mm256_fmadd_ps(W_val, out_err, in_err);
										_mm256_store_ps(&in_err_ptr[frame], in_err);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	void Update(void)
	{
		m_optimizer_W->Update(m_W, m_dW);
		m_optimizer_b->Update(m_b, m_db);

		if (m_binary_mode) {
			for (int n = 0; n < m_output_c_size; ++n) {
				for (int c = 0; c < m_input_c_size; ++c) {
					for (int fy = 0; fy < m_filter_h_size; ++fy) {
						for (int fx = 0; fx < m_filter_w_size; ++fx) {
							W(n, c, fy, fx) = std::min((T)+1, std::max((T)-1, W(n, c, fy, fx)));
						}
					}
				}
			}
		}

		/*
		for (size_t i = 0; i < m_W.size(); ++i) {
			m_W[i] -= (T)(m_dW[i] * 0.01);
		}
		for (size_t i = 0; i < m_b.size(); ++i) {
			m_b[i] -= (T)(m_db[i] * 0.01);
		}
		*/
	}

};


}