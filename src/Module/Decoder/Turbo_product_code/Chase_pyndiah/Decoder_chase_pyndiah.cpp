#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "Decoder_chase_pyndiah.hpp"

#include "Tools/Exception/exception.hpp"
#include "Tools/Perf/hard_decision.h"
#include "Tools/Perf/compute_parity.h"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename B, typename R>
Decoder_chase_pyndiah<B,R>
::Decoder_chase_pyndiah(const int& n_ite,
                        const Interleaver<R> &pi,
                        Decoder_HIHO<B> &hiho_r,
                        Decoder_HIHO<B> &hiho_c,
	                    const R alpha_,
                        const int n_least_reliable_positions_,
                        const int n_competitors_)
: Decoder(hiho_r.get_K() * hiho_c.get_K(), pi.get_core().get_size(), hiho_r.get_n_frames(), hiho_r.get_simd_inter_frame_level()),
  Decoder_turbo_product_code<B,R>(n_ite, pi, hiho_r, hiho_c),

  alpha                     (alpha_                                          ),
  n_least_reliable_positions(n_least_reliable_positions_                     ),
  least_reliable_pos        (n_least_reliable_positions                      ),
  hard_Rprime               (std::max(hiho_r.get_N(), hiho_c.get_N()) +1     ), // +1 for parity bit if any
  n_test_vectors            ((int)1 << n_least_reliable_positions            ),
  test_vect                 (n_test_vectors * hard_Rprime.size()             ),
  metrics                   (n_test_vectors                                  ),
  n_competitors             (n_competitors_ ? n_competitors_ : n_test_vectors),
  competitors               (n_test_vectors                                  ),
  Y_N_cha_i                 (pi.get_core().get_size()                        )
{
	const std::string name = "Decoder_chase_pyndiah";
	this->set_name(name);

	if (n_least_reliable_positions <= 0 || n_least_reliable_positions >= hiho_r.get_N() || n_least_reliable_positions >= hiho_c.get_N())
	{
		std::stringstream message;
		message << "'n_least_reliable_positions' has to be positive and lower than 'hiho_r.get_N()' and 'hiho_c.get_N()' ('n_least_reliable_positions' = "
		        << n_least_reliable_positions << ", 'hiho_c.get_N()' = " << hiho_c.get_N() << " and 'hiho_r.get_N()' = " << hiho_r.get_N() << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (n_competitors <= 0 || n_competitors > n_test_vectors)
	{
		std::stringstream message;
		message << "'n_competitors' has to be positive and lower or equal than 'n_test_vectors' ('n_competitors' = "
		        << n_competitors << " and 'n_test_vectors' = " << n_test_vectors << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}


	// TODO : donner possibilité de choisir les vecteurs de tests (n_test_vectors doit pouvoir être inférieur à 2^n_least_reliable_positions)
	// consequently, in apply_candidate, invert the bit value and not force 0 or 1
}

template <typename B, typename R>
void Decoder_chase_pyndiah<B,R>
::_decode(const R *Y_N_cha, int return_K_siso)
{
	const int n_cols = this->hiho_r.get_N() + (this->parity_extended ? 1 : 0);
	const int n_rows = this->hiho_c.get_N() + (this->parity_extended ? 1 : 0);

	this->pi.interleave(Y_N_cha, this->Y_N_cha_i.data(), 0, 1); // interleave data from the channel


	for (int i = 0; i < this->n_ite; i++)
	{
		this->pi.interleave(this->Y_N_i.data(), this->Y_N_pi.data(), 0, 1); // columns becomes rows

		// decode each col
		for (int j = 0; j < n_cols; j++)
			_decode_row_siso(Y_N_cha_i   .data() + j*n_rows,
			                 this->Y_N_pi.data() + j*n_rows,
			                 this->Y_N_pi.data() + j*n_rows,
			                 this->hiho_c,
			                 n_rows); // overwrite Y_N_pi


		this->pi.deinterleave(this->Y_N_pi.data(), this->Y_N_i.data(), 0, 1); // rows go back as columns

		// decode each row
		if (i < this->n_ite -1 || (return_K_siso != 0 && return_K_siso != 1))
		{
			for (int j = 0; j < n_rows; j++)
				_decode_row_siso(Y_N_cha + j*n_cols,
				                 this->Y_N_i.data() + j*n_cols,
				                 this->Y_N_i.data() + j*n_cols,
				                 this->hiho_r,
				                 n_cols); // overwrite Y_N_i
		}
		else if(return_K_siso == 0)
		{
			for (int j = 0; j < this->hiho_c.get_K(); j++)
				_decode_row_siho(Y_N_cha + j*n_cols,
				                 this->Y_N_i.data() + (j + this->hiho_c.get_N() - this->hiho_c.get_K())*n_cols,
				                 this->V_K_i.data() + j*this->hiho_r.get_K(),
				                 this->hiho_r, n_cols,
				                 true);
		}
		else if (return_K_siso == 1)
		{
			for (int j = 0; j < n_cols; j++)
				_decode_row_siho(Y_N_cha + j*n_cols,
				                 this->Y_N_i.data() + j*n_cols,
				                 this->V_N_i.data() + j*n_cols,
				                 this->hiho_r, n_cols,
				                 false);
		}

	}
}


template <typename B, typename R>
void Decoder_chase_pyndiah<B,R>
::_decode_row_siho(const R *R_cha, const R *R_prime, B *R_dec, Decoder_HIHO<B> &hiho, const int size, const bool return_K)
{
	if (_decode_chase(R_prime, hiho, size))
	{	// syndrome ok, R_prime is a code word, then copy its hard decided version and exit the function
		std::copy(hard_Rprime.data() + (return_K ? hiho.get_N() - hiho.get_K() : 0),
		          hard_Rprime.data() + (return_K ? hiho.get_N() : size),
		          R_dec);
		return;
	}

	auto* DW = test_vect.data() + competitors.front().pos;

	std::copy(DW + (return_K ? hiho.get_N() - hiho.get_K() : 0),
	          DW + (return_K ? hiho.get_N() : size),
	          R_dec);
}

template <typename B, typename R>
void Decoder_chase_pyndiah<B,R>
::_decode_row_siso(const R *R_cha, const R *R_prime, R *R_dec, Decoder_HIHO<B> &hiho, const int size)
{
	if (_decode_chase(R_prime, hiho, size))
	{	// syndrome ok, R_prime is a code word, then copy it as decided word and exit the function
		std::copy(R_prime, R_prime + size, R_dec);
		return;
	}

	compute_reliability(R_cha, R_prime, R_dec, size);
}

template <typename B, typename R>
bool Decoder_chase_pyndiah<B,R>
::_decode_chase(const R *R_prime, Decoder_HIHO<B> &hiho, const int size)
{
	tools::hard_decide(R_prime, hard_Rprime.data(), size);

	// if (this->parity_extended)
	// 	parity_diff = tools::compute_parity(hard_Rprime.data()) ^ hard_Rprime[N -1];
	// else
	// 	parity_diff = false;

	find_least_reliable_pos(R_prime, hiho.get_N()); // without parity bit if any
	compute_test_vectors   (hiho,    size        );
	compute_metrics        (R_prime, size        );

	return false;
}

template <typename B, typename R>
void Decoder_chase_pyndiah<B,R>
::find_least_reliable_pos(const R* R_prime, const int size)
{
	for (auto it = least_reliable_pos.begin(); it != least_reliable_pos.end(); it++)
	{
	    it->metric = std::numeric_limits<R>::max();
	    it->pos    = size;
	}

	for (int i = 0; i < size; i++)
	{
		// check if R[i] is a least reliable value compared to the already saved positions
		for (auto it = least_reliable_pos.begin(); it != least_reliable_pos.end(); it++)
		{
			R val = std::abs(R_prime[i]);

			if (val < it->metric)
			{ // R[i] is weak, then save its position
				least_reliable_pos.erase(least_reliable_pos.end() -1); // remove the last position that is not weak enough
				least_reliable_pos.insert(it, {val, i});
				break;
			}
		}
	}
}

template <typename B, typename R>
void Decoder_chase_pyndiah<B,R>
::compute_test_vectors(Decoder_HIHO<B> &hiho, const int size)
{
	for (int c = 0; c < n_test_vectors; c++)
	{
		// rearrange hard_Rprime to be a good candidate
		apply_candidate(hard_Rprime.data(), c);

		hiho.decode_hiho_cw(hard_Rprime.data(), test_vect.data() + c*size); // parity bit is ignored by the decoder

		if (this->parity_extended)
			test_vect[(c+1)*size -1] = tools::compute_parity(test_vect.data() + c*size, hiho.get_N());
	}
}

template <typename B, typename R>
void Decoder_chase_pyndiah<B,R>
::compute_metrics(const R* R_prime, const int size)
{
	std::fill(metrics.begin(), metrics.end(), (R)0);

	for (int c = 0; c < n_test_vectors; c++)
	{
		int tv_off = c*size;

		for (int i = 0; i < size; i++)
		{
			B sign = std::signbit(R_prime[i]) ? 1 : 0;

			if (sign ^ test_vect[tv_off + i])
				metrics[c] += std::abs(R_prime[i]);
		}
	}

	// reorder metrics -> decided word is at first position of copetitors list
	for (int c = 0; c < n_test_vectors; c++)
	{
		competitors[c].metric = metrics[c];
		competitors[c].pos    = c*size;
	}

	std::sort(competitors.begin(), competitors.end(), [](const info& a, const info& b) { return a.metric < b.metric; });
}

template <typename B, typename R>
void Decoder_chase_pyndiah<B,R>
::compute_reliability(const R* R_cha, const R* R_prime, R* R_dec, const int size)
{
	const auto & DW = competitors.front(); // the Decided Word

	// compute beta, the sum of the least reliable position reliabilities in the decided word
	R beta = 0;
	for (int i = 0; i < n_least_reliable_positions; i++)
		beta += least_reliable_pos[i].metric;


	for (int i = 0; i < size; i++)
	{
		const auto DB = test_vect[DW.pos + i]; // decided bit at the position i

		int j = 1;

		for (; j < n_competitors; j++)
			if (test_vect[competitors[j].pos + i] != DB)
				break;

		R reliability;

		if (j < n_competitors) // then there is a competitor with a different bit at the position i
		{
			reliability = competitors[j].metric - DW.metric;
		}
		else // same bits for each candidates
		{
			reliability = std::abs(R_prime[i]) + beta - DW.metric;
			if (reliability < 0)
				reliability = 0;
		}

		if (DB) // if DB is a 1
			reliability = -reliability; // set as negative

		R_dec[i] = R_cha[i] + alpha * (reliability - R_prime[i]);
	}
}

template <typename B, typename R>
void Decoder_chase_pyndiah<B,R>
::apply_candidate(B* hard_vect, const int c)
{
	// take the bits of 'c' as the different values to apply to each least reliable bits
	// being a counter, the bits of 'c' change for each candidate and so the test vector
	for (int i = 0; i < n_least_reliable_positions; i++)
		hard_vect[least_reliable_pos[i].pos] = (c >> i) & (int)1;
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef MULTI_PREC
template class aff3ct::module::Decoder_chase_pyndiah<B_8,Q_8>;
template class aff3ct::module::Decoder_chase_pyndiah<B_16,Q_16>;
template class aff3ct::module::Decoder_chase_pyndiah<B_32,Q_32>;
template class aff3ct::module::Decoder_chase_pyndiah<B_64,Q_64>;
#else
template class aff3ct::module::Decoder_chase_pyndiah<B,R>;
#endif
// ==================================================================================== explicit template instantiation
