#pragma once


template <typename T>
inline T Max(T const &a, T const &b)
{
	return (a > b) ? a : b;
}

template <typename T, typename ...ARGS>
inline T Max(T const &a, T const &b, ARGS ...args)
{
	return Max(a, Max(b, args...));
}