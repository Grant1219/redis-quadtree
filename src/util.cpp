#include <iostream>

#include <util.hpp>

rectangle::rectangle (int _x, int _y, int _width, int _height)
    : x (_x), y (_y), width (_width), height (_height) {
}

rectangle::rectangle (redisContext* _context, std::string _key) {
    redisReply* reply = (redisReply*)redisCommand (_context, "HVALS %s", _key.c_str () );

    if (reply) {
        if (reply->elements == 4) {
            x = std::stoi (reply->element[0]->str);
            y = std::stoi (reply->element[1]->str);
            width = std::stoi (reply->element[2]->str);
            height = std::stoi (reply->element[3]->str);
        }
    }
}

const bool rectangle::contains (const point& _point) {
    return false;
}

const bool rectangle::contains (const rectangle& _rect) {
    return false;
}

const bool rectangle::intersects (const rectangle& _rect) {
    return false;
}
