#pragma once

namespace fstd
{
	namespace math
	{
		template<class T>
		const T& min(const T& a, const T& b)
		{
			return (b < a) ? b : a;
		}

		template<class T>
		const T& max(const T& a, const T& b)
		{
			return (b > a) ? b : a;
		}
	}
}
