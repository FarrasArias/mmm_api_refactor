#pragma once

#include <vector>
#include <map>
#include <tuple>

#include "../enum/token_types.h"
#include "../enum/constants.h"
// here we implement two class TOKEN and REPRESENTATION

// for code clarity we need to have values for tokens
// rep is simply 
// 1 std::map<std::pair<mmm::TOKEN_TYPE,int>,int> 
// 2 std::map<int,std::pair<mmm::TOKEN_TYPE,int>>

typedef struct CONTINUOUS_FLAG {};
typedef struct TIMESIG_FLAG {};
typedef struct STRING_FLAG {};
typedef struct RANGE_FLAG {};
typedef struct VALUES_FLAG {};

CONTINUOUS_FLAG CONTINUOUS_DOMAIN;
TIMESIG_FLAG TIMESIG_DOMAIN;
STRING_FLAG STRING_DOMAIN;
RANGE_FLAG RANGE_DOMAIN;
VALUES_FLAG VALUES_DOMAIN;

int to_integer_beatlength(int n, int d) {
  return (int)round(((float)n / d) * 1000);
}

class TOKEN_DOMAIN {
public:
  TOKEN_DOMAIN(int n) {
    for (int i=0; i<n; i++) {
      domain.insert( i );
    }
  }
  TOKEN_DOMAIN(int min, int max, RANGE_FLAG x) {
    for (int i=min; i<max; i++) {
      domain.insert( i );
    }
  }
  TOKEN_DOMAIN(std::vector<int> values, VALUES_FLAG x) {
    for (const auto value : values) {
      domain.insert( value );
    }
  }
  TOKEN_DOMAIN(std::vector<float> bins, CONTINUOUS_FLAG x) {
    // this lets us divide a continuous representation into bins
    // only thing this doesn't support is conditional binning (i.e. density)
    int value = 0;
    for (const auto bin : bins) {
      to_domain.insert( std::make_pair(bin, value) );
      domain.insert( value );
      value++;
    }
  }
  TOKEN_DOMAIN(std::map<std::tuple<int,int>,int> values, TIMESIG_FLAG x) {
    // this lets us specify the set of time sigs that we will use
    timesig_map = values;
    for (const auto kv : values) {
      domain.insert( kv.second );
      float blen = to_integer_beatlength(std::get<0>(kv.first), std::get<1>(kv.first));
      beatlength_timesig_map.insert( std::make_pair(blen,kv.second) );
    }
  }
  TOKEN_DOMAIN(std::map<std::string,int> values, STRING_FLAG x) {
    // this lets us define a mapping between strings and integers
    string_domain = values;
    for (const auto kv : values) {
      domain.insert( kv.second );
    }
  }
  std::set<int> domain;
  std::map<float,int> to_domain;
  std::map<std::string,int> string_domain;
  std::map<std::tuple<int,int>,int> timesig_map;
  std::map<int,int> beatlength_timesig_map;
};

class REPRESENTATION {
public:
  REPRESENTATION(std::vector<std::pair<mmm::TOKEN_TYPE,TOKEN_DOMAIN>> spec) {
    vocab_size = 0;
    for (const auto token_domain : spec) {
      mmm::TOKEN_TYPE tt = std::get<0>(token_domain);
      TOKEN_DOMAIN domain = std::get<1>(token_domain);
      for (const auto value : domain.domain) {
        forward[std::make_tuple(tt,value)] = vocab_size;
        backward[vocab_size] = std::make_tuple(tt,value);
        vocab_size++;
      }
      domains.insert( std::make_pair(tt,domain.domain.size()) );
      token_domains.insert( std::make_pair(tt,domain) );
    }
  }
  bool has_token_type(mmm::TOKEN_TYPE tt) {
    return token_domains.find(tt) != token_domains.end();
  }
  int encode_timesig(int n, int d, bool allow_beatlength_matches) {
    mmm::TOKEN_TYPE tt = TIME_SIGNATURE;
    auto it = token_domains.find(tt);
    if (it != token_domains.end()) {
      if (allow_beatlength_matches) {
        int blen = to_integer_beatlength(n, d);
        auto itt = it->second.beatlength_timesig_map.find( blen );
        if (itt != it->second.beatlength_timesig_map.end()) {
          return encode(tt, itt->second);
        }
      }
      else {
        auto itt = it->second.timesig_map.find(std::make_tuple(n,d));
        if (itt != it->second.timesig_map.end()) {
          return encode(tt, itt->second);
        }
      }
      std::ostringstream buffer;
      buffer << "timesig not supported " << n << "/" << d;
      throw std::runtime_error(buffer.str());
    }
    throw std::runtime_error("token type is invalid");
  }
  int encode_continuous(mmm::TOKEN_TYPE tt, float fvalue) {
    auto it = token_domains.find(tt);
    if (it != token_domains.end()) {
      int value = it->second.to_domain.lower_bound(fvalue)->second;
      return encode(tt, value);
    }
    throw std::runtime_error("token type is invalid");
  }
  int encode_string(mmm::TOKEN_TYPE tt, std::string s) {
    auto it = token_domains.find(tt);
    if (it != token_domains.end()) {
      auto vit = it->second.string_domain.find(s);
      if (vit != it->second.string_domain.end()) {
        return encode(tt, it->second.string_domain[s]);
      }
      std::ostringstream buffer;
      buffer << "string " << s << " is not in domain!";
      throw std::runtime_error(buffer.str());
    }
    throw std::runtime_error("token type is invalid");
  }
  int encode(mmm::TOKEN_TYPE tt, int value) {
    std::tuple<mmm::TOKEN_TYPE,int> key = std::make_tuple(tt,value);
    auto it = forward.find(key);
    if (it == forward.end()) {
      std::ostringstream buffer;
      buffer << "token value out of range " << toString(tt) << " = " << value;
      throw std::runtime_error(buffer.str());
    }
    return it->second;
  }
  int decode(int token) {
    return std::get<1>(backward[token]);
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
      return it->second.domain.find(value) != it->second.domain.end();
    }
    return false;
  }
  bool is_token_type(int token, mmm::TOKEN_TYPE tt) {
    return std::get<0>(backward[token]) == tt;
  }
  std::vector<int> encode_to_one_hot(mmm::TOKEN_TYPE tt, std::vector<int> values) {
    std::vector<int> x(vocab_size,0);
    auto it = token_domains.find(tt);
    if (it != token_domains.end()) {
      for (const auto value : values) {
        if (value == -1) {
          for (const auto v : it->second.domain) {
            x[encode(tt, v)] = 1;
          }
        }
        else {
          x[encode(tt, value)] = 1;
        }
      }
    }
    return x;
  }
  std::vector<int> decode_to_one_hot(std::vector<int> tokens) {
    std::vector<int> one_hot(vocab_size,0);
    for (const auto token : tokens) {
      one_hot[decode(token)] = 1;
    }
    return one_hot;
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
  std::string pretty(int token) {
    auto token_value = backward[token];
    return toString(std::get<0>(token_value)) + std::string(" = ") + std::to_string(std::get<1>(token_value));
  }

  int vocab_size;
  std::map<std::tuple<mmm::TOKEN_TYPE,int>,int> forward;
  std::map<int,std::tuple<mmm::TOKEN_TYPE,int>> backward;
  std::map<mmm::TOKEN_TYPE,int> domains;
  std::map<mmm::TOKEN_TYPE,TOKEN_DOMAIN> token_domains;

};