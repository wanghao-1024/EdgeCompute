#pragma once

#include "common/SystemHead.h"

const std::string kEdgeFSMagic = "edgefs";

const uint32_t kInvalidChunkid = -1;

const std::string kDataFileName = "edgefs.data";

const std::string kIndexFileName = "edgefs.idx";

const std::string kLogFileName = "edgefs.log";

const uint32_t kMinChunkSize = 1024 * 1024;
//const uint32_t kMinChunkSize = 1024;

const uint32_t kMaxChunkSize = 128 * 1024 * 1024;

// 磁盘操作比较按照4K对齐
const uint32_t kDiskRWAlignSize = 4096;