#include <iostream>
#include <vector>
#include <tuple>
#include <map>
#include <set>

#include <google/protobuf/util/json_util.h>

#include "lz4.h"
#include "../protobuf/midi.pb.h"
#include "../encoder/encoder_all.h"
#include "../enum/encoder_types.h"
#include "../enum/train_config.h"

;

class Jagged {
public:
  Jagged(std::string filepath_) {
    filepath = filepath_;
    header_filepath = filepath_ + ".header";
    can_write = false;
    can_read = false;
    flush_count = 0;
    num_bars = 4;
    max_tracks = 12;
    max_seq_len = 2048;

    encoder = NULL;
  }

  void set_seed(int seed) {
    srand(seed); // set the seed
  }

  void set_num_bars(int x) {
    num_bars = x;
  }

  void set_max_tracks(int x) {
    max_tracks = x;
  }

  void set_max_seq_len(int x) {
    max_seq_len = x;
  }

  void enable_write() {
    assert(can_read == false);
    if (can_write) { return; }
    // check that the current file is empty unless force flag is present ?
    fs.open(filepath, ios::out | ios::binary);
    can_write = true;
  }
  
  void enable_read() {
    assert(can_write == false);
    if (can_read) { return; }
    fs.open(filepath, ios::in | ios::binary);
    header_fs.open(header_filepath, ios::in | ios::binary);
    header.ParseFromIstream(&header_fs);
    can_read = true;
  }

  void append(std::string &s, size_t split_id) {
    enable_write();

    size_t start = fs.tellp();
    // begin compress ===============================
    size_t src_size = sizeof(char)*s.size();
    size_t dst_capacity = LZ4_compressBound(src_size);
    char* dst = new char[dst_capacity];
    size_t dst_size = LZ4_compress_default(
      (char*)s.c_str(), dst, src_size, dst_capacity);
    fs.write(dst, dst_size);
    delete[] dst;
    // end compress =================================
    size_t end = fs.tellp();
    midi::Dataset::Item *item;
    switch (split_id) {
      case 0: item = header.add_train(); break;
      case 1: item = header.add_valid(); break;
      case 2: item = header.add_test(); break;
    }
    item->set_start(start);
    item->set_end(end);
    item->set_src_size(src_size);
    flush_count++;

    if (flush_count >= 1000) {
      flush();
      flush_count = 0;
    }
  }

  std::string read(size_t index, size_t split_id) {
    enable_read();

    midi::Dataset::Item item;
    switch (split_id) {
      case 0: item = header.train(index); break;
      case 1: item = header.valid(index); break;
      case 2: item = header.test(index); break;
    }
    size_t csize = item.end() - item.start();
    char* src = new char[csize/sizeof(char)];
    fs.seekg(item.start());
    fs.read(src, csize);
    std::string x(item.src_size(), ' ');
    LZ4_decompress_safe(src,(char*)x.c_str(),csize,item.src_size());
    delete[] src;
    return x;
  }

  py::bytes read_bytes(size_t index, size_t split_id) {
    return py::bytes(read(index, split_id));
  }

  std::string read_json(size_t index, size_t split_id) {
    midi::Piece p;
    std::string serialized_data = read(index, split_id);
    p.ParseFromString(serialized_data);
    std::string json_string;
    google::protobuf::util::MessageToJsonString(p, &json_string);
    return json_string;
  }

  // a new function will create batches of sequences really long
  std::vector<int> encode_for_continue(int split_id, int seq_len, bool LEFT_PAD) {
    
    // wrap this in a while block
    bool no_success = true;
    std::vector<int> tokens;
    while (no_success) {
      try {

        int index = rand() % get_split_size(split_id);
        midi::Piece p;
        std::string serialized_data = read(index, split_id);
        p.ParseFromString(serialized_data);

        // pick a random transpose amount
        std::vector<int> ts;
        for (int t=-6; t<=6; t++) {
          if ((p.min_pitch() + t >= 0) && (p.max_pitch() + t < 128)) {
            ts.push_back(t);
          }
        }

        // encode into a token sequence
        EncoderConfig e;
        e.transpose = ts[rand() % ts.size()];
        e.seed = -1;
        e.do_track_shuffle = true;
        tokens = encoder->encode(&p);

        // pad the tokens (on the LEFT) so they are multiple of seq_len
        // if its equal to zero then we don't need to pad
        if (LEFT_PAD && (tokens.size() % seq_len != 0)) {
          int pad_amount = seq_len - (tokens.size() % seq_len);
          tokens.insert(tokens.begin(), pad_amount, 0);
        }
        no_success = false; // parsing complete
      }
      catch(...) {

      }
    }
    return tokens;
  }

