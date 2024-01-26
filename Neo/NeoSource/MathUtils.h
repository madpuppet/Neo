#pragma once

#include <math.h>

const float PI = 3.14159265358979323846264338327950288f;
const float PIBy2 = 6.28318530717958647692528676655900577f;
const float PIDiv2 = 1.57079632679489661923132169163975144f;
const float PISq = 9.86960440108935861883449099987615114f;

const float FLOAT_MAX = 3.402823466e+38f;
const float ALMOST_ZERO = 0.000001f;
const float ALMOST_ZERO_SQUARED = ALMOST_ZERO * ALMOST_ZERO;
const float EPSILON = 0.0001f;

// Calculate the absolute value of a number
template<typename T>
inline T Abs(T value)
{
	return (value < 0) ? -value : value;
}

// Swap two values
template <typename T>
inline void Swap(T& a, T& b)
{
	T t = a; a = b; b = t;
}

template <typename T>
inline T Min(const T& a, const T& b)
{
	return a < b ? a : b;
}

// Calculate the maximum of two values
template <typename T>
inline T Max(const T& a, const T& b)
{
	return a > b ? a : b;
}

inline bool InRange(int val, int min, int max)
{
	return val >= min && val <= max;
}

// Clamp 'value' to be between 'floor' and 'ceiling'
template <typename T>
inline T Clamp(const T& value, const T& floor, const T& ceiling)
{
	return Max(Min(value, ceiling), floor);
}

// Round off a number
template<typename T>
inline T Round(T value)
{
	if (value > 0.0f)
		return (T)(value + 0.5f);

	return -(T)(0.5f - value);
}

// Calculate the sign of a float
inline float Sign(float f)
{
	unsigned int s = (*reinterpret_cast<int*>(&f) & 0x80000000) | 0x3f800000;
	return (*reinterpret_cast<float*>(&s));
}

// Calculate the sign of an int
inline int Sign(int i)
{
	return (int)((i & 0x80000000) | 0x00000001);
}

// Calculate the square of a number
template<typename T>
inline T Sqr(T x)
{
	return x * x;
}

// Calculate the square root of a number
inline float Sqrt(float f)
{
	return sqrtf(f);
}

// Calculate the absolute (unsigned) value of a float
inline float Fabs(float f)
{
	return fabsf(f);
}

// Test if a value is a power of two
inline bool PowerOfTwo(int n)
{
	return (n & (n - 1)) == 0;
}

// Find the shift equivalent of a multiply (only works for powers of 2)
inline bool GetShiftEquivalent(int n, int& shifter)
{
	shifter = 0;
	if (PowerOfTwo(n))
	{
		while (n > 1)
		{
			n >>= 1;
			shifter++;
		}
		return true;
	}
	return false;
}

// Convert degrees to radians
inline float DegToRad(float fDeg)
{
	return fDeg * (PI / 180.0f);
}

// Convert radians to degrees
inline float RadToDeg(float fRad)
{
	return fRad * ((1.0f / PI) * 180.0f);
}

extern u32 gSyncSeed;           // seed should only be updated in Update() by things that are not view dependant
extern u32 gNonSyncSeed;        // seed does not affect gameplay - used for stuff like particle effects and anything view dependant



// Perform a linear interpolation between 'from' and 'to' (fFraction should be in the range 0.0f - 1.0f)
template<typename T>
inline T Interp(const T& from, const T& to, float fFraction)
{
	return from + ((to - from) * fFraction);
}

inline float InterpRadian(float from, float to, float frac)
{
	if (to - from > PI)
		to -= 2 * PI;
	else if (to - from < -PI)
		to += 2 * PI;
	return Interp(from, to, frac);
}

// Normalise angle to be within a range of 0 to 2PI
inline float NormaliseRadian(float a)
{
	if (a > PI * 64.0f || a < -PI * 64.0f)
		a = 0.0f;

	while (a > PI * 2.0f)
		a -= PI * 2.0f;

	while (a < 0.0f)
		a += PI * 2.0f;

	return a;
}

// Calculate the difference between 2 angles
// This will always be within +/-180 degrees (+/-PI radians)
inline float GetRadianDelta(float a, float b)
{
	return fmodf((a - b) + 5 * PI, 2 * PI) - PI;
}

inline float MoveToRadian(float src, float dst, float maxChange)
{
	return src + Clamp(GetRadianDelta(dst, src), -maxChange, maxChange);
}

inline float SmoothToRadian(float to, float from, float smooth, float maxSpeed)
{
	float diff = to - from;
	if (diff > PI) diff -= PI * 2.0f;
	if (diff < -PI) diff += PI * 2.0f;
	diff = Clamp(diff * smooth, -maxSpeed, maxSpeed);
	return from + diff;
}

// normalise a range of values (min..max) to 0..1
inline float NormaliseRange(float fVal, float fMin, float fMax) { return Clamp((fVal - fMin) / (fMax - fMin), 0.0f, 1.0f); }

// remap a range of values to another range of values
inline float RemapRange(float fVal, float fMin, float fMax, float fNewMin, float fNewMax) { return NormaliseRange(fVal, fMin, fMax) * (fNewMax - fNewMin) + fNewMin; }

// Smooth to a target value using double stepping
template<typename T>
inline T SmoothDS(const T& from, const T& to, float fFactor, float fTimeDelta)
{
	float fTemp = Min(fFactor * fTimeDelta * 0.5f, 1.f);
	T step1 = from + (to - from) * fTemp;
	T step2 = step1 + (to - step1) * fTemp;
	return step2;
}

