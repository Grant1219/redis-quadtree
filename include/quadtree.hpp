#ifndef REDIS_QUADTREE_QUADTREE_HPP
#define REDIS_QUADTREE_QUADTREE_HPP

#include <string>
#include <vector>
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
    std::string parentKey;
    bool subdivided;
    uint32_t entities;
    rectangle rect;
};

class quadtree {
    public:
        quadtree (redisContext* _context, const rectangle& _rect);

        void get_entity (uint32_t _id, entity& _ent);

        void insert_entity (entity& _ent);
        void remove_entity (entity& _ent);
        void relocate_entity (entity& _ent);

        void get_entities (const rectangle& _rect, std::vector<entity>& _ents);

    private:
        bool subdivide (const node& _node);
        void clean (node& _node);

        void get_node (const std::string& _nodeKey, node& _node);
        void get_destination_node (const entity& _ent, const node& _currNode, node& _destNode, bool& _stayParent);

        void add_entity (const node& _node, entity& _ent);
        void update_entity (const entity& _ent);
        void delete_entity (entity& _ent);
        void move_entity (entity& _ent, const node& _srcNode, const node& _destNode);
        void reinsert_entity (entity& _ent, const node& _ownerNode, node& _destNode);
        void relocate_entity (entity& _ent, node& _ownerNode, node& _currNode, node& _destNode);

        void get_node_entities (const node& _node, std::vector<entity>& _ents);
        void get_entities (const node& _node, const rectangle& _rect, std::vector<entity>& _ents);
        void get_all_entities (const node& _node, std::vector<entity>& _ents);

        void delete_empty_subnodes (const std::string& _nodeKey, bool& empty);
        void delete_subnodes (const std::string& _nodeKey);

    private:
        redisContext* context;
        const uint32_t maxEntitiesPerNode;
        const uint32_t minNodeSize;

        std::vector<entity> tempEnts;
};

#endif
