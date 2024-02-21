#include "Neo.h"
#include "ImmDynamicRenderer.h"

DECLARE_MODULE(Profiler, NeoModuleInitPri_Profiler, NeoModulePri_None, NeoModulePri_None);

u32 ProfilerScopeGPU::s_uniqueID = 0;

u32 s_colors[16] =
{
	0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00, 0xff00ffff, 0xffff00ff, 0xffffffff, 0xffff4004,
	0xff400000, 0xff004000, 0xff000040, 0xff404000, 0xff004040, 0xff400040, 0xff404040, 0xff4040ff
};

Profiler::Profiler()
{
	m_font.Create("c64");
	m_white.Create("white_ortho");
}

void Profiler::FrameSync()
{
	ScopedMutexLock lock(m_lock);

	// end previous frame...
	auto& oldFrame = m_frames[m_currentFrame];
	u64* resultBuffer;
	int resultCount;
	GIL::Instance().GetGpuTimeQueryResults(resultBuffer, resultCount);
	oldFrame.gpuStart = resultBuffer[0];
	for (auto& point : oldFrame.gpuPoints)
	{
		point.second.start = resultBuffer[point.second.start];
		point.second.end = resultBuffer[point.second.end];
	}

	// start next frame
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	auto& frame = m_frames[m_currentFrame];
	for (auto& thread : frame.threads)
		thread.second->points.clear();
	frame.start = NeoTimeNowU64;
	frame.gpuPoints.clear();
}

void Profiler::Render()
{
	int drawFrame = (m_currentFrame + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;
	auto& frame = m_frames[drawFrame];

	if (frame.start == 0 || !m_white->IsLoaded() || frame.gpuPoints.empty() || frame.threads.empty())
		return;

	const float baseY = 10.0f;
	const float left = 10.0f;
	const float right = 1280.0f - 10.0f;
	const float lineHeight = 20.0f;
	const u32 lineCol = 0xffffffff;

	auto& gil = GIL::Instance();
	auto& idr = ImmDynamicRenderer::Instance();

	// draw row lines
	float y = baseY;
	idr.BeginRender();
	idr.StartPrimitive(PrimType_LineList);
	idr.UseMaterial(m_white);
	idr.AddVert(vec3(left, y, 0), vec2(0,0), lineCol);
	idr.AddVert(vec3(right, y, 0), vec2(1, 1), lineCol);
	y += lineHeight;

	for (auto thread : frame.threads)
	{
		idr.AddVert(vec3(left, y, 0), vec2(0, 0), lineCol);
		idr.AddVert(vec3(right, y, 0), vec2(1, 1), lineCol);
		y += lineHeight;
	}

	idr.AddVert(vec3(left, y, 0), vec2(0, 0), lineCol);
	idr.AddVert(vec3(right, y, 0), vec2(1, 1), lineCol);

	// some dividers
	float div25 = (right - left) * 0.25 + left;
	float div50 = (right - left) * 0.5 + left;
	float div75 = (right - left) * 0.75 + left;
	for (int i = 0; i <= 4; i++)
	{
		float x = (right - left) * (i / 4.0f) + left;
		idr.AddVert(vec3(x, baseY, 0), vec2(0, 0), 0x80808080);
		idr.AddVert(vec3(x, y, 0), vec2(1, 1), 0x80808080);
	}

	// TODO: thread bars

	idr.EndPrimitive();

#define GPU_SCALE 4.0/(60.0*1000.0*1000.0)*(right-left)
#define GPU_TIME_TO_POS(t) (float)((double)(t - frame.gpuStart) * GPU_SCALE + left)

#define CPU_SCALE 4.0/(60.0*1000.0*1000.0)*(right-left)
#define CPU_TIME_TO_POS(t) (float)((double)(t - frame.start) * CPU_SCALE + left)

	// draw bars
	idr.StartPrimitive(PrimType_TriangleList);
	idr.UseMaterial(m_white);

	// gpu bars
	y = baseY;
	for (auto& it : frame.gpuPoints)
	{
		auto& point = it.second;
		float x1 = GPU_TIME_TO_POS(point.start);
		float x2 = GPU_TIME_TO_POS(point.end);
		float y1 = y + lineHeight * 0.1f;
		float y2 = y + lineHeight * 0.9f;
		u32 col = point.color;
		idr.AddVert({ x1,y1,0 }, { 0,0 }, col);
		idr.AddVert({ x2,y1,0 }, { 1,0 }, col);
		idr.AddVert({ x1,y2,0 }, { 0,1 }, col);

		idr.AddVert({ x2,y2,0 }, { 1,1 }, col);
		idr.AddVert({ x1,y2,0 }, { 0,1 }, col);
		idr.AddVert({ x2,y1,0 }, { 1,0 }, col);
	}
	y += lineHeight;

	for (auto& thread : frame.threads)
	{
		for (auto& point : thread.second->points)
		{
			float x1 = CPU_TIME_TO_POS(point.start);
			float x2 = CPU_TIME_TO_POS(point.end);
			float y1 = y + lineHeight * 0.1f;
			float y2 = y + lineHeight * 0.9f;
			u32 col = point.color;
			idr.AddVert({ x1,y1,0 }, { 0,0 }, col);
			idr.AddVert({ x2,y1,0 }, { 1,0 }, col);
			idr.AddVert({ x1,y2,0 }, { 0,1 }, col);

			idr.AddVert({ x2,y2,0 }, { 1,1 }, col);
			idr.AddVert({ x1,y2,0 }, { 0,1 }, col);
			idr.AddVert({ x2,y1,0 }, { 1,0 }, col);
		}
		y += lineHeight;
	}

	idr.EndPrimitive();
	idr.EndRender();

//	if (m_font->IsLoaded())
//	{
//		int fps = (int)(1.0f / NeoTimeDelta);
//		int ms = (int)(1000.0f * NeoTimeDelta);
//		m_font->RenderText(STR("{}ms FPS {}", ms, fps), rect(20.0f, 680.0f, 500.0f, 20.0f), 0.0f, Alignment_CenterLeft, { 2.0f,2.0f }, { 1,1,1,1 }, -3.0f);
//	}
}

void Profiler::AddProfileCPU(u64 thread, u64 start, u64 end, const string& label)
{
	u32 color = s_colors[StringHash64(label) & 15];
	auto& frame = m_frames[m_currentFrame];

	ScopedMutexLock lock(m_lock);
	auto it = frame.threads.find(thread);
	ThreadProfile* tp = nullptr;
	if (it == frame.threads.end())
	{
		tp = new ThreadProfile;
		frame.threads[thread] = tp;
	}
	else
	{
		tp = it->second;
	}
	tp->points.emplace_back(start, end, color, label);
}

void Profiler::AddProfileGPU_Start(u32 uid, const string& label)
{
	u32 color = s_colors[StringHash64(label) & 15];
	ScopedMutexLock lock(m_lock);
	auto& frame = m_frames[m_currentFrame];
	ProfilePoint point;
	point.start = GIL::Instance().AddGpuTimeQuery();
	point.label = label;
	point.color = color;
	frame.gpuPoints[uid] = point;
}

void Profiler::AddProfileGPU_End(u32 uid)
{
	ScopedMutexLock lock(m_lock);
	auto& frame = m_frames[m_currentFrame];
	auto& point = frame.gpuPoints[uid];
	point.end = GIL::Instance().AddGpuTimeQuery();
}

