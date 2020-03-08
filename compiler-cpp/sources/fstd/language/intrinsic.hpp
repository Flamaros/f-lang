#pragma once

#include <fstd/platform.hpp>

#include <fstd/language/types.hpp>

namespace fstd
{
	namespace intrinsic
	{
		inline void swap(uint16_t* a, uint16_t* b)
		{
			// https://c9x.me/x86/html/file_module_x86_id_328.html
			// xchg is certainly not faster as the C implementation
			// xchg does a lock (memory barrier)
			uint16_t	temp = *a;
			*a = *b;
			*b = temp;
		}

		inline void swap(uint8_t* a, uint8_t* b)
		{
			// https://c9x.me/x86/html/file_module_x86_id_328.html
			// xchg is certainly not faster as the C implementation
			// xchg does a lock (memory barrier)
			uint8_t	temp = *a;
			*a = *b;
			*b = temp;
		}

		inline void divide(uint32_t value, uint32_t divisor, uint32_t* quotien, uint32_t* reminder)
		{
			// @Warning the div instruction can throw an exception with the code DE to indicate an overflow

			// https://c9x.me/x86/html/file_module_x86_id_72.html
			// https://www.aldeid.com/wiki/X86-assembly/Instructions/div
//#if defined (FSTD_X86_32)
//			__asm
//			{
//				mov edx, 0			// clear higher part of dividend (here value is only i32 will div instruction take edx and eax for a 64 bit value
//				// @todo test to do the clear with command 'xor edx, edx' is it faster?
//				mov eax, value		// dividend
//				mov ecx, divisor	// divisor
//				div ecx
//
//				// copy eax into quotien (but we have to use an other register to be able to use indirect mode)
//				mov ecx, quotien
//				mov [ecx], eax
//				
//				// copy edx into reminder (but we have to use an other register to be able to use indirect mode)
//				mov ecx, reminder
//				mov[ecx], edx
//
//				// Here is the code generated in release by Visual Studio when divisor is set to 100
//				// I suspect that with the combination of the inlining the compiler is able to generate
//				// a better code without a division actually
//				//mov         eax, 51EB851Fh
//				//mov         ecx, esi
//				//mul         eax, esi
//				//mov         esi, edx
//				//shr         esi, 5
//				//imul        eax, esi, 64h
//			}
//#else	// There is no inline assembly in x64 with Visual Studio

			// It seams that some C/C++ compilers are capable to optimize this code by using intrinsic
			// as both operations are done closely
			*quotien = value / divisor;
			*reminder = value % divisor;
//#endif
		}

		inline void divide(uint64_t value, uint64_t divisor, uint64_t* quotien, uint64_t* reminder)
		{
			*quotien = value / divisor;
			*reminder = value % divisor;
		}
	}
}
