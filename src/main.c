#include <stdio.h>

int main() {
    printf("--- TEST MAPY ---\n");
    Mapa m;
    alokujMapu(&m, 10, 10);
    printf("Mapa uspesne alokovana (rozsah X: %d az %d)\n", m.minX, m.maxX);

    // Test indexu
    int idx = getIndexMapa(&m, 0, 0);
    printf("Index stredu [0,0] je: %d\n", idx);

    uvolniMapu(&m);
    printf("Mapa uspesne uvolnena.\n");
    return 0;
}