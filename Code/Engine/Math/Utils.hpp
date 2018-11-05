#pragma once


template <typename Number>
inline Number Max(const Number& a, const Number& b)
{
	return (a > b) ? a : b;
}

template <typename Number, typename ...ARGS>
inline Number Max(const Number& a, const Number& b, ARGS ...args)
{
	return Max(a, Max(b, args...));
}

template <typename Number>
inline Number Min(const Number& a, const Number& b)
{
	return (a < b) ? a : b;
}

template <typename Number, typename ...ARGS>
inline Number Min(const Number& a, const Number& b, ARGS ...args)
{
	return Min(a, Min(b, args...));
}

template<typename Int>
Int UpperPowerOfTwo(const Int& num)
{
	Int temp = num;
	temp--;
	temp |= temp >> 1;
	temp |= temp >> 2;
	temp |= temp >> 4;
	temp |= temp >> 8;
	temp |= temp >> 16;
	temp++;
	return temp;
}

template<typename Number>
Number ClampWithin(const Number& input, const Number& maxValue, const Number& minValue)
{
	if (input < minValue)
		return minValue;
	if (input > maxValue)
		return maxValue;
	return input;
}