  // fixed length batches with continuation
  std::tuple<std::vector<std::vector<int>>,std::vector<int>> read_batch_w_continue(int batch_size, int split_id, ENCODER_TYPE et, int seq_len, bool FULL_BATCH) {
    enable_read();

    if (!encoder) {
      encoder = getEncoder(et);
    }

    while (bstore.size() < batch_size) {
      bstore.push_back( std::vector<int>() );
    }
    
    // create a batch
    std::vector<int> is_continued(batch_size,0);
    std::vector<std::vector<int>> batch(batch_size,std::vector<int>(seq_len,0));
    for (int i=0; i<batch_size; i++) {

      // add new sequence if necesary
      if (bstore[i].size() > 0) {
        is_continued[i] = 1;
      }
      while (bstore[i].size() == 0) {
        try {
          std::vector<int> tokens;
          if (!FULL_BATCH) {
            tokens = encode_for_continue(split_id, seq_len, true);
          }
          else {
            while (tokens.size() < seq_len) {
              tokens = encode_for_continue(split_id, seq_len, false);
            }
            assert(tokens.size() >= seq_len);
          }
          copy(tokens.begin(), tokens.end(), back_inserter(bstore[i]));
          is_continued[i] = 0;
        }
        catch(...) {
          std::cout << "encoding failed ..." << std::endl;
        }
      }

      // copy first seq_len tokens
      for (int j=0; j<seq_len; j++) {
        batch[i][j] = bstore[i][j];
      }

      // always erase first seq_len tokens
      bstore[i].erase(bstore[i].begin(), bstore[i].begin() + seq_len);

    }
    return std::make_pair(batch, is_continued);
  }

  std::tuple<std::vector<std::vector<int>>,std::vector<std::vector<int>>> read_batch(int batch_size, size_t split_id, ENCODER_TYPE et, TrainConfig *tc) {
    enable_read();
    midi::Piece x;
    int index;
    int nitems = get_split_size(split_id);
    int maxlen = 0;

    ENCODER* enc = getEncoder(et);

    std::vector<std::vector<int>> mask;
    std::vector<std::vector<int>> batch;
    while (batch.size() < batch_size) {
      index = rand() % nitems;
      std::string serialized_data = read(index, split_id);
      x.ParseFromString(serialized_data);

      // pick random segment
      random_segment_dev(&x, 4, true);

      // pick random transpose
      std::tuple<int,int> pitch_ext = get_pitch_extents(&x);
      std::vector<int> choices;
      for (int tr=-6; tr<6; tr++) {
        if ((std::get<0>(pitch_ext)+tr >= 0) && (std::get<1>(pitch_ext)+tr < 128)) {
          choices.push_back( tr );
        }
      }
      enc->config->transpose = choices[rand() % choices.size()];

      // pick bars for infilling
      if (enc->config->do_multi_fill) {
        enc->config->multi_fill = make_bar_mask(&x, .75);
      }

      // encode tokens
      std::vector<int> raw_tokens = enc->encode(&x);
      std::vector<int> tokens;
      if (raw_tokens.size() > max_seq_len) {
        // pick a random section
        int offset = rand() % (raw_tokens.size() - max_seq_len + 1);
        copy(
          raw_tokens.begin() + offset, 
          raw_tokens.begin() + offset + max_seq_len,
          back_inserter(tokens));
      }
      else {
        copy(raw_tokens.begin(), raw_tokens.end(), back_inserter(tokens));
      }
      batch.push_back( tokens );
      mask.push_back( std::vector<int>(tokens.size(),1) );
      maxlen = std::max((int)tokens.size(), maxlen);

    }
    // right pad the sequences
    for (int i=0; i<batch_size; i++) {
      batch[i].insert(batch[i].end(), maxlen-batch[i].size(), 0);
      mask[i].insert(mask[i].end(), maxlen-mask[i].size(), 0);
      assert(mask[i].size() == maxlen);
      assert(batch[i].size() == maxlen);
    }
    return std::make_pair(batch,mask);
  }

