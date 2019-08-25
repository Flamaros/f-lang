#include "native_generator.hpp"

#include "f_tokenizer.hpp"

#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "third-party/microsoft_craziness.h"

int main(int ac, char** av)
{


	Find_Result find_visual_studio_result = find_visual_studio_and_windows_sdk();

	free_resources(&find_visual_studio_result);

//    generate_hello_world();
	return 0;
}
