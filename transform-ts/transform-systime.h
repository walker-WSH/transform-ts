#pragma once
#include <mutex>
#include <atomic>
#include <stdint.h>
#include <thread>
#include <chrono>

struct timestamp_info {
	volatile bool timing_set = false;   // 当前数据流的转换基数是否已经初始化
	volatile int64_t base_data_ts = 0;  // 当前数据流的转换基数所基于的数据时间戳
	volatile int64_t timing_adjust = 0; // 当前数据流计算出来的转换基数

	volatile int64_t used_timing_adjust = 0; // 当前流实际使用的转换基数。如果音视频同时有转换基数，则取基准数据时间戳最小的值所计算出来的基数

	volatile int64_t prev_data_ts = 0; // 上一帧数据的时间戳
	volatile int64_t next_data_ts = 0; // 理论上下一帧数据的时间戳
};

class transform_event {
public:
	virtual ~transform_event() = default;
	virtual void request_clear_audio_catch(const timestamp_info &old_audio_info) = 0;
	virtual void request_clear_video_catch(const timestamp_info &old_video_info) = 0;
};

class transform_systime {
public:
	transform_systime(transform_event *cb);
	virtual ~transform_systime() = default;

	void reset(); // 采集端开始一个新的采集会话时 需要调用这个函数

        // 传入的时间戳 要保证是正常采集会话中的时间戳
	int64_t audio_transform_to_system(int64_t ts_ns, size_t sample_rate, size_t frames_per_channel);
	int64_t video_transform_to_system(int64_t ts_ns, int fps);

protected:
	int64_t os_gettime_ns();
	int64_t uint64_diff(int64_t ts1, int64_t ts2);
	void reset_timing(timestamp_info &info, int64_t ts_ns, int64_t os_ns);
	int64_t frames_duration_ns(size_t sample_rate, size_t frames_per_channel);
	int64_t get_best_adjust();

private:
	transform_event *callback = nullptr;

	std::recursive_mutex info_lock;
	timestamp_info audio_info;
	timestamp_info video_info;
};
