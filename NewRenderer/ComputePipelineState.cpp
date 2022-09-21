#include "ComputePipelineState.h"

#include <xxhash.h>

namespace Farlor {

ComputePipelineState::ComputePipelineState() { }

ComputePipelineState::ComputePipelineState(const std::string &filename, bool hasVS, bool hasPS,
      bool hasDS, bool hasHS, bool hasGS, bool hasCS, const std::vector<std::string> &defines)
{
}

void ComputePipelineState::ComputeHash()
{
    /* create a hash state */
    XXH32_state_t *const state = XXH32_createState();
    if (state == NULL) {
        return;
    }

    /* Initialize state with selected seed */
    XXH32_hash_t const seed = 0; /* or any other value */
    if (XXH32_reset(state, seed) == XXH_ERROR) {
        return;
    }

    /* Feed the state with input data, any size, any number of times */
    if (m_ComputeShader.pNumBytes) {
        if (XXH32_update(state, m_ComputeShader.pData, m_ComputeShader.pNumBytes) == XXH_ERROR) {
            return;
        }
    }

    /* Produce the final hash value */
    XXH32_hash_t const hash = XXH32_digest(state);

    /* State could be re-used; but in this example, it is simply freed  */
    XXH32_freeState(state);

    m_hash = hash;
    m_hashCached = true;
}

}
