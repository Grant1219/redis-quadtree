#ifndef REDIS_QUADTREE_QUADTREE_HPP
#define REDIS_QUADTREE_QUADTREE_HPP

#include <string>
#include <hiredis/hiredis.h>
#include <util.hpp>

class quadtree {
    public:
        quadtree (redisContext* _context, const rectangle& _rect);

        void test ();

    private:
        void subdivide (std::string _level, const rectangle& _rect);

    private:
        redisContext* context;
};

#endif
