#include "BCH/Codec_BCH.hpp"

#include "Codec_HIHO.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Codec_HIHO_name   = "Codec HIHO";
const std::string aff3ct::factory::Codec_HIHO_prefix = "cdc";

Codec_HIHO::parameters
::parameters(const std::string &prefix)
: Codec::parameters(Codec_HIHO_name, prefix)
{
}

Codec_HIHO::parameters
::parameters(const std::string &name, const std::string &prefix)
: Codec::parameters(name, prefix)
{
}

Codec_HIHO::parameters
::~parameters()
{
}

template <typename B, typename Q>
module::Codec_HIHO<B,Q>* Codec_HIHO::parameters
::build(module::CRC<B>* crc) const
{
	     if (get_name() == "Codec BCH"       ) return dynamic_cast<const Codec_BCH       ::parameters&>(*this).template build<B,Q>(crc);

	throw tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

template <typename B, typename Q>
module::Codec_HIHO<B,Q>* Codec_HIHO
::build(const parameters &params, module::CRC<B> *crc)
{
	return params.template build<B,Q>(crc);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef MULTI_PREC
template aff3ct::module::Codec_HIHO<B_8 ,Q_8 >* aff3ct::factory::Codec_HIHO::parameters::build<B_8 ,Q_8 >(aff3ct::module::CRC<B_8 >*) const;
template aff3ct::module::Codec_HIHO<B_16,Q_16>* aff3ct::factory::Codec_HIHO::parameters::build<B_16,Q_16>(aff3ct::module::CRC<B_16>*) const;
template aff3ct::module::Codec_HIHO<B_32,Q_32>* aff3ct::factory::Codec_HIHO::parameters::build<B_32,Q_32>(aff3ct::module::CRC<B_32>*) const;
template aff3ct::module::Codec_HIHO<B_64,Q_64>* aff3ct::factory::Codec_HIHO::parameters::build<B_64,Q_64>(aff3ct::module::CRC<B_64>*) const;
template aff3ct::module::Codec_HIHO<B_8 ,Q_8 >* aff3ct::factory::Codec_HIHO::build<B_8 ,Q_8 >(const aff3ct::factory::Codec_HIHO::parameters&, aff3ct::module::CRC<B_8 >*);
template aff3ct::module::Codec_HIHO<B_16,Q_16>* aff3ct::factory::Codec_HIHO::build<B_16,Q_16>(const aff3ct::factory::Codec_HIHO::parameters&, aff3ct::module::CRC<B_16>*);
template aff3ct::module::Codec_HIHO<B_32,Q_32>* aff3ct::factory::Codec_HIHO::build<B_32,Q_32>(const aff3ct::factory::Codec_HIHO::parameters&, aff3ct::module::CRC<B_32>*);
template aff3ct::module::Codec_HIHO<B_64,Q_64>* aff3ct::factory::Codec_HIHO::build<B_64,Q_64>(const aff3ct::factory::Codec_HIHO::parameters&, aff3ct::module::CRC<B_64>*);
#else
template aff3ct::module::Codec_HIHO<B,Q>* aff3ct::factory::Codec_HIHO::parameters::build<B,Q>(aff3ct::module::CRC<B>*) const;
template aff3ct::module::Codec_HIHO<B,Q>* aff3ct::factory::Codec_HIHO::build<B,Q>(const aff3ct::factory::Codec_HIHO::parameters&, aff3ct::module::CRC<B>*);
#endif
// ==================================================================================== explicit template instantiation
