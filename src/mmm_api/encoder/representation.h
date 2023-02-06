#pragma once

#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <string>
#include <iostream>

#include "../enum/token_types.h"
#include <variant>

// START OF NAMESPACE
namespace mmm {

using TOKEN_VARIANT = std::variant<int,std::string,std::tuple<int,int>>;
using TOKEN_TUPLE = std::tuple<mmm::TOKEN_TYPE,TOKEN_VARIANT>;

struct INT_RANGE_FLAG {};
struct INT_VALUES_FLAG {};
struct INT_MAP_FLAG {};
struct STRING_VALUES_FLAG {};
struct STRING_MAP_FLAG {};
struct TIMESIG_VALUES_FLAG {};
struct TIMESIG_MAP_FLAG {};
struct CONTINUOUS_FLAG {};
struct STRING_VECTOR_FLAG {};

INT_RANGE_FLAG RANGE_DOMAIN;
INT_VALUES_FLAG INT_VALUES_DOMAIN;
INT_MAP_FLAG INT_MAP_DOMAIN;
STRING_VALUES_FLAG STRING_VALUES_DOMAIN;
STRING_MAP_FLAG STRING_MAP_DOMAIN;
TIMESIG_VALUES_FLAG TIMESIG_VALUES_DOMAIN;
TIMESIG_MAP_FLAG TIMESIG_MAP_DOMAIN;
CONTINUOUS_FLAG CONTINUOUS_DOMAIN;
STRING_VECTOR_FLAG STRING_VECTOR;

enum TOKEN_INPUT_TYPE {
  TI_INT,
  TI_STRING,
  TI_TIMESIG
};

int to_integer_beatlength(int n, int d) {
  return (int)round(((float)n / d) * 1000);
}

class TOKEN_DOMAIN {
public:
  TOKEN_DOMAIN(int n) {
    for (int value=0; value<n; value++) {
      add(value);
      input_types.push_back( TI_INT );
    }
  }
  TOKEN_DOMAIN(int min, int max, INT_RANGE_FLAG x) {
    for (int value=min; value<max; value++) {
      add(value);
      input_types.push_back( TI_INT );
    }
  }
  TOKEN_DOMAIN(std::vector<int> values, INT_VALUES_FLAG x) {
    for (const auto value : values) {
      add(value);
      input_types.push_back( TI_INT );
    }
  }
  TOKEN_DOMAIN(std::map<int,int> values, INT_MAP_FLAG x) {
    for (const auto kv : values) {
      add(kv.first, kv.second);
      input_types.push_back( TI_INT );
    }
  }
  TOKEN_DOMAIN(std::vector<std::string> values, STRING_VALUES_FLAG x) {
    for (const auto value : values) {
      add(value);
      input_types.push_back( TI_STRING );
    }
  }
  TOKEN_DOMAIN(std::map<std::string,int> values, STRING_MAP_FLAG x) {
    for (const auto kv : values) {
      add(kv.first, kv.second);
      input_types.push_back( TI_STRING );
    }
  }
  TOKEN_DOMAIN(std::vector<std::tuple<int,int>> values, TIMESIG_VALUES_FLAG x) {
    for (const auto value : values) {
      add(value);
      input_types.push_back( TI_TIMESIG );
    }
  }
  TOKEN_DOMAIN(std::map<std::tuple<int,int>,int> values, TIMESIG_MAP_FLAG x) {
    for (const auto kv : values) {
      add(kv.first, kv.second);
      input_types.push_back( TI_TIMESIG );
    }
  }
  TOKEN_DOMAIN(std::vector<float> values, CONTINUOUS_FLAG x) {
    throw std::runtime_error("NOT CURRENTLY IMPLEMENTED!");
  }
  void add_internal(TOKEN_VARIANT x, int y) {
    map_items.push_back( std::make_tuple(x,y) );
    mapping.insert( std::make_pair(x,y) );
    output_domain.insert( y );
    input_domain.insert( x );
  }
  void add(TOKEN_VARIANT x) {
    add_internal(x, (int)input_domain.size());
  }
  void add(TOKEN_VARIANT x, int y) {
    // ensure contiguous domain
    if (contiguous_output.find(y) == contiguous_output.end()) {
      int current_size = contiguous_output.size();
      contiguous_output.insert( std::make_pair(y,current_size) );
    }
    add_internal(x, contiguous_output[y]);
  }
  int encode(TOKEN_VARIANT x) {
    auto it = mapping.find(x);
    if (it == mapping.end()) {
      throw std::runtime_error("TOKEN VALUE IS OUT OF RANGE!");
    }
    return it->second;
  }
  int token_count;
  std::vector<std::tuple<TOKEN_VARIANT,int>> map_items;
  std::map<TOKEN_VARIANT,int> mapping;
  std::set<TOKEN_VARIANT> input_domain;
  std::set<int> output_domain;
  std::map<int,int> contiguous_output; // ensure contiguous output domain
  std::vector<TOKEN_INPUT_TYPE> input_types; // keep track of data types
};

class REPRESENTATION {
public:
  REPRESENTATION(std::vector<std::pair<mmm::TOKEN_TYPE,TOKEN_DOMAIN>> spec) {
    vocab_size = 0;
    for (const auto token_domain : spec) {
      mmm::TOKEN_TYPE tt = std::get<0>(token_domain);
      TOKEN_DOMAIN domain = std::get<1>(token_domain);
      int index = 0;
      for (const auto value : domain.map_items) {
        int token = vocab_size + std::get<1>(value);
        TOKEN_TUPLE toktup = std::make_tuple(tt,std::get<0>(value));
        forward[toktup] = token;
        // we can also encode in two steps variant --> int --> token
        if (domain.input_types[index] != TI_INT) {
          forward[std::make_tuple(tt,std::get<1>(value))] = token;
        }
        backward[token] = toktup;
        backward_types[token] = domain.input_types[index];
        index++;
      }
      vocab_size += domain.output_domain.size();
      domains.insert( std::make_pair(tt,domain.output_domain.size()) );
      token_domains.insert( std::make_pair(tt,domain) );
    }
  }
  int encode(mmm::TOKEN_TYPE tt, TOKEN_VARIANT value) {
    std::tuple<mmm::TOKEN_TYPE,TOKEN_VARIANT> key = std::make_tuple(tt,value);
    auto it = forward.find(key);
    if (it == forward.end()) {
      std::ostringstream buffer;
      auto tdit = token_domains.find(tt);
      if (tdit == token_domains.end()) {
        buffer << "ENCODER ERROR : TOKEN TYPE " << toString(tt) << " IS NOT IN REPRESENTATION";
      }
      else {
        TOKEN_INPUT_TYPE ti = tdit->second.input_types[0];
        buffer << "ENCODER ERROR : VALUE (" << token_variant_to_string(ti, value) << ") NOT IN DOMAIN FOR TOKEN TYPE " << toString(tt);
      }
      throw std::runtime_error(buffer.str());
    }
    return it->second;
  }
  int encode_partial(mmm::TOKEN_TYPE tt, TOKEN_VARIANT value) {
    auto it = token_domains.find(tt);
    if (it == token_domains.end()) {
      throw std::runtime_error("mmm::TOKEN_TYPE NOT PART OF THIS REPRESENTATION");
    }
    return it->second.encode(value);
  }
  int encode_partial_py_int(mmm::TOKEN_TYPE tt, int value) {
    auto it = token_domains.find(tt);
    if (it == token_domains.end()) {
      throw std::runtime_error("mmm::TOKEN_TYPE NOT PART OF THIS REPRESENTATION");
    }
    return it->second.encode(value);
  }
  void token_in_range(int token) {
    if (token >= vocab_size) {
      throw std::runtime_error("TOKEN IS LARGER THAN VOCAB SIZE!");
    }
  }
  int decode(int token) {
    token_in_range(token);
    if (backward_types[token] != TI_INT) {
      throw std::runtime_error("TOKEN CAN NOT BE DECODED AS INT");
    }
    return std::get<int>(std::get<1>(backward[token]));
  }
  std::string decode_string(int token) {
    token_in_range(token);
    if (backward_types[token] != TI_STRING) {
      throw std::runtime_error("TOKEN CAN NOT BE DECODED AS STRING");
    }
    return std::get<std::string>(std::get<1>(backward[token]));
  }
  std::tuple<int,int> decode_timesig(int token) {
    token_in_range(token);
    if (backward_types[token] != TI_TIMESIG) {
      throw std::runtime_error("TOKEN CAN NOT BE DECODED AS TIMESIG");
    }
    return std::get<std::tuple<int,int>>(std::get<1>(backward[token]));
  }
  int max_token() {
    return vocab_size;
  }
  int get_domain_size(mmm::TOKEN_TYPE tt) {
    auto it = domains.find(tt);
    if (it == domains.end()) {
      return 0;
    }
    return it->second;
  }
  bool in_domain(mmm::TOKEN_TYPE tt, int value) {
    auto it = token_domains.find(tt);
    if (it != token_domains.end()) {
      return it->second.output_domain.find(value) != it->second.output_domain.end();
    }
    return false;
  }
  std::vector<int> get_num_bars_domain() {
    std::vector<int> model_dims;
    auto itt = token_domains.find(NUM_BARS);
    if (itt != token_domains.end()) {
      for (const auto value : itt->second.input_domain) {
        model_dims.push_back( std::get<int>(value) );
      }
    }
    return model_dims;
  }
  std::vector<std::tuple<int,int>> get_time_signature_domain() {
    std::vector<std::tuple<int,int>> timesigs;
    auto itt = token_domains.find(TIME_SIGNATURE);
    if (itt != token_domains.end()) {
      for (const auto ts : itt->second.input_domain) {
        timesigs.push_back( std::get<std::tuple<int,int>>(ts) );
      }
    }
    else {
      // the standard models without time signatures only trained on 4/4
      timesigs.push_back( std::make_tuple(4,4) );
    }
    return timesigs;
  }
  void check_token(int token) {
    auto it = backward.find(token);
    if (it == backward.end()) {
      std::ostringstream buffer;
      buffer << "ENCODER ERROR : TOKEN " << token << "IS NOT IN REPRESENTATION";
      throw std::runtime_error(buffer.str());
    }
  }
  bool is_token_type(int token, mmm::TOKEN_TYPE tt) {
    check_token(token);
    return std::get<0>(backward[token]) == tt;
  }
  mmm::TOKEN_TYPE get_token_type(int token) {
    check_token(token);
    return std::get<0>(backward[token]);
  }
  std::set<mmm::TOKEN_TYPE> get_mask_token_types(std::vector<int> &mask) {
    std::set<mmm::TOKEN_TYPE> tts;
    for (int i=0; i<mask.size(); i++) {
      if (mask[i]) {
        tts.insert( get_token_type(i) );
      }
    }
    return tts;
  }
  void show_mask_token_types(std::vector<int> &mask) {
    std::set<mmm::TOKEN_TYPE> tts = get_mask_token_types(mask);
    for (const auto tt : tts) {
      std::cout << toString(tt) << ", ";
    }
    std::cout << std::endl;
  }
  bool has_token_type(mmm::TOKEN_TYPE tt) {
    return token_domains.find(tt) != token_domains.end();
  }
  bool has_token_types(std::vector<mmm::TOKEN_TYPE> tts) {
    for (const auto tt : tts) {
      if (!has_token_type(tt)) {
        return false;
      }
    }
    return true;
  }
  void set_mask(mmm::TOKEN_TYPE tt, std::vector<int> values, std::vector<int> &mask, int mask_value) {
    auto it = token_domains.find(tt);
    if (it != token_domains.end()) {
      for (const auto value : values) {
        if (value == -1) {
          for (const auto v : it->second.input_domain) {
            mask[encode(tt, v)] = mask_value;
          }
        }
        else {
          mask[encode(tt, value)] = mask_value;
        }
      }
    }
  }
  void set_mask(mmm::TOKEN_TYPE tt, std::vector<std::string> values, std::vector<int> &mask, int mask_value, STRING_VECTOR_FLAG x) {
    auto it = token_domains.find(tt);
    if (it != token_domains.end()) {
      for (const auto value : values) {
        mask[encode(tt, value)] = mask_value;
      }
    }
  }
  std::vector<int> encode_to_one_hot(mmm::TOKEN_TYPE tt, std::vector<int> values) {
    std::vector<int> x(vocab_size,0);
    set_mask(tt, values, x, 1);
    /*
    auto it = token_domains.find(tt);
    if (it != token_domains.end()) {
      for (const auto value : values) {
        if (value == -1) {
          for (const auto v : it->second.input_domain) {
            x[encode(tt, v)] = 1;
          }
        }
        else {
          x[encode(tt, value)] = 1;
        }
      }
    }
    */
    return x;
  }
  std::vector<int> get_type_mask(std::vector<mmm::TOKEN_TYPE> tts) {
    std::vector<int> mask(vocab_size,0);
    for (int i=0; i<vocab_size; i++) {
      for (const auto tt : tts) {
        if (is_token_type(i,tt)) {
          mask[i] = 1;
          break;
        }
      }
    }
    return mask;
  }


