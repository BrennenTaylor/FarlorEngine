#include <ImfArray.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <ImfStringAttribute.h>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

bool isPowerOfTwo(int n) { return (ceil(log2(n)) == floor(log2(n))); }

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cout << "Call as: " << argv[0] << " filepath base_mip_dimension" << std::endl;
    }

    const std::filesystem::path baseImagePath = argv[1];
    const uint64_t baseMipDim = std::stoi(argv[2]);

    std::filesystem::path filename = baseImagePath.stem();
    std::filesystem::path extension = baseImagePath.extension();
    std::filesystem::path directory = baseImagePath.parent_path();

    std::ostringstream filename_mip_0;
    filename_mip_0 << filename.string() << "_" << baseMipDim << "_" << baseMipDim
                   << extension.string();

    std::filesystem::path filepath_mip_0 = directory / filename_mip_0.str();

    Imf::RgbaInputFile base_mip_file(filepath_mip_0.string().c_str());
    Imath::Box2i dw = base_mip_file.dataWindow();

    Imf::Array2D<Imf::Rgba> mip_level_0;
    uint32_t width = dw.max.x - dw.min.x + 1;
    uint32_t height = dw.max.y - dw.min.y + 1;

    if (!isPowerOfTwo(width) || !isPowerOfTwo(height) || (width != height))
    {
        std::cout << "Cannot work with this dimension, requires square, power of 2 image" << std::endl;
    }
    if (width == 1) {
        std::cout << "Smallest image possible, no mips needed" << std::endl;
    }
    std::cout << "Width, Height: (" << width << ", " << height << ")" << std::endl;
    const uint32_t numMips = std::log2(width);
    std::cout << "Num mips: " << numMips << std::endl;

    mip_level_0.resizeErase(height, width);

    if (base_mip_file.header().findTypedAttribute<Imf::StringAttribute>("comments")) {
        std::cout << "mip_level_0 comments: "
                  << base_mip_file.header()
                           .findTypedAttribute<Imf::StringAttribute>("comments")
                           ->value()
                  << std::endl;
    } else {
        std::cout << "No comments" << std::endl;
    }

    base_mip_file.setFrameBuffer(
          &mip_level_0[0][0] - dw.min.x - dw.min.y * width, 1, width);
    base_mip_file.readPixels(dw.min.y, dw.max.y);

    auto GenerateMip
          = [](const Imf::Array2D<Imf::Rgba> &mipLevel, Imf::Array2D<Imf::Rgba> &newMip) {
                newMip.resizeErase(mipLevel.height() / 2, mipLevel.width() / 2);
                for (uint32_t newMipX = 0; newMipX < newMip.width(); newMipX++) {
                    for (uint32_t newMipY = 0; newMipY < newMip.height(); newMipY++) {
                        float r = mipLevel[newMipY * 2][newMipX * 2].r
                              + mipLevel[newMipY * 2 + 1][newMipX * 2].r
                              + mipLevel[newMipY * 2][newMipX * 2 + 1].r
                              + mipLevel[newMipY * 2 + 1][newMipX * 2 + 1].r;
                        r *= 0.25;

                        float g = mipLevel[newMipY * 2][newMipX * 2].g
                              + mipLevel[newMipY * 2 + 1][newMipX * 2].g
                              + mipLevel[newMipY * 2][newMipX * 2 + 1].g
                              + mipLevel[newMipY * 2 + 1][newMipX * 2 + 1].g;
                        g *= 0.25;

                        float b = mipLevel[newMipY * 2][newMipX * 2].b
                              + mipLevel[newMipY * 2 + 1][newMipX * 2].b
                              + mipLevel[newMipY * 2][newMipX * 2 + 1].b
                              + mipLevel[newMipY * 2 + 1][newMipX * 2 + 1].b;
                        b *= 0.25;

                        newMip[newMipY][newMipX].r = r;
                        newMip[newMipY][newMipX].g = g;
                        newMip[newMipY][newMipX].b = b;
                        newMip[newMipY][newMipX].a = 1.0f;
                    }
                }
            };

    std::vector<Imf::Array2D<Imf::Rgba>> mipChain(numMips);
    GenerateMip(mip_level_0, mipChain[0]);

    for (uint32_t mipLevel = 1; mipLevel < numMips; mipLevel++) {
        GenerateMip(mipChain[mipLevel - 1], mipChain[mipLevel]);
    }

    uint64_t current_dim = width;

    for (uint32_t mipLevel = 1; mipLevel <= mipChain.size(); mipLevel++) {
        current_dim /= 2;
        std::ostringstream filename_current_mip;
        filename_current_mip << filename.string() << "_" << current_dim << "_" << current_dim
                       << extension.string();

        std::filesystem::path filepath_current_mip = directory / filename_current_mip.str();
        Imf::RgbaOutputFile file(
              filepath_current_mip.string().c_str(), current_dim, current_dim, Imf::WRITE_RGBA);  // 1

        file.setFrameBuffer(&mipChain[mipLevel - 1][0][0], 1, current_dim);  // 2

        file.writePixels(current_dim);  // 3
    }

    return 0;
}