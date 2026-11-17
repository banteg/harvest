// Headless driver ownership checks: zig build test-video_lifetime.
#include "video/CVideoGL.h"
#include "video/ColorFormats.h"
#include "ox/IOxDevice.h"
#include "ox/video/IImage.h"
#include "ox/video/IMaterialRenderer.h"
#include <cstdio>
#include <cstring>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "Video lifetime check failed: %s\n", #c); return 1; } } while (0)

class Material : public ox::video::IMaterialRenderer
{
public:
    Material(ox::video::IVideoDriver* driver, int& type) : Driver(driver), Type(type) {}
    ~Material() { Type = Driver->getDriverType(); }
private:
    ox::video::IVideoDriver* Driver;
    int& Type;
};

int main()
{
    const ox::core::CDimension2d<int> size(64, 64);
    ox::video::IVideoDriver* driver = new port::video::CVideoGL(size, 0, 0);

    // The menu writes this image when skyboxRoof.jpg is missing.
    ox::video::IImage* image = driver->createImageFromData(port::ECF_R5G6B5, size, 0);
    CHECK(image && image->getImageDataSizeInBytes() == 64 * 64 * 2);
    void* pixels = image->lock();
    CHECK(pixels);
    std::memset(pixels, 0, image->getImageDataSizeInBytes());
    image->unlock();
    CHECK(image->drop());
    char* storage = new char[64 * 64 * 2];
    image = driver->createImageFromData(port::ECF_R5G6B5, size, storage);
    CHECK(image && image->lock() == storage);
    image->unlock();
    CHECK(image->drop());

    // A registered shader can still call back into the GL driver during material destruction.
    int typeAtDestruction = -1;
    Material* material = new Material(driver, typeAtDestruction);
    CHECK(driver->addMaterialRenderer(material, 0) >= 0);
    CHECK(!material->drop() && driver->drop());
    CHECK(typeAtDestruction == ox::video::EDT_OPENGL);
    return 0;
}