  std::string token_variant_to_string(TOKEN_INPUT_TYPE ti, TOKEN_VARIANT v) {
    std::string value_str;
    if (ti == TI_INT) {
      value_str = std::to_string(std::get<int>(v));
    }
    else if (ti == TI_STRING) {
      value_str = std::get<std::string>(v);
    }
    else if (ti == TI_TIMESIG) {
      auto ts = std::get<std::tuple<int,int>>(v);
      value_str = std::to_string(std::get<0>(ts)) + "/" + std::to_string(std::get<1>(ts));
    }
    else {
      throw std::runtime_error("THE TOKEN HAS NO INVALID TOKEN_INPUT_TYPE");
    }
    return value_str;
  }

  std::string pretty(int token) {
    auto token_value = backward[token];
    TOKEN_INPUT_TYPE ti = backward_types[token];
    return toString(std::get<0>(token_value)) + std::string(" = ") + token_variant_to_string(ti, std::get<1>(token_value));
  }

  int vocab_size;
  std::map<TOKEN_TUPLE,int> forward;
  std::map<int,TOKEN_TUPLE> backward;
  std::map<int,TOKEN_INPUT_TYPE> backward_types;

  std::map<mmm::TOKEN_TYPE,int> domains;
  std::map<mmm::TOKEN_TYPE,TOKEN_DOMAIN> token_domains;
};

}
// END OF NAMESPACE
