#include <iostream>
#include <cstring>

#include <sstream>
#include <quadtree.hpp>

quadtree::quadtree (redisContext* _context, const rectangle& _rect)
    : maxEntitiesPerNode (10), minNodeSize (8) {
    context = _context;

    // check if the root already exists
    redisReply* reply;
    reply = (redisReply*)redisCommand (context, "EXISTS root");

    if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 0) {
        freeReplyObject (reply);
        //std::cout << "Creating quadtree with size (" << _rect.x << ", " << _rect.y << ", " << _rect.width << ", " << _rect.height << ")" << std::endl;
        // setup the base of the quadtree in redis
        redisAppendCommand (context, "HSET root subdivided 0");
        redisAppendCommand (context, "HSET root entities 0");

        redisAppendCommand (context, "HSET root:rect x %i", _rect.x);
        redisAppendCommand (context, "HSET root:rect y %i", _rect.y);
        redisAppendCommand (context, "HSET root:rect w %i", _rect.width);
        redisAppendCommand (context, "HSET root:rect h %i", _rect.height);

        for (int n = 0; n < 6; n++) {
            redisGetReply (context, (void**)&reply);
            freeReplyObject (reply);
        }
    }
}

void quadtree::get_entity (uint32_t _id, entity& _ent) {
    _ent.key = "entities:" + std::to_string (_id);

    redisReply* reply;
    reply = (redisReply*)redisCommand (context, "EXISTS %s", _ent.key.c_str () );

    if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1) {
        freeReplyObject (reply);

        redisAppendCommand (context, "HGET %s x", _ent.key.c_str () );
        redisAppendCommand (context, "HGET %s y", _ent.key.c_str () );
        redisAppendCommand (context, "HGET %s owner", _ent.key.c_str () );

        redisGetReply (context, (void**)&reply);
        if (reply->type == REDIS_REPLY_STRING)
            _ent.pos.x = std::stoi (reply->str);
        freeReplyObject (reply);
        redisGetReply (context, (void**)&reply);
        if (reply->type == REDIS_REPLY_STRING)
            _ent.pos.y = std::stoi (reply->str);
        freeReplyObject (reply);
        redisGetReply (context, (void**)&reply);
        if (reply->type == REDIS_REPLY_STRING)
            _ent.ownerKey = reply->str;
        freeReplyObject (reply);
    }

    std::cout << "Got entity (" << _ent.key << ", " << _ent.ownerKey << ")" << std::endl;
}

void quadtree::insert_entity (entity& _ent) {
    bool done = false;
    node currNode, destNode;
    bool stayParent = false;

    get_node ("root", currNode);

    do {
        // check to see if the entity will fit in this node
        if (currNode.rect.contains (_ent.pos) ) {
            if (!currNode.subdivided && currNode.entities + 1 <= maxEntitiesPerNode) {
                // there's still room here so add it
                add_entity (currNode, _ent);
                done = true;
            }
            else {
                // subdivide if needed
                if (!currNode.subdivided) {
                    if (!subdivide (currNode) ) {
                        // this is as small as the nodes can get, add the entity here
                        add_entity (currNode, _ent);
                        done = true;

                        //std::cout << "Minimum node size reached, cannot subdivide " << currNode.key << std::endl;
                    }
                }

                // figure out which subnode the entity should move into
                get_destination_node (_ent, currNode, destNode, stayParent);

                if (stayParent) {
                    // if this entity won't fit in the subnodes it stays here
                    add_entity (currNode, _ent);
                    done = true;
                }
                else {
                    // change the level down one
                    // the loop will repeat and try to insert into the subnode
                    get_node (destNode.key, currNode);
                }
            }
        }
    } while (!done);
}

void quadtree::remove_entity (entity& _ent) {
    std::string nodeKey = _ent.ownerKey;
    delete_entity (_ent);

    node ownerNode;
    get_node (nodeKey, ownerNode);
    // if this node only has this entity in it (or less for whatever reason), then try to clean it
    if (ownerNode.entities == 0) {
        clean (ownerNode);
    }
}

void quadtree::relocate_entity (entity& _ent) {
    node ownerNode, destNode;
    get_node (_ent.ownerKey, ownerNode);
    node currNode = ownerNode;
    // change its position in the tree if needed
    relocate_entity (_ent, ownerNode, currNode, destNode);
    // update it's info in redis
    update_entity (_ent);
}

void quadtree::get_entities (const rectangle& _rect, std::vector<entity>& _ents) {
    node rootNode;
    get_node ("root", rootNode);
    get_entities (rootNode, _rect, _ents);
}

