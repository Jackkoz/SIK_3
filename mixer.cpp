#include <cstddef>
#include "mixer.h"

void mixer(
    struct mixer_input* inputs, size_t n,  // tablica struktur mixer_input, po jednej strukturze na każdą
                                           // kolejkę FIFO w stanie ACTIVE
    void* output_buf,                      // bufor, w którym mikser powinien umieścić dane do wysłania
    size_t* output_size,                   // początkowo rozmiar output_buf, następnie mikser umieszcza
                                           // w tej zmiennej liczbę bajtów zapisanych w output_buf
    unsigned long tx_interval_ms           // wartość zmiennej TX_INTERVAL
)
{}