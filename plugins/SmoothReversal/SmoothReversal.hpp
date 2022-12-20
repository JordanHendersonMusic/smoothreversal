// PluginSmoothReversal.hpp
// Jordan Henderson (j.henderson.music@outlook.com)

#pragma once

#include <optional>
#include "SC_PlugIn.hpp"

namespace SmoothReversal {

class ScopedSndBuf {
public:
	explicit ScopedSndBuf(SndBuf* ptr) noexcept : ptr(ptr) { ACQUIRE_SNDBUF_SHARED(ptr); }
	~ScopedSndBuf() noexcept { RELEASE_SNDBUF_SHARED(ptr); }
	ScopedSndBuf(ScopedSndBuf&&) noexcept = default;
	ScopedSndBuf(const ScopedSndBuf&) noexcept = default;
	ScopedSndBuf& operator=(ScopedSndBuf&&) noexcept = default;
	ScopedSndBuf& operator=(const ScopedSndBuf&) noexcept = default;

	[[nodiscard]] float* data() const;
	[[nodiscard]] uint32_t channels() const;
	[[nodiscard]] uint32_t samples() const;
	[[nodiscard]] uint32_t frames() const;
	[[nodiscard]] double sample_dur() const;
	[[nodiscard]] double sample_rate() const;
	[[nodiscard]] int mask() const;
	[[nodiscard]] int guardFrame() const;


	[[nodiscard]] float read(uint32_t index, uint32_t channel) const;
	[[nodiscard]] float no_lerp(double phase, uint32_t channel) const;
	[[nodiscard]] float linear_lerp(double phase, uint32_t channel) const;
	[[nodiscard]] float cubic_lerp(double phase, uint32_t channel) const;

private:
	void assert_phase(double phase) const;
	SndBuf* ptr;
};

class ScopedSndBufFactory {
public:
	explicit ScopedSndBufFactory(uint32_t buf_num, SndBuf* buffer_ptr) : buf_num(buf_num), buffer_ptr(buffer_ptr) {}
	~ScopedSndBufFactory() noexcept = default;
	ScopedSndBufFactory(ScopedSndBufFactory&&) noexcept = default;
	ScopedSndBufFactory(const ScopedSndBufFactory&) noexcept = default;
	ScopedSndBufFactory& operator=(ScopedSndBufFactory&&) noexcept = default;
	ScopedSndBufFactory& operator=(const ScopedSndBufFactory&) noexcept = default;

	[[nodiscard]] ScopedSndBuf buf() const { return ScopedSndBuf{buffer_ptr}; };
	[[nodiscard]] bool holds_buf_num(uint32_t bf) const { return buf_num == bf; }

private:
	uint32_t buf_num;
	SndBuf* buffer_ptr;
};

class ScopedSndBufManager {
public:
	const std::optional<ScopedSndBufFactory>& try_to_load_snd_buf_factory(uint32_t buf_num, World& world, uint32_t num_outputs, bool done);
private:
	std::optional<ScopedSndBufFactory> buf_factory{std::nullopt};
	std::optional<uint32_t> previous_invalid_buffer;
};

class Phase {
public:
	void reset(){ v = 0; }
	void increment(double step, double max_value);
	operator double() const { return v; }
private:
	double v{0};
};


class SmoothReversal : public SCUnit {
	enum Inputs { BufNum, PlayBackRate, SwitchDirectionTrigger, Threshold };
public:
	SmoothReversal();
	~SmoothReversal() = default;

private:
	void next(int nSamples);
	ScopedSndBufManager scoped_snd_buf_manager;

	Phase phase;
	bool target_direction{true};
	bool direction{true};
};

} // namespace SmoothReversal
