#pragma once

#include "BitmapFont.h"

#if PROFILING_ENABLED
class Profiler : public Module<Profiler>
{
	int m_currentFrame = 0;

	struct ProfilePoint
	{
		u64 start;
		u64 end;
		u32 color;
		string label;
	};
	struct ThreadProfile
	{
		string name;
		int guid;
		vector<ProfilePoint> points;
	};
	struct FrameInfo
	{
		hashtable<u64, ThreadProfile*> threads;
		hashtable<u32, ProfilePoint> gpuPoints;
		u64 start = 0;
		u64 gpuStart = 0;
	};
	FrameInfo m_frames[MAX_FRAMES_IN_FLIGHT];
	BitmapFontRef m_font;
	MaterialRef m_white;
	Mutex m_lock;

public:
	Profiler();
	void FrameSync();
	void Render();

	void AddProfileCPU(u64 thread, u64 start, u64 end, const string& label);
	void AddProfileGPU_Start(u32 uid, const string& label);
	void AddProfileGPU_End(u32 uid);
};

struct ProfilerScopeCPU
{
	u64 start;
	string label;

	ProfilerScopeCPU(const string& str) : label(str)
	{
		start = NeoTimeNowU64;
	};

	~ProfilerScopeCPU()
	{
		u64 threadID = Thread::GetCurrentThreadGUID();
		u64 end = NeoTimeNowU64;
		Profiler::Instance().AddProfileCPU(threadID, start, end, label);
	}
};

struct ProfilerScopeGPU
{
	static u32 s_uniqueID;
	u64 start;
	string label;
	u32 uid = s_uniqueID++;

	ProfilerScopeGPU(const string& str) : label(str)
	{
		start = NeoTimeNowU64;
		Profiler::Instance().AddProfileGPU_Start(uid, label);
	};

	~ProfilerScopeGPU()
	{
		u64 threadID = Thread::GetCurrentThreadGUID();
		u64 end = NeoTimeNowU64;
		Profiler::Instance().AddProfileCPU(threadID, start, end, label);
		Profiler::Instance().AddProfileGPU_End(uid);
	}
};

#define PROFILE_CPU(str) ProfilerScopeCPU __tempProfCPU##__LINE__(str)
#define PROFILE_GPU(str) ProfilerScopeGPU __tempProfGPU##__LINE__(str)
#define PROFILE_FRAME_SYNC() Profiler::Instance().FrameSync();
#define PROFILE_RENDER() Profiler::Instance().Render();

#else
#define PROFILE_FRAME_SYNC() {}
#define PROFILE_RENDER() {}
#define PROFILE_CPU(str) {}
#define PROFILE_GPU(str) {}
#endif
