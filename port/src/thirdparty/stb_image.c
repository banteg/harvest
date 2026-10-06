// stb_image's implementation: JPEG and TGA for the game's textures, PNG for the port's own use.
// Images are decoded from memory (the game reads files through its own file system).

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_TGA
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include <stb_image.h>
