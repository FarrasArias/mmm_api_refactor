#include <vector>
#include "xxhash.h"
#include "encoder.h"

// random length up to w ensure first and last are token

std::vector<int> sgbf(std::vector<int> &tokens, int n, int m, int w, int seed) {
  //ENCODER *e = getEncoder(ENCODER_TYPE::TRACK_ENCODER);
  TrackEncoder *enc = new TrackEncoder();
  //PerformanceEncoder *enc = new PerformanceEncoder();
  //EncoderConfig e;
  //string jstr = tenc->tokens_to_json(raw_tokens,&e);
  //vector<int> tokens = enc->json_to_tokens(jstr,&e);

  int vocab_size = enc->rep->max_token();
  std::vector<int> vec(n,0);
  std::vector<int> window(w,-1);
  for (int i=0; i<m; i++) {
    bool started = false;
    int start = 0;
    int end = 0;
    std::fill(window.begin(), window.end(), -1);
    int offset = rand() % ((int)tokens.size()-w+1);
    int last_pitch = -1;
    for (int j=0; j<w; j++) {
      if (rand() % 2) {
        int value = enc->rep->decode(tokens[offset + j], mmm::TOKEN_TYPE::PITCH);
        
        if (value >= 0) {
          if (last_pitch == -1) {
            window[j] = vocab_size + 128;
          }
          else {
            window[j] = vocab_size + 128 + value - last_pitch;
          }
          last_pitch = value;
        }
        else {
          window[j] = tokens[offset + j];
        }
        
        //window[j] = tokens[offset + j];
        if (!started) {
          start = j;
          started = true;
        }
        end = j;
      }
    }
    if ((end - start + 1) > 1) {
      uint64_t h = XXH64(
        reinterpret_cast<char*>(window.data() + start), (end-start+1)*sizeof(int), seed);
      vec[h % n]++;
    }
    
  }
  delete enc;
  return vec;
}