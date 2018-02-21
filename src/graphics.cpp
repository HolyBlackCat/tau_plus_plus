#include "program.h" // For DebugAssert()

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#define STBI_ASSERT(expr) DebugAssert("STB Image assertion.", expr)
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ASSERT(expr) DebugAssert("STB Image Write assertion.", expr)
#include <stb_image_write.h>

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_ASSERT(expr) DebugAssert("STB Rectangle Packing assertion.", expr)
#include <stb_rect_pack.h>
