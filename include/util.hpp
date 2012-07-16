#ifndef REDIS_QUADTREE_UTIL_HPP
#define REDIS_QUADTREE_UTIL_HPP

#include <string>
#include <cstdint>
#include <hiredis/hiredis.h>

class point {
    public:
        point () {}
        point (uint32_t _x, uint32_t _y)
            : x (_x), y (_y) {}

    public:
        union {
            struct {
                uint32_t x, y;
            } __attribute ((__packed__));
            unsigned char bytes[8];
        };
};

class rectangle {
    public:
        rectangle () {}
        rectangle (uint32_t _x, uint32_t _y, uint32_t _width, uint32_t _height);
        rectangle (redisContext* _context, std::string _key);

        const bool contains (const point& _point);
        const bool contains (const rectangle& _rect);

        const bool intersects (const rectangle& _rect);

    public:
        union {
            struct {
                uint32_t x, y;
                uint32_t x2, y2;
                uint32_t width, height;
            } __attribute__ ((__packed__));
            unsigned char bytes[24];
        };
};

#endif