bool quadtree::subdivide (const node& _node) {
    std::cout << "Subdividing at node: " << _node.key << std::endl;

    // don't subdivide if we've reached the minimum
    if (_node.rect.width / 2 < minNodeSize || _node.rect.height / 2 < minNodeSize)
        return false;

    redisReply* reply;
    const char* nodeKey = _node.key.c_str ();
    const char* quads[4] = {"tl", "tr", "bl", "br"};

    // setup the four quadrants
    for (int i = 0; i < 4; i++) {
        redisAppendCommand (context, "HSET %s:%s subdivided 0", nodeKey, quads[i]);
        redisAppendCommand (context, "HSET %s:%s entities 0", nodeKey, quads[i]);

        for (int n = 0; n < 2; n++) {
            redisGetReply (context, (void**)&reply);
            freeReplyObject (reply);
        }
    }

    // setup the sub rectangles
    point size (_node.rect.width / 2, _node.rect.height / 2);
    point mid (_node.rect.x + size.x, _node.rect.y + size.y);

    //std::cout << "Size of children: (" << size.x << ", " << size.y << ")" << std::endl;
    //std::cout << "Midpoint: (" << mid.x << ", " << mid.y << ")" << std::endl;

    // top left rectangle
    rectangle tlRect (_node.rect.x, _node.rect.y, size.x, size.y);
    redisAppendCommand (context, "HSET %s:tl:rect x %i", nodeKey, tlRect.x);
    redisAppendCommand (context, "HSET %s:tl:rect y %i", nodeKey, tlRect.y);
    redisAppendCommand (context, "HSET %s:tl:rect w %i", nodeKey, tlRect.width);
    redisAppendCommand (context, "HSET %s:tl:rect h %i", nodeKey, tlRect.height);

    // top right rectangle
    rectangle trRect (mid.x, _node.rect.y, size.x, size.y);
    redisAppendCommand (context, "HSET %s:tr:rect x %i", nodeKey, trRect.x);
    redisAppendCommand (context, "HSET %s:tr:rect y %i", nodeKey, trRect.y);
    redisAppendCommand (context, "HSET %s:tr:rect w %i", nodeKey, trRect.width);
    redisAppendCommand (context, "HSET %s:tr:rect h %i", nodeKey, trRect.height);

    // bottom left rectangle
    rectangle blRect (_node.rect.x, mid.y, size.x, size.y);
    redisAppendCommand (context, "HSET %s:bl:rect x %i", nodeKey, blRect.x);
    redisAppendCommand (context, "HSET %s:bl:rect y %i", nodeKey, blRect.y);
    redisAppendCommand (context, "HSET %s:bl:rect w %i", nodeKey, blRect.width);
    redisAppendCommand (context, "HSET %s:bl:rect h %i", nodeKey, blRect.height);

    // bottom right rectangle
    rectangle brRect (mid.x, mid.y, size.x, size.y);
    redisAppendCommand (context, "HSET %s:br:rect x %i", nodeKey, brRect.x);
    redisAppendCommand (context, "HSET %s:br:rect y %i", nodeKey, brRect.y);
    redisAppendCommand (context, "HSET %s:br:rect w %i", nodeKey, brRect.width);
    redisAppendCommand (context, "HSET %s:br:rect h %i", nodeKey, brRect.height);

    // set subdivided to true
    redisAppendCommand (context, "HSET %s subdivided 1", nodeKey);

    for (int n = 0; n < 17; n++) {
        redisGetReply (context, (void**)&reply);
        freeReplyObject (reply);
    }

    node destNode;
    bool stayParent;
    std::vector<entity> ents;
    get_node_entities (_node, ents);

    std::cout << "Moving " << ents.size () << " entities down to subnodes" << std::endl;

    if (ents.size () > 0) {
        // move all the entities in this node down if possible
        for (std::vector<entity>::iterator it = ents.begin (); it != ents.end (); it++) {
            get_destination_node ( (*it), _node, destNode, stayParent);

            if (!stayParent) {
                move_entity ( (*it), _node, destNode);
            }
        }
    }

    return true;
}

void quadtree::clean (node& _node) {
    std::cout << "Cleaning node " << _node.key << std::endl;
    if (_node.subdivided) {
        bool empty = true;
        delete_empty_subnodes (_node.key, empty);

        if (empty) {
            // this node has been updated (subnodes deleted)
            get_node (_node.key, _node);

            if (!_node.subdivided && _node.entities == 0 && _node.parentKey != "") {
                node parentNode;
                get_node (_node.parentKey, parentNode);
                clean (parentNode);
            }
        }
    }
    else if (_node.entities == 0 && _node.parentKey != "") {
        // this could be one of the empty nodes, tell parent to clean up
        node parentNode;
        get_node (_node.parentKey, parentNode);
        clean (parentNode);
    }
}