// Smooth to a target value using critical damping
template <typename T>
inline T SmoothCD(const T& from, const T& to, T& velocity, float fTimeDelta)
{
	float fOmega = 2.0f / fTimeDelta;
	float fX = fOmega * fTimeDelta;
	float fExp = 1.0f / (1.0f + fX + 0.48f * Sqr(fX) + (0.235f * Sqr(fX) * fX));
	T change = from - to;
	T temp = (velocity + fOmega * change) * fTimeDelta;
	velocity = (velocity - fOmega * temp) * fExp;
	return (to + (change + temp) * fExp);
}

template<class T, int MAXNODES>
class EnvelopeRange
{
public:
	EnvelopeRange() : nodes(0) {}

	void Add(float _time, const T& minVal, const T& maxVal)
	{
		time[nodes] = _time;
		min[nodes] = minVal;
		max[nodes] = maxVal;
		nodes++;
	}

	int nodes;
	float time[MAXNODES];
	T min[MAXNODES];
	T max[MAXNODES];
};

template<typename T, int MAXNODES>
class Envelope
{
public:
	Envelope() : nodes(0) {}

	void Reset()
	{
		nodes = 0;
	}

	void Add(float _time, const T& val)
	{
		time[nodes] = _time;
		value[nodes] = val;
		nodes++;
	}

	int nodes;
	float time[MAXNODES];
	T value[MAXNODES];

	void Init(u32& seed, const EnvelopeRange<T, MAXNODES>& def, float scale = 1.0f)
	{
		nodes = def.nodes;
		for (int i = 0; i < nodes; i++)
		{
			time[i] = def.time[i];
			value[i] = Random(seed, def.min[i], def.max[i]) * scale;
		}
	}

	T Evaluate(float _time)
	{
		if (nodes == 1)
			return value[0];
		else
		{
			for (int i = 1; i < nodes; i++)
			{
				if (time[i] > _time)
				{
					float range = time[i] - time[i - 1];
					float relpos = (_time - time[i - 1]) / range;
					return value[i - 1] * (1.0f - relpos) + value[i] * relpos;
				}
			}
			return value[nodes - 1];
		}
	}
};

template<class T>
class MinMaxRange
{
public:
	MinMaxRange() {}
	MinMaxRange(T v) : startMin(v), startMax(v), endMin(v), endMax(v) {}

	T startMin;
	T startMax;
	T endMin;
	T endMax;
};

template<class T>
class MinMax
{
public:
	MinMax() {}
	MinMax(T v) : start(v), end(v) {}

	void Init(u32& seed, const MinMaxRange<T>& def)
	{
		start = ::Random(seed, def.startMin, def.startMax);
		end = ::Random(seed, def.endMin, def.endMax);
	}

	T start;
	T end;
	T Random(u32& seed) { return ::Random(seed, start, end); }
	T Evaluate(float normalisedTime) { return start * (1.0f - normalisedTime) + end * normalisedTime; }
};

inline float SplineInterpolate(float y0, float y1, float y2, float y3, float t)
{
	return 0.5f * ((2.0f * y1) + (-y0 + y2) * t + (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * (t * t) + (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * (t * t * t));
}

template<int MAXNODES>
class EnvelopeF
{
public:
	EnvelopeF() : nodes(0) {}

	void Reset()
	{
		nodes = 0;
	}

	void Add(float _time, const float& val)
	{
		time[nodes] = _time;
		value[nodes] = val;
		nodes++;
	}

	int nodes;
	float time[MAXNODES];
	float value[MAXNODES];

	void Init(u32& seed, const EnvelopeRange<float, MAXNODES>& def, float scale = 1.0f)
	{
		nodes = def.nodes;
		for (int i = 0; i < nodes; i++)
		{
			time[i] = def.time[i];
			value[i] = Random(seed, def.min[i], def.max[i]) * scale;
		}
	}

	float Evaluate(float _time)
	{
		if (nodes == 1)
			return value[0];
		else
		{
			for (int i = 1; i < nodes; i++)
			{
				if (time[i] > _time)
				{
					float range = time[i] - time[i - 1];
					float relpos = (_time - time[i - 1]) / range;
					return value[i - 1] * (1.0f - relpos) + value[i] * relpos;
				}
			}
			return value[nodes - 1];
		}
	}

	float EvaluateSpline(float _time)
	{
		if (nodes == 1)
			return value[0];
		else
		{
			for (int i = 1; i < nodes; i++)
			{
				if (time[i] > _time)
				{
					float range = time[i] - time[i - 1];
					float relpos = (_time - time[i - 1]) / range;
					int i1 = Max(i - 2, 0);
					int i2 = Max(i - 1, 0);
					int i3 = i;
					int i4 = Min(i + 1, nodes - 1);
					return SplineInterpolate(value[i1], value[i2], value[i3], value[i4], relpos);
				}
			}
			return value[nodes - 1];
		}
	}
};

inline float InvLerp(int minValue, int maxValue, int value)
{
	return Clamp((float)(value - minValue) / (float)(maxValue - minValue), 0.0f, 1.0f);
}
inline int Lerp(int minValue, int maxValue, float time)
{
	return Clamp(minValue + (int)((float)(maxValue - minValue) * time), minValue, maxValue);
}

inline float InvLerp(float minValue, float maxValue, float value)
{
	return (value - minValue) / (maxValue - minValue);
}
inline float Lerp(float minValue, float maxValue, float time)
{
	return minValue + (maxValue - minValue) * time;
}

