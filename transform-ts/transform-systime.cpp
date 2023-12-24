#include "transform-systime.h"
#include <assert.h>

static const auto MAX_TS_VAR = 2000000000ULL;           // 2s in nanoseconds
static const auto TS_SMOOTHING_THRESHOLD = 70000000ULL; // 70ms in nanoseconds

transform_systime::transform_systime(transform_event *cb) : callback(cb)
{
	assert(cb != nullptr);
}

void transform_systime::reset()
{
	std::scoped_lock<std::recursive_mutex> auto_lock(info_lock);
	audio_info = timestamp_info();
	video_info = timestamp_info();
}

int64_t transform_systime::audio_transform_to_system(int64_t ts_ns, size_t sample_rate, size_t frames_per_channel)
{
	int64_t os_time = os_gettime_ns();
	int64_t transformed_ts = ts_ns;
	bool ts_jumped = false;
	bool new_capture = false;
	timestamp_info old_audio;
	timestamp_info old_video;

	{
		std::scoped_lock<std::recursive_mutex> auto_lock(info_lock);

		if (uint64_diff(ts_ns, os_time) < MAX_TS_VAR) {
			audio_info = timestamp_info();
			return ts_ns;
		}

		if (!audio_info.timing_set) {
			reset_timing(audio_info, ts_ns, os_time);

		} else {
			do {
				if (audio_info.prev_data_ts > 0 && ts_ns < audio_info.prev_data_ts) {
					new_capture = true; // 新数据的时间戳 比上一帧的时间戳还小，说明开始了新的采集会话
					audio_info = timestamp_info();
					video_info = timestamp_info();
					assert(false);
					break;
				}

				if (audio_info.next_data_ts != 0) {
					auto diff = uint64_diff(audio_info.next_data_ts, ts_ns);
					if (diff > MAX_TS_VAR) {
						ts_jumped = true;
						break;

					} else if (diff < TS_SMOOTHING_THRESHOLD) {
						ts_ns = audio_info.next_data_ts;
						break;
					}
				}
			} while (false);

			if (new_capture || ts_jumped) {
				old_audio = audio_info; // firstly save old value
				old_video = video_info;
				reset_timing(audio_info, ts_ns, os_time);
			}
		}

		audio_info.prev_data_ts = ts_ns;
		audio_info.next_data_ts = ts_ns + frames_duration_ns(sample_rate, frames_per_channel);

		auto best_adjust = get_best_adjust();
		transformed_ts = ts_ns + best_adjust;

		if (audio_info.used_timing_adjust != best_adjust) {
			audio_info.used_timing_adjust = best_adjust;
			// log : best_adjust changed
		}
	}

	if (callback) {
		if (new_capture) {
			callback->request_clear_audio_catch(old_audio);
			callback->request_clear_video_catch(old_video);
		} else if (ts_jumped) {
			callback->request_clear_audio_catch(old_audio);
		}
	}

	return transformed_ts;
}

int64_t transform_systime::video_transform_to_system(int64_t ts_ns, int fps)
{
	// TODO
	return ts_ns;
}

int64_t transform_systime::os_gettime_ns()
{
	auto tm = std::chrono::steady_clock::now().time_since_epoch().count();
	return tm;
}

int64_t transform_systime::uint64_diff(int64_t ts1, int64_t ts2)
{
	return (ts1 < ts2) ? (ts2 - ts1) : (ts1 - ts2);
}

void transform_systime::reset_timing(timestamp_info &info, int64_t ts_ns, int64_t os_ns)
{
	if (info.next_data_ts > 0) {
		// log : timestamp jumped
	}

	info.timing_set = true;
	info.timing_adjust = os_ns - ts_ns;
	info.base_data_ts = ts_ns;

	info.prev_data_ts = ts_ns;
	info.next_data_ts = 0;
}

int64_t transform_systime::frames_duration_ns(size_t sample_rate, size_t frames_per_channel)
{
	if (!sample_rate)
		return 0;

	auto percent = double(frames_per_channel) / double(sample_rate);
	auto duration = percent * 1000000000.0;

	return int64_t(duration);
}

int64_t transform_systime::get_best_adjust()
{
	// 如果音视频同时都有转换基数，则取对应最小的数据时间戳所计算出来的转换基数

	std::scoped_lock<std::recursive_mutex> auto_lock(info_lock);

	if (audio_info.timing_set && video_info.timing_set) {
		if (audio_info.base_data_ts < video_info.base_data_ts) {
			return audio_info.timing_adjust;
		} else {
			return video_info.timing_adjust;
		}
	}

	if (audio_info.timing_set) {
		return audio_info.timing_adjust;
	}

	if (video_info.timing_set) {
		return video_info.timing_adjust;
	}

	assert(false);
	return 0;
}