void quadtree::get_node (const std::string& _nodeKey, node& _node) {
    _node.key = _nodeKey;
    _node.parentKey = "";
    _node.rect = rectangle (context, _nodeKey + ":rect");

    redisReply* reply;
    reply = (redisReply*)redisCommand (context, "HGET %s subdivided", _node.key.c_str () );

    if (reply->type == REDIS_REPLY_STRING) {
        int value = std::stoi (reply->str);
        _node.subdivided = (bool)value;
    }

    reply = (redisReply*)redisCommand (context, "HGET %s entities", _node.key.c_str () );

    if (reply->type == REDIS_REPLY_STRING) {
        int value = std::stoi (reply->str);
        _node.entities = value;
    }

    // calculate the parent
    if (_nodeKey != "root") {
        _node.parentKey = _nodeKey.substr (0, _nodeKey.size () - 3);
    }
}

void quadtree::get_destination_node (const entity& _ent, const node& _currNode, node& _destNode, bool& _stayParent) {
    _stayParent = true;
 
    node tlNode, trNode, blNode, brNode;
    get_node (_currNode.key + ":tl", tlNode);
    get_node (_currNode.key + ":tr", trNode);
    get_node (_currNode.key + ":bl", blNode);
    get_node (_currNode.key + ":br", brNode);

    if (tlNode.rect.contains (_ent.pos) ) {
        _destNode = tlNode;
        _stayParent = false;

        //std::cout << "Found destination node " << _destNode.key << std::endl;
    }
    else if (trNode.rect.contains (_ent.pos) ) {
        _destNode = trNode;
        _stayParent = false;

        //std::cout << "Found destination node " << _destNode.key << std::endl;
    }
    else if (blNode.rect.contains (_ent.pos) ) {
        _destNode = blNode;
        _stayParent = false;

        //std::cout << "Found destination node " << _destNode.key << std::endl;
    }
    else if (brNode.rect.contains (_ent.pos) ) {
        _destNode = brNode;
        _stayParent = false;

        //std::cout << "Found destination node " << _destNode.key << std::endl;
    }
}

void quadtree::add_entity (const node& _node, entity& _ent) {
    std::cout << "Adding entity " << _ent.id << " to " << _node.key << " at (" << _ent.pos.x << ", " << _ent.pos.y << ")" << std::endl;

    _ent.key = "entities:" + std::to_string (_ent.id);
    _ent.ownerKey = _node.key;

    // update the node
    redisAppendCommand (context, "HINCRBY %s entities 1", _node.key.c_str () );
    redisAppendCommand (context, "SADD %s:entities %i", _node.key.c_str (), _ent.id);

    // add the entity into redis
    redisAppendCommand (context, "HSET %s x %i", _ent.key.c_str (), _ent.pos.x);
    redisAppendCommand (context, "HSET %s y %i", _ent.key.c_str (), _ent.pos.y);
    redisAppendCommand (context, "HSET %s owner %s", _ent.key.c_str (), _ent.ownerKey.c_str () );
 
    redisReply* reply;
    for (int n = 0; n < 5; n++) {
        redisGetReply (context, (void**)&reply);
        freeReplyObject (reply);
    }
}

void quadtree::update_entity (const entity& _ent) {
    // resave the entity info in redis
    redisAppendCommand (context, "HSET %s x %i", _ent.key.c_str (), _ent.pos.x);
    redisAppendCommand (context, "HSET %s y %i", _ent.key.c_str (), _ent.pos.y);
    redisAppendCommand (context, "HSET %s owner %s", _ent.key.c_str (), _ent.ownerKey.c_str () );
 
    redisReply* reply;
    for (int n = 0; n < 3; n++) {
        redisGetReply (context, (void**)&reply);
        freeReplyObject (reply);
    }
}

void quadtree::delete_entity (entity& _ent) {
    std::cout << "Deleting entity " << _ent.id << " from " << _ent.ownerKey << std::endl;

    // remove the entity from redis and update the node
    redisAppendCommand (context, "HINCRBY %s entities -1", _ent.ownerKey.c_str () );
    redisAppendCommand (context, "SREM %s:entities %i", _ent.ownerKey.c_str (), _ent.id);
    redisAppendCommand (context, "HDEL %s x", _ent.key.c_str () );
    redisAppendCommand (context, "HDEL %s y", _ent.key.c_str () );
    redisAppendCommand (context, "HDEL %s owner", _ent.key.c_str () );

    _ent.key = "";
    _ent.ownerKey = "";

    redisReply* reply;
    for (int n = 0; n < 5; n++) {
        redisGetReply (context, (void**)&reply);
        freeReplyObject (reply);
    }
}

