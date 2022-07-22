//
// Created by wangrl2016 on 2022/7/22.
//

#include <glog/logging.h>
#include "media/base/container_names.h"

namespace mm {
    #define R_CHECK(x)      \
        do {                \
          if (!(x))         \
            return false;   \
        } while (0)

    // Additional checks for a MOV/QuickTime/MPEG4 container.
    static bool CheckMov(const uint8_t* buffer, int buffer_size) {
        R_CHECK(buffer_size > 8);


    }

    MediaContainerName DetermineContainer(const uint8_t* buffer,
                                          int buffer_size) {
        DCHECK(buffer);

        // Since MOV/QuickTime/MPEG4 streams are common, check for them first.
        if (CheckMov(buffer, buffer_size))
            return CONTAINER_MOV;

        return CONTAINER_UNKNOWN;
    }
}
