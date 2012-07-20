#include <util.hpp>

rectangle::rectangle (uint32_t _x, uint32_t _y, uint32_t _width, uint32_t _height)
    : x (_x), y (_y), x2 (_x + _width), y2 (_y + _height), width (_width), height (_height) {
}

rectangle::rectangle (redisContext* _context, std::string _key) {
    redisReply* reply = (redisReply*)redisCommand (_context, "HVALS %s", _key.c_str () );

    if (reply) {
        if (reply->elements == 4) {
            x = std::stoi (reply->element[0]->str);
            y = std::stoi (reply->element[1]->str);
            width = std::stoi (reply->element[2]->str);
            height = std::stoi (reply->element[3]->str);
            x2 = x + width;
            y2 = y + height;
        }
    }
}

const bool rectangle::contains (const point& _point) const {
    return (_point.x > x && _point.y > y && _point.x < x2 && _point.y < y2);
}

const bool rectangle::contains (const rectangle& _rect) const {
    return (_rect.x > x && _rect.y > y && _rect.x2 < x2 && _rect.y2 < y2);
}

const bool rectangle::intersects (const rectangle& _rect) const {
    return !(_rect.x > x2 || _rect.x2 < x || _rect.y > y2 || _rect.y2 < y);
}
