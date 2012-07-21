#include <ctime>
#include <iostream>
#include <quadtree.hpp>

int main (int argcontext, char** argv) {
    redisContext* context = redisConnect ("localhost", 6379);
    if (context->err) {
        std::cout << "Error: " << context->errstr << std::endl;
        return -1;
    }

    quadtree qtree (context, rectangle (0, 0, 4096, 4096) );

    srand (time (NULL) );
    time_t start, stop;
    int nextId = 1;
    int entityNum = 100;
    std::vector<entity> ents;

    start = time (NULL);
    for (int i = 0; i < entityNum; i++) {
        entity ent;
        ent.id = nextId;
        ent.pos = point (rand () % 4095 + 1, rand () % 4095 + 1);
        ents.push_back (ent);
        qtree.insert_entity (ent);

        nextId++;
    }
    stop = time (NULL);

    std::cout << "Added " << entityNum << " entities in " << stop - start << " seconds" << std::endl;

    int x, y;
    int times = 100;
    std::vector<entity> ents2;
    start = time (NULL);
    for (int n = 0; n < times; n++) {
        x = rand () % 3800 + 1;
        y = rand () % 3800 + 1;
        qtree.get_entities (rectangle (y, x, 200, 200), ents2);
    }
    stop = time (NULL);

    std::cout << "Got " << ents2.size () << " entities searching " << times << " times in " << stop - start << " seconds" << std::endl;
    ents2.clear ();

    start = time (NULL);
    for (std::vector<entity>::iterator it = ents.begin (); it != ents.end (); it++) {
        qtree.get_entity ( (*it).id, *it);
        if ( (*it).ownerKey != "")
            qtree.remove_entity (*it);
    }
    stop = time (NULL);

    std::cout << "Removed " << entityNum << " entities in " << stop - start << " seconds" << std::endl;

    redisFree (context);

    return 0;
}
