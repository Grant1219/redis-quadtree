#ifndef REDIS_QUADTREE_UTIL_HPP
#define REDIS_QUADTREE_UTIL_HPP

#include <string>
#include <hiredis/hiredis.h>

class point {
    public:
        point (int _x, int _y)
            : x (_x), y (_y) {}

    public:
        int x;
        int y;
};

class rectangle {
    public:
        rectangle (int _x, int _y, int _width, int _height);
        rectangle (redisContext* _context, std::string _key);

        const bool contains (const point& _point);
        const bool contains (const rectangle& _rect);

        const bool intersects (const rectangle& _rect);

    public:
        int x, y, width, height;
};

#endif
