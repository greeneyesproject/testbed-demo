#ifndef AC_HEADER
#define AC_HEADER

#include <stdio.h>
#include <vector>

typedef struct {
  long low;
  long high;
  long fbits;
  int buffer;
  int bits_to_go;
  long total_bits;
  std::vector<unsigned char> *bitstream;
} ac_encoder;

typedef struct {
  long value;
  long low;
  long high;
  int buffer;
  int bits_to_go;
  int garbage_bits;
  std::vector<unsigned char> bitstream;
} ac_decoder;

typedef struct {
  int nsym;
  int *freq;
  int *cfreq;
  int adapt;
} ac_model;

void ac_encoder_init (ac_encoder *, std::vector<unsigned char> &);
void ac_encoder_done (ac_encoder *);
void ac_decoder_init (ac_decoder *, std::vector<unsigned char> &);
void ac_decoder_done (ac_decoder *);
void ac_model_init (ac_model *, int, int *, int);
void ac_model_done (ac_model *);
long ac_encoder_bits (ac_encoder *);
void ac_encode_symbol (ac_encoder *, ac_model *, int);
int ac_decode_symbol (ac_decoder *, ac_model *);

void ac_encode_symbol_updateModel(ac_encoder *, ac_model *, int, int*);
int  ac_decode_symbol_updateModel(ac_decoder *, ac_model *, int*);

#endif
