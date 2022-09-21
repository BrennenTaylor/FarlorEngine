#include "GraphicsPipelineState.h"

#include <xxhash.h>

#include <d3dcompiler.h>

#include "../Util/StringUtil.h"
#include "../Util/Logger.h"

#include <iostream>
#include <memory>

// TODO: Make sure we can capture all shader compile warnings and force them to the log

namespace Farlor {

GraphicsPipelineState::GraphicsPipelineState() { }

GraphicsPipelineState::GraphicsPipelineState(const std::string &filename, bool hasVS, bool hasPS,
      bool hasDS, bool hasHS, bool hasGS, bool hasCS, const std::vector<std::string> &defines)
{
}
}