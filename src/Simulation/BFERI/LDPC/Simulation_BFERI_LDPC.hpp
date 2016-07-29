#ifndef SIMULATION_BFERI_LDPC_HPP_
#define SIMULATION_BFERI_LDPC_HPP_

#include "../Simulation_BFERI.hpp"

#include "../../../Tools/params.h"

template <typename B, typename R, typename Q>
class Simulation_BFERI_LDPC : public Simulation_BFERI<B,R,Q>
{
public:
	Simulation_BFERI_LDPC(const t_simulation_param& simu_params,
	                      const t_code_param&       code_params,
	                      const t_encoder_param&    enco_params,
	                      const t_mod_param&        mod_params,
	                      const t_channel_param&    chan_params,
	                      const t_decoder_param&    deco_params);

	virtual ~Simulation_BFERI_LDPC();

protected:
	void          launch_precompute();
	void          snr_precompute   ();
	Encoder<B>*   build_encoder    (const int tid = 0);
	SISO<Q>*      build_siso       (const int tid = 0);
	Decoder<B,Q>* build_decoder    (const int tid = 0);
};

#endif /* SIMULATION_BFERI_LDPC_HPP_ */
