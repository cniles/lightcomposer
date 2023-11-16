#ifndef SAMPLESET_H_
#define SAMPLESET_H_

#include <cstdlib>
#include <cstring>

struct sampleset {
  int size;
  double *data;

  sampleset(int capacity) {
    size = capacity;
    data = (double *)malloc(sizeof(double) * capacity);
  }

  sampleset(const sampleset &o) {
    data = new double[o.size];
    memcpy(data, o.data, o.size);
  }

  ~sampleset() { free(data); }
};

#endif
