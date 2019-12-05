#include "../fstd/memory/string.hpp"

#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace tests
{
	TEST_CLASS(string_tests)
	{
	public:
		
		TEST_METHOD(string_litteral)
		{
			fstd::string	string;

			assign(string, "test");

			Assert::AreEqual("test", (char*)string.data);
		}
	};
}
