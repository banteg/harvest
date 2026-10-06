// Game globals whose defining units are not recovered yet. Remove each one when its unit joins
// the build.

#include "harvest/entity/CEntityManager.h"

namespace harvest {
namespace entity {

//! Linux .data 0x868e00, initial value 1. Saved in save games.
int g_nextEntityId = 1;

} // end namespace entity
} // end namespace harvest
