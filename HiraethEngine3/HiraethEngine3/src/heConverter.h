#ifndef HE_CONVERTER_H
#define HE_CONVERTER_H

#include "heTypes.h"

struct HeTexture;

// converts a d3 instance file from ascii to binary
extern HE_API void heBinaryConvertD3InstanceFile(std::string const& inFile, std::string const& outFile);
// loads a texture, compresses it and then exports that texture into the out file
extern HE_API void heTextureCompress(std::string const& inFile, std::string const& outFile);
// exports that compressed texture into the out file
extern HE_API void heTextureExport(HeTexture* inFile, std::string const& outFile);


#endif
