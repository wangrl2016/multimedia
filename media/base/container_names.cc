//
// Created by wangrl2016 on 2022/7/22.
//

#include <glog/logging.h>
#include "media/base/container_names.h"

namespace mm {
#define TAG(a, b, c, d)                                     \
    ((static_cast<uint32_t>(static_cast<uint8_t>(a)) << 24) |   \
    (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 16) |    \
    (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 8) |     \
    (static_cast<uint32_t>(static_cast<uint8_t>(d))))

#define R_CHECK(x)      \
        do {                \
          if (!(x))         \
            return false;   \
        } while (0)

#define UTF8_BYTE_ORDER_MARK "\xef\xbb\xbf"

    // Helper function to read 2 bytes (16 bits, bit endian) from a buffer.
    static int Read16(const uint8_t* p) {
        return p[0] << 8 | p[1];
    }

    // Helper function to read 3 bytes (24 bits, big endian) from a buffer.
    static uint32_t Read24(const uint8_t* p) {
        return p[0] << 16 | p[1] << 8 | p[2];
    }

    // Helper function to read 4 bytes (32 bits, big endian) from a buffer.
    static uint32_t Read32(const uint8_t* p) {
        return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
    }

    // Helper function to read 4 bytes (32 bits, little endian) from a buffer.
    static uint32_t Read32LE(const uint8_t* p) {
        return p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0];
    }

    // Additional checks for a MOV/QuickTime/MPEG4 container.
    // https://docs.fileformat.com/video/mp4/
    static bool CheckMov(const uint8_t* buffer, int buffer_size) {
        R_CHECK(buffer_size > 8);

        int offset = 0;
        int valid_top_level_boxes = 0;
        while (offset + 8 < buffer_size) {
            uint32_t atom_size = Read32(buffer + offset);
            uint32_t atom_type = Read32(buffer + offset + 4);

            // Only need to check for atoms that are valid at the top level. However,
            // "Boxes with an unrecognized type shall be ignored and skipped." So
            // simply make sure that at least two recognized top level boxes are found.
            switch (atom_type) {
                case TAG('f', 't', 'y', 'p'):
                case TAG('p', 'd', 'i', 'n'):
                case TAG('b', 'l', 'o', 'c'):
                case TAG('m', 'o', 'o', 'v'):
                case TAG('m', 'o', 'o', 'f'):
                case TAG('m', 'f', 'r', 'a'):
                case TAG('m', 'd', 'a', 't'):
                case TAG('f', 'r', 'e', 'e'):
                case TAG('s', 'k', 'i', 'p'):
                case TAG('m', 'e', 't', 'a'):
                case TAG('m', 'e', 'c', 'o'):
                case TAG('s', 't', 'y', 'p'):
                case TAG('s', 'i', 'd', 'x'):
                case TAG('s', 's', 'i', 'x'):
                case TAG('p', 'r', 'f', 't'):
                case TAG('u', 'u', 'i', 'd'):
                case TAG('e', 'm', 's', 'g'):
                    ++valid_top_level_boxes;
                    break;
                default:
                    break;
            }

            if (atom_size == 1) {
                // Indicates that the length is the next 64 bits.
                if (offset + 16 > buffer_size)
                    break;
                if (Read32(buffer + offset + 8) != 0)
                    break;  // Offset is way past buffer size
                atom_size = Read32(buffer + offset + 12);
            }

            if (atom_size == 0 || atom_size > static_cast<size_t>(buffer_size))
                break;  // Indicates the last atom or length too big
            offset += atom_size;
        }
        return valid_top_level_boxes >= 2;
    }

    // Additional checks for a WEBM container.
    static bool CheckWebm(const uint8_t* buffer, int buffer_size) {
        // Reference: http://www.matroska.org/technical/specs/index.html
        R_CHECK(buffer_size > 12);

        return false;
    }

    // Attempt to determine the container type from the buffer provided. This is
    // a simple pass, that uses the first 4 bytes of the buffer as an index to get
    // a rough idea of the container format.
    static MediaContainerName LookupContainerByFirst4(const uint8_t* buffer,
                                                      int buffer_size) {
        // Minimum size that the code expects to exist without checking size.
        if (buffer_size < kMinimumContainerSize)
            return CONTAINER_UNKNOWN;

        uint32_t first4 = Read32(buffer);
        switch (first4) {
            case 0x1a45dfa3:
                if (CheckWebm(buffer, buffer_size))
                    return CONTAINER_WEBM;
                break;
        }

        return CONTAINER_UNKNOWN;
    }

    MediaContainerName DetermineContainer(const uint8_t* buffer,
                                          int buffer_size) {
        DCHECK(buffer);

        // Since MOV/QuickTime/MPEG4 streams are common, check for them first.
        if (CheckMov(buffer, buffer_size))
            return CONTAINER_MOV;

        // Next attempt the simple checks, that typically look at just the
        // first few bytes of the file.
        MediaContainerName result = LookupContainerByFirst4(buffer, buffer_size);

        return CONTAINER_UNKNOWN;
    }
}