void quadtree::move_entity (entity& _ent, const node& _srcNode, const node& _destNode) {
    std::cout << "Moving entity " << _ent.id << " from " << _srcNode.key << " to " << _destNode.key << std::endl;

    // update both nodes entity info
    redisAppendCommand (context, "HINCRBY %s entities -1", _srcNode.key.c_str () );
    redisAppendCommand (context, "HINCRBY %s entities 1", _destNode.key.c_str () );
    redisAppendCommand (context, "SREM %s:entities %i", _srcNode.key.c_str (), _ent.id);
    redisAppendCommand (context, "SADD %s:entities %i", _destNode.key.c_str (), _ent.id);

    // change the entity's owner
    redisAppendCommand (context, "HSET %s owner %s", _ent.key.c_str (), _destNode.key.c_str () );

    _ent.ownerKey = _destNode.key;

    redisReply* reply;
    for (int n = 0; n < 5; n++) {
        redisGetReply (context, (void**)&reply);
        freeReplyObject (reply);
    }
}

void quadtree::reinsert_entity (entity& _ent, const node& _ownerNode, node& _currNode) {
    std::string nodeKey;
    bool done = false;
    node destNode;
    bool stayParent = false;

    do {
        // check to see if the entity will fit in this node
        if (_currNode.rect.contains (_ent.pos) ) {
            if (!_currNode.subdivided && _currNode.entities + 1 <= maxEntitiesPerNode) {
                // there's still room here so add it
                move_entity (_ent, _ownerNode, _currNode);
                done = true;
            }
            else {
                // subdivide if needed
                if (!_currNode.subdivided) {
                    if (!subdivide (_currNode) ) {
                        // this is as small as the nodes can get, add the entity here
                        move_entity (_ent, _ownerNode, _currNode);
                        done = true;

                        //std::cout << "Minimum node size reached, cannot subdivide " << currNode.key << std::endl;
                    }
                }

                // figure out which subnode the entity should move into
                get_destination_node (_ent, _currNode, destNode, stayParent);

                if (stayParent) {
                    // if this entity won't fit in the subnodes it stays here
                    move_entity (_ent, _ownerNode, _currNode);
                    done = true;
                }
                else {
                    // change the level down one
                    // the loop will repeat and try to insert into the subnode
                    get_node (destNode.key, _currNode);
                }
            }
        }
    } while (!done);
}

void quadtree::relocate_entity (entity& _ent, node& _ownerNode, node& _currNode, node& _destNode) {
    // is the entity contained by this node?
    if (_currNode.rect.contains (_ent.pos) ) {
        bool stayParent = true;
        // check if it needs to move between subnodes
        get_destination_node (_ent, _currNode, _destNode, stayParent);

        // check if the entity is still in the owner same node
        if (_ownerNode.key == _currNode.key) {
            // check if the entity needs to move down,  otherwise we don't have to do anything
            if (!stayParent) {
                // reinsert the entity elsewhere
                reinsert_entity (_ent, _ownerNode, _destNode);
            }
        }
        else {
            // the entity has moved out of its owner node, so reinsert it elsewhere
            reinsert_entity (_ent, _ownerNode, _destNode);

            // clean the former owner node
            clean (_ownerNode);
        }
    }
    else {
        // move up the tree
        get_node (_ownerNode.parentKey, _currNode);
        relocate_entity (_ent, _ownerNode, _currNode, _destNode);
    }
}

void quadtree::get_node_entities (const node& _node, std::vector<entity>& _ents) {
    redisReply* reply;
    redisReply* entityReply;

    reply = (redisReply*)redisCommand (context, "SMEMBERS %s:entities", _node.key.c_str () );

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (unsigned int n = 0; n < reply->elements; n++) {
            entityReply = (redisReply*)redisCommand (context, "HVALS entities:%s", reply->element[n]->str);

            if (entityReply->type == REDIS_REPLY_ARRAY && entityReply->elements == 3) {
                entity ent;
                ent.id = std::stoi (reply->element[n]->str);
                ent.pos = point (std::stoi (entityReply->element[0]->str), std::stoi (entityReply->element[1]->str) );
                ent.key = "entities:" + std::string (reply->element[n]->str);
                ent.ownerKey = entityReply->element[2]->str;
                _ents.push_back (ent);
            }
            freeReplyObject (entityReply);
        }
    }
    freeReplyObject (reply);
}

