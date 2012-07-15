#include <iostream>

#include <sstream>
#include <quadtree.hpp>

quadtree::quadtree (redisContext* _context, const rectangle& _rect) {
    context = _context;

    // setup the base of the quadtree in redis
    redisAppendCommand (context, "HSET qtree entities qtree:entities");
    redisAppendCommand (context, "HSET qtree subdivided 0");

    redisAppendCommand (context, "HSET qtree rect qtree:rect");
    redisAppendCommand (context, "HSET qtree:rect x %i", _rect.x);
    redisAppendCommand (context, "HSET qtree:rect y %i", _rect.y);
    redisAppendCommand (context, "HSET qtree:rect w %i", _rect.width);
    redisAppendCommand (context, "HSET qtree:rect h %i", _rect.height);

    redisReply* reply;
    for (int n = 0; n < 7; n++) {
        redisGetReply (context, (void**)&reply);
        freeReplyObject (reply);
    }
}

void quadtree::test () {
    // test subdivding
    subdivide ("qtree", rectangle (context, "qtree:rect") );
    subdivide ("qtree:br", rectangle (context, "qtree:br:rect") );
}

void quadtree::subdivide (std::string _level, const rectangle& _rect) {
    std::cout << "Subdividing at level: " << _level << std::endl;

    redisReply* reply;
    const char* level = _level.c_str ();
    const char* quads[4] = {"tl", "tr", "bl", "br"};

    // setup the four quadrants
    for (int i = 0; i < 4; i++) {
        // HSET level quad level:quad
        redisAppendCommand (context, "HSET %s %s %s:%s", level, quads[i], level, quads[i]);
        // HSET level:quad entities level:quad:entities
        redisAppendCommand (context, "HSET %s:%s entities %s:%s:entities", level, quads[i], level, quads[i]);
        // HSET level:quad subdivided 0
        redisAppendCommand (context, "HSET %s:%s subdivided 0", level, quads[i]);
        // HSET level:quad rect level:quad:rect
        redisAppendCommand (context, "HSET %s:%s rect %s:%s:rect", level, quads[i], level, quads[i]);

        for (int n = 0; n < 4; n++) {
            redisGetReply (context, (void**)&reply);
            freeReplyObject (reply);
        }
    }

    // setup the sub rectangles
    point size (_rect.width / 2, _rect.height / 2);
    point mid (_rect.x + size.x, _rect.y + size.y);

    std::cout << "Size of children: (" << size.x << ", " << size.y << ")" << std::endl;
    std::cout << "Midpoint: (" << mid.x << ", " << mid.y << ")" << std::endl;

    // top left rectangle
    rectangle tl_rect (_rect.x, _rect.y, size.x, size.y);
    redisAppendCommand (context, "HSET %s:tl:rect x %i", level, tl_rect.x);
    redisAppendCommand (context, "HSET %s:tl:rect y %i", level, tl_rect.y);
    redisAppendCommand (context, "HSET %s:tl:rect w %i", level, tl_rect.width);
    redisAppendCommand (context, "HSET %s:tl:rect h %i", level, tl_rect.height);

    // top right rectangle
    rectangle tr_rect (mid.x, _rect.y, size.x, size.y);
    redisAppendCommand (context, "HSET %s:tr:rect x %i", level, tr_rect.x);
    redisAppendCommand (context, "HSET %s:tr:rect y %i", level, tr_rect.y);
    redisAppendCommand (context, "HSET %s:tr:rect w %i", level, tr_rect.width);
    redisAppendCommand (context, "HSET %s:tr:rect h %i", level, tr_rect.height);

    // bottom left rectangle
    rectangle bl_rect (_rect.x, mid.y, size.x, size.y);
    redisAppendCommand (context, "HSET %s:bl:rect x %i", level, bl_rect.x);
    redisAppendCommand (context, "HSET %s:bl:rect y %i", level, bl_rect.y);
    redisAppendCommand (context, "HSET %s:bl:rect w %i", level, bl_rect.width);
    redisAppendCommand (context, "HSET %s:bl:rect h %i", level, bl_rect.height);

    // bottom right rectangle
    rectangle br_rect (mid.x, mid.y, size.x, size.y);
    redisAppendCommand (context, "HSET %s:br:rect x %i", level, br_rect.x);
    redisAppendCommand (context, "HSET %s:br:rect y %i", level, br_rect.y);
    redisAppendCommand (context, "HSET %s:br:rect w %i", level, br_rect.width);
    redisAppendCommand (context, "HSET %s:br:rect h %i", level, br_rect.height);

    // set subdivided to true
    redisAppendCommand (context, "HSET %s subdivided 1", level);

    for (int n = 0; n < 17; n++) {
        redisGetReply (context, (void**)&reply);
        freeReplyObject (reply);
    }
}
