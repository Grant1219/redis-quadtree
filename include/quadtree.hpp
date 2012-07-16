#ifndef REDIS_QUADTREE_QUADTREE_HPP
#define REDIS_QUADTREE_QUADTREE_HPP

#include <string>
#include <list>
#include <hiredis/hiredis.h>
#include <util.hpp>

struct entity {
    union {
        uint32_t id;
        unsigned char bytes[4];
    };
    point pos;
    std::string key;
    std::string ownerKey;
};

struct node {
    std::string key;
    bool subdivided;
    int entities;
    rectangle rect;
};

class quadtree {
    public:
        quadtree (redisContext* _context, const rectangle& _rect);

        void insert_entity (entity& _ent);
        void remove_entity (entity& _ent);
        void relocate_entity (entity& _ent);

        void get_entities (const rectangle& _rect, std::list<entity>& _ents);

    private:
        void subdivide (const node& _node);
        void clean (const node& _node);

        void get_node (const std::string& _nodeKey, node& _node);
        void get_destination_node (const entity& _ent, const node& _currNode, node& _destNode, bool& _stayParent);

        void add_entity (const node& _node, entity& _ent);
        void delete_entity (entity& _ent);
        void move_entity (entity& _ent, const node& _srcNode, const node& _destNode);

        void get_entities (const node& _node, std::list<entity>& _ents);

    private:
        redisContext* context;
        const int maxEntitiesPerNode;
};

#endif
