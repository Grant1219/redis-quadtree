#include <ctime>
#include <iostream>
#include <quadtree.hpp>

union player_id {
    unsigned int id;
    unsigned char bytes[4];
};

int main (int argcontext, char** argv) {
    redisContext* context = redisConnect ("localhost", 6379);
    if (context->err) {
        std::cout << "Error: " << context->errstr << std::endl;
        return -1;
    }

    quadtree qtree (context, rectangle (0, 0, 4096, 4096) );

    int nextId = 1;
    std::list<entity> ents;

    srand (time (NULL) );
    for (int i = 0; i < 20; i++) {
        entity ent;
        ent.id = nextId;
        ent.pos = point (rand () % 4095 + 1, rand () % 4095 + 1);
        ents.push_back (ent);
        qtree.insert_entity (ent);

        nextId++;
    }

    /*
    void* reply;
    reply = redisCommand (context, "HSET qtree entities qtree:entities");

    if (!reply) {
        std::cout << "Error: " << context->errstr << std::endl;
    }

    player_id id;
    id.id = 65;
    reply = redisCommand (context, "SADD qtree:entities %b", id.bytes, 4);

    if (!reply) {
        std::cout << "Error: " << context->errstr << std::endl;
    }
    */

    redisFree (context);

    return 0;
}
