// PluginSmoothReversal.cpp
// Jordan Henderson (j.henderson.music@outlook.com)

#include "SC_PlugIn.hpp"
#include "SmoothReversal.hpp"

static InterfaceTable* ft;

namespace SmoothReversal {

inline uint32_t get_low_phase_clipped(double p, uint32_t clip) {
	return std::min(static_cast<uint32_t>(std::floor(p)), clip);
}


float* ScopedSndBuf::data() const { return ptr->data; }
uint32_t ScopedSndBuf::channels() const { return ptr->channels; }
uint32_t ScopedSndBuf::samples() const { return ptr->samples; }
uint32_t ScopedSndBuf::frames() const { return ptr->frames; }
double ScopedSndBuf::sample_dur() const { return ptr->sampledur; }
double ScopedSndBuf::sample_rate() const { return ptr->samplerate; }
int ScopedSndBuf::mask() const { return ptr->mask; }
int ScopedSndBuf::guardFrame() const { return ptr->frames - 2; }
void ScopedSndBuf::assert_phase(double phase) const {
	assert(phase > 0);
	assert(phase < frames());
}
float ScopedSndBuf::read(uint32_t index, uint32_t channel) const {
	assert(index < frames());
	assert(channel < channels());
	// ... close your eyes and think of the England
	return *(data() + (index * channels()) + channel);
}
float ScopedSndBuf::no_lerp(double phase, uint32_t channel) const {
	assert_phase(phase);
	const std::size_t low = std::floor(phase);
	return read(low, channel);
}
float ScopedSndBuf::linear_lerp(double phase, uint32_t channel) const {
	assert_phase(phase);

	const uint32_t low = get_low_phase_clipped(phase, frames() - 2);
	const auto fractional = static_cast<float>(phase - static_cast<double>(low));

	// time now, time + 1
	const float t1 = read(low, channel);
	const float t2 = read(low + 1, channel);

	const auto diff = t2 - t1;

	return t1 + fractional * diff;
}
float ScopedSndBuf::cubic_lerp(double phase, uint32_t channel) const {
	assert_phase(phase);

	// cannot use the first nor last two values with cubic
	const uint32_t low = std::max(uint32_t{1}, get_low_phase_clipped(phase, frames() - 3));
	const auto fractional = static_cast<float>(phase - static_cast<double>(low));

	// t0 is in the past, t1 is now, etc...
	const float t0 = read(low, channel);
	const float t1 = read(low, channel);
	const float t2 = read(low + 1, channel);
	const float t3 = read(low + 2, channel);

	return cubicinterp(fractional, t0, t1, t2, t3);
}

const std::optional<ScopedSndBufFactory>&
ScopedSndBufManager::try_to_load_snd_buf_factory(uint32_t buf_num, World& world, uint32_t num_outputs, bool done) {
	if (buf_factory && buf_factory->holds_buf_num(buf_num)) [[likely]]
		return buf_factory;

	const auto valid_snd_buf_num = [&](uint32_t b) -> bool {
		return 0 <= buf_num && buf_num < world.mNumSndBufs;
	};

	if (!valid_snd_buf_num(buf_num)) {
		if (previous_invalid_buffer != buf_num)
			Print("Bad buffer\n");
		previous_invalid_buffer = buf_num;
		return buf_factory = std::nullopt;
	}

	SndBuf* maybe_buffer{world.mSndBufs + buf_num};

	// should be impossible
	assert(maybe_buffer != nullptr);
	if (maybe_buffer == nullptr) {
		if (previous_invalid_buffer != buf_num)
			Print("Bad buffer\n");
		return buf_factory = std::nullopt;
	}

	// buffer wasn't loaded
	if (maybe_buffer->data == nullptr) {
		if (world.mVerbosity > -1 && !done && (previous_invalid_buffer != buf_num))
			Print("Buffer UGen: no buffer data\n");
		previous_invalid_buffer = buf_num;
		return buf_factory = std::nullopt;
	}

	if (num_outputs != maybe_buffer->channels) {
		if (world.mVerbosity > -1 && !done && (previous_invalid_buffer != buf_num))
			Print("Buffer UGen channel mismatch: expected %i, yet buffer has %i channels\n", num_outputs,
			      maybe_buffer->channels);
		previous_invalid_buffer = buf_num;
		return buf_factory = std::nullopt;
	}

	return buf_factory = ScopedSndBufFactory{buf_num, maybe_buffer};
}


template<typename I>
I wrap(I in, I max) {
	I out{in};
	while (out > max) out -= max;
	while (out < 0) out += max;
	return out;
}

void Phase::increment(double step, double max_value) {
	v += step;
	v = wrap(v, static_cast<double>(max_value));
}


SmoothReversal::SmoothReversal() {
	mCalcFunc = make_calc_function<SmoothReversal, &SmoothReversal::next>();
	phase.reset();
	next(1);
}

void SmoothReversal::next(int num_samples) {
	const auto buffer_factory = scoped_snd_buf_manager.try_to_load_snd_buf_factory(
			static_cast<uint32_t>(in0(Inputs::BufNum)),
			*mWorld,
			mNumOutputs,
			mDone == 0
	);
	if (!buffer_factory) {
		ClearUnitOutputs(this, num_samples);
		return;
	}

	auto buf{buffer_factory->buf()};
	const float* playback_rate_signal{in(Inputs::PlayBackRate)};
	const float* switch_direction_trigger{in(Inputs::SwitchDirectionTrigger)};
	const float threshold{in(Inputs::Threshold)[0]};

	float* output = out(0);

	for (int i{0}; i < num_samples; ++i) {
		if (switch_direction_trigger[i] > 0.5f)
			target_direction = !target_direction;

		const auto phase_step = playback_rate_signal[i] * (direction ? 1.0 : -1.0);
		phase.increment(phase_step, static_cast<double>(buf.frames()));

		// will change direction on the next sample
		if (target_direction != direction) {
			const bool at_end_or_beginning_of_buffer = phase >= (buf.frames() - 2) or phase < 1;
			if (not at_end_or_beginning_of_buffer) {
				const float delta_d = buf.no_lerp(phase + (target_direction ? -1 : 1), 0) - buf.no_lerp(phase, 0);
				if (std::abs(delta_d) < threshold)
					direction = target_direction;
			}
		}

		output[i] = buf.cubic_lerp(phase, 0);
	}
}


}


PluginLoad(SmoothReversalUGens) {
	ft = inTable;
	registerUnit<SmoothReversal::SmoothReversal>(ft, "SmoothReversal", false);
}
