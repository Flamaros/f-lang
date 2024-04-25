#include <tracy/Tracy.hpp>

int main(int ac, char** av)
{
#if defined(TRACY_ENABLE)
	tracy::SetThreadName("Main thread");
#endif
	FrameMark;



	FrameMark;
	return 0;
}
