#ifndef AUTONOMOUS_ACORE_COMPAT_H
#define AUTONOMOUS_ACORE_COMPAT_H

#include "QueryResult.h"
#include "Log.h"
#include <string>

#ifndef TC_LOG_INFO
#define TC_LOG_INFO LOG_INFO
#endif
#ifndef TC_LOG_DEBUG
#define TC_LOG_DEBUG LOG_DEBUG
#endif

// Compatibility for the older Autonomous AI database API.
#define PQuery Query
#define PExecute Execute
#define GetUInt8 Get<uint8>
#define GetUInt16 Get<uint16>
#define GetUInt32 Get<uint32>
#define GetUInt64 Get<uint64>
#define GetInt8 Get<int8>
#define GetInt16 Get<int16>
#define GetInt32 Get<int32>
#define GetInt64 Get<int64>
#define GetFloat Get<float>
#define GetDouble Get<double>
#define GetString Get<std::string>

#ifndef MAX_GROUP_SIZE
#define MAX_GROUP_SIZE MAXGROUPSIZE
#endif

#endif
