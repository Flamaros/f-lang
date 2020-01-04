#pragma once

#include <fstd/platform.hpp>

#include <cstdint>

namespace fstd
{
	namespace intrinsic
	{
		inline void swap(uint16_t* a, uint16_t* b)
		{
			// @TODO @SpeedUp use a real intresec here
			wchar_t	temp = *a;
			*a = *b;
			*b = temp;
		}

		inline void divide(uint32_t value, uint32_t divisor, uint32_t* quotien, uint32_t* reminder)
		{
			// @Warning the div instruction can throw an exception with the code DE to indicate an overflow

			// https://c9x.me/x86/html/file_module_x86_id_72.html
			// https://www.aldeid.com/wiki/X86-assembly/Instructions/div
#if defined (FSTD_X86_32)
			__asm
			{
				mov edx, 0			// clear higher part of dividend (here value is only i32 will div instruction take edx and eax for a 64 bit value
				// @todo test to do the clear with command 'xor edx, edx' is it faster?
				mov eax, value		// dividend
				mov ecx, divisor	// divisor
				div ecx

				// copy eax into quotien (but we have to use an other register to be able to use indirect mode)
				mov ecx, quotien
				mov [ecx], eax
				
				// copy edx into reminder (but we have to use an other register to be able to use indirect mode)
				mov ecx, reminder
				mov[ecx], edx
			}
#else	// There is no inline assembly in x64 with Visual Studio

			// It seams that some C/C++ compilers are capable to optimize this code by using intrinsic
			// as both operations are done closely
			*quotien = value / divisor;
			*reminder = value % divisor;
#endif
		}
	}
}