void quadtree::get_entities (const node& _node, const rectangle& _rect, std::vector<entity>& _ents) {
    if (_rect.contains (_node.rect) ) {
        // the search area completely contains this node, get all entities
        get_all_entities (_node, _ents);
    }
    else if (_rect.intersects (_node.rect) ) {
        // go through the entities in this node and figure out which ones to add
        get_node_entities (_node, tempEnts);

        for (std::vector<entity>::iterator it = tempEnts.begin (); it != tempEnts.end (); it++) {
            if (_rect.contains ( (*it).pos) ) {
                _ents.push_back ( (*it) );
            }
        }
        tempEnts.clear ();

        if (_node.subdivided) {
            // keep going for all subnodes
            node tlNode, trNode, blNode, brNode;
            get_node (_node.key + ":tl", tlNode);
            get_node (_node.key + ":tr", trNode);
            get_node (_node.key + ":bl", blNode);
            get_node (_node.key + ":br", brNode);

            get_entities (tlNode, _rect, _ents);
            get_entities (trNode, _rect, _ents);
            get_entities (blNode, _rect, _ents);
            get_entities (brNode, _rect, _ents);
        }
    }
}

void quadtree::get_all_entities (const node& _node, std::vector<entity>& _ents) {
    get_node_entities (_node, _ents);
    
    if (_node.subdivided) {
        // keep going for all subnodes
        node tlNode, trNode, blNode, brNode;
        get_node (_node.key + ":tl", tlNode);
        get_node (_node.key + ":tr", trNode);
        get_node (_node.key + ":bl", blNode);
        get_node (_node.key + ":br", brNode);

        get_all_entities (tlNode, _ents);
        get_all_entities (trNode, _ents);
        get_all_entities (blNode, _ents);
        get_all_entities (brNode, _ents);
    }
}

void quadtree::delete_empty_subnodes (const std::string& _nodeKey, bool& empty) {
    std::cout << "Deleting empty subnodes for node " << _nodeKey << std::endl;

    std::string quads[4] = {_nodeKey + ":tl", _nodeKey + ":tr", _nodeKey + ":bl", _nodeKey + ":br"};

    // first check if these subnodes contain any entities
    for (int i = 0; i < 4; i++) {
        std::cout << "HGET " << quads[i] << " entities" << std::endl;
        redisAppendCommand (context, "HGET %s entities", quads[i].c_str () );
    }

    redisReply* reply;
    for (int n = 0; n < 4; n++) {
        redisGetReply (context, (void**)&reply);

        if (reply->type == REDIS_REPLY_STRING && strncmp (reply->str, "0", 1) == 0) {
            empty = false;
            std::cout << "Node " << quads[n] << " is not empty" << std::endl;
        }
        freeReplyObject (reply);

        if (!empty)
            break;
    }

    if (empty) {
        // continue on to the subnodes if there are any
        for (int i = 0; i < 4; i++) {
            redisAppendCommand (context, "HGET %s subdivided", quads[i].c_str () );
        }

        for (int i = 0; i < 4; i++) {
            redisGetReply (context, (void**)&reply);

            // only recurse into subnodes if they are subdivided
            if (reply->type == REDIS_REPLY_STRING && strncmp (reply->str, "1", 1) == 0) {
                delete_empty_subnodes (quads[i].c_str (), empty);
            }
            freeReplyObject (reply);

            if (!empty)
                break;
        }
    }

    // delete only this node's subnodes (everything below has been deleted or never existed)
    if (empty) {
        std::cout << "Delete subnodes for node " << _nodeKey << std::endl;
        //delete_subnodes (_nodeKey);
    }
}

void quadtree::delete_subnodes (const std::string& _nodeKey) {
    std::string quads[4] = {_nodeKey + ":tl", _nodeKey + ":tr", _nodeKey + ":bl", _nodeKey + ":br"};
 
    // delete the hashes for each node
    for (int i = 0; i < 4; i++) {
        redisAppendCommand (context, "HDEL %s:%s subdivided", quads[i].c_str () );
        redisAppendCommand (context, "HDEL %s:%s entities", quads[i].c_str () );
    }

    // reset this node to an unsubdivided state
    redisAppendCommand (context, "HSET %s subdivided 0", _nodeKey.c_str () );

    redisReply* reply;
    for (int n = 0; n < 9; n++) {
        redisGetReply (context, (void**)&reply);
        freeReplyObject (reply);
    }
}
