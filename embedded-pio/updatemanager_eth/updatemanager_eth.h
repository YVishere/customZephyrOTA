#ifndef ZEPHYRUPDATEETHERNET_H
#define ZEPHYRUPDATEETHERNET_H

void acceptEthernetPayload();
void stopEthernetPayload();

void ethernetUpdateTask(void * p1, void * p2, void * p3);

#endif