  /*
  // LEGACY
  std::tuple<std::vector<std::vector<int>>,std::vector<std::vector<int>>> read_batch(int batch_size, size_t split_id, ENCODER_TYPE et, int minlen) {
    //srand(time(NULL));
    enable_read();
    int nitems = get_split_size(split_id);
    int index;
    midi::Piece p;
    int maxlen = minlen; // every batch will have atleast this shape
    std::vector<std::vector<int>> batch;
    std::vector<std::vector<int>> mask;
    ENCODER* enc = getEncoder(et);
    EncoderConfig e;
    while (batch.size() < batch_size) {
      try {
        index = rand() % nitems;
        //cout << "INDEX = " << index << std::endl;
        std::string serialized_data = read(index, split_id);
        p.ParseFromString(serialized_data);
        // determine a valid transpose on the range [-6,6]
        std::vector<int> ts;
        for (int t=-6; t<=6; t++) {
          if ((p.min_pitch() + t >= 0) && (p.max_pitch() + t < 128)) {
            ts.push_back(t);
          }
        }
        int random_transpose = ts[rand() % ts.size()];
        e.transpose = random_transpose;
        e.seed = -1;
        
        // these can be set by the user with functions
        e.num_bars = num_bars;
        e.max_tracks = max_tracks;
        //e.num_bars = p.segment_length(); // get segment length
        // set this to restrict number of tracks
        // make sure the model is trained on shorter combos too
        if (et == TRACK_INST_HEADER_ENCODER) {
          e.max_tracks = rand() % 10 + 2; // random on [2,12]
        }
        if ((et == TRACK_ONE_TWO_THREE_BAR_FILL_ENCODER) || (et == TRACK_BAR_FILL_ENCODER)) {
          e.multi_fill.clear(); // so that it will be randomly init each time
        }
        std::vector<int> raw_tokens = enc->encode(&p,&e);
        std::vector<int> tokens;
        if (raw_tokens.size() > max_seq_len) {
          // pick a random section
          int offset = rand() % (raw_tokens.size() - max_seq_len + 1);
          copy(
            raw_tokens.begin() + offset, 
            raw_tokens.begin() + offset + max_seq_len,
            back_inserter(tokens));
        }
        else {
          copy(raw_tokens.begin(), raw_tokens.end(), back_inserter(tokens));
        }
        batch.push_back( tokens );
        mask.push_back( std::vector<int>(tokens.size(),1) );
        maxlen = std::max((int)tokens.size(), maxlen);
      }
      catch(...) {
        std::cout << "encoding failed ..." << std::endl;
      }
    }
    // right pad the sequences
    for (int i=0; i<batch_size; i++) {
      batch[i].insert(batch[i].end(), maxlen-batch[i].size(), 0);
      mask[i].insert(mask[i].end(), maxlen-mask[i].size(), 0);
      assert(mask[i].size() == maxlen);
      assert(batch[i].size() == maxlen);
    }

    return std::make_pair(batch,mask);
  }
  */

  int get_size() {
    enable_read();
    return header.train_size() + header.valid_size() + header.test_size();
  }

  int get_split_size(int split_id) {
    enable_read();
    switch (split_id) {
      case 0 : return header.train_size();
      case 1 : return header.valid_size();
      case 2 : return header.test_size();
    }
    return 0; // invalid split id
  }

  void flush() {
    fs.flush();
    header_fs.open(header_filepath, ios::out | ios::binary);
    if (!header.SerializeToOstream(&header_fs)) {
      std::cerr << "ERROR : Failed to write header file" << std::endl;
    }
    header_fs.close();
  }

  void close() {
    flush();
    fs.close();
    header_fs.close();
    can_read = false;
    can_write = false;
  }
  
private:
  std::string filepath;
  std::string header_filepath;
  fstream fs;
  fstream header_fs;
  bool can_write;
  bool can_read;
  midi::Dataset header;
  int flush_count;

  int num_bars;
  int max_tracks;
  int max_seq_len;

  std::vector<std::vector<int>> bstore;
  ENCODER *encoder;
};


// encode midi for dataset
py::bytes encode(std::string &filepath, EncoderConfig *ec, std::map<std::string,std::vector<std::string>> &genre_data) {
  std::string x;
  midi::Piece p;
  parse_new(filepath, &p, ec, &genre_data);
  // filtering out for track dataset
  if (!p.valid_segments_size()) {
    return py::bytes(x); // empty bytes
  }
  p.SerializeToString(&x);
  return py::bytes(x);
}
