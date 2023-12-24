#pragma once
#include <mutex>
#include <atomic>
#include <stdint.h>
#include <thread>
#include <chrono>

struct timestamp_info {
	volatile bool timing_set = false;   // ��ǰ��������ת�������Ƿ��Ѿ���ʼ��
	volatile int64_t base_data_ts = 0;  // ��ǰ��������ת�����������ڵ�����ʱ���
	volatile int64_t timing_adjust = 0; // ��ǰ���������������ת������

	volatile int64_t used_timing_adjust = 0; // ��ǰ��ʵ��ʹ�õ�ת���������������Ƶͬʱ��ת����������ȡ��׼����ʱ�����С��ֵ����������Ļ���

	volatile int64_t prev_data_ts = 0; // ��һ֡���ݵ�ʱ���
	volatile int64_t next_data_ts = 0; // ��������һ֡���ݵ�ʱ���
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

	void reset();

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
