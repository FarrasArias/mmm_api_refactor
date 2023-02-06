#pragma once

#include <map>
#include <string>
#include <array>

// START OF NAMESPACE
namespace mmm {

// we map top 99% of the common time signatures
std::map<int,int> time_sig_map = {
  {4,0},
  {3,1},
  {2,2},
  {6,3},
  {1,4},
};

std::map<int,int> rev_time_sig_map = {
  {0,4},
  {1,3},
  {2,2},
  {3,6},
  {4,1},
};

// ",".join(map(str,list((np.arange(128) / (128 / 31)).astype(np.int32) + 1)))
std::map<std::string,std::array<int,128>> velocity_maps = {
  {"no_velocity", {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}},
  {"magenta", {0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,24,25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,30,30,30,30,31,31,31,31}}
};

// ",".join(map(str,list((np.arange(32) * 128/31).astype(np.int32)) + [128] * 96)) 
std::map<std::string,std::array<int,128>> velocity_rev_maps = {
  {"no_velocity",{0,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99}},
  {"magenta", {0,4,8,12,16,20,24,28,33,37,41,45,49,53,57,61,66,70,74,78,82,86,90,94,99,103,107,111,115,119,123,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128}}
};

// maps for genre
std::map<std::string,std::map<std::string,int>> genre_maps = {
  {"msd_cd1",{
    {"Blues",0},
    {"Country",1},
    {"Electronic",2},
    {"Folk",3},
    {"International",4},
    {"Jazz",5},
    {"Latin",6},
    {"New Age",7},
    {"Pop_Rock",8},
    {"Rap",9},
    {"Reggae",10},
    {"RnB",11},
    {"Vocal",12},
    {"none",13}}
  },
  {"msd_cd2c",{
    {"Blues",0},
    {"Country",1},
    {"Electronic",2},
    {"Folk",3},
    {"Jazz",4},
    {"Latin",5},
    {"Metal",6},
    {"New Age",7},
    {"Pop",8},
    {"Punk",9},
    {"Rap",10},
    {"Reggae",11},
    {"RnB",12},
    {"Rock",13},
    {"World",14},
    {"none",15}}
  },
  {"msd_cd2",{
    {"Blues",0},
    {"Country",1},
    {"Electronic",2},
    {"Folk",3},
    {"Jazz",4},
    {"Latin",5},
    {"Metal",6},
    {"New Age",7},
    {"Pop",8},
    {"Punk",9},
    {"Rap",10},
    {"Reggae",11},
    {"RnB",12},
    {"Rock",13},
    {"World",14},
    {"none",15}}
  }
};

const int SAFE_TRACK_MAP[15] = {0,1,2,3,4,5,6,7,8,10,11,12,13,14,15};

std::map<int,std::vector<int>> TRACK_MASKS = {
  {1, {3}},
  {2, {2}},
  {3, {2,3}},
  {4, {1}},
  {5, {1,3}},
  {6, {1,2}},
  {7, {1,2,3}},
  {8, {0}},
  {9, {0,3}},
  {10, {0,2}},
  {11, {0,2,3}},
  {12, {0,1}},
  {13, {0,1,3}},
  {14, {0,1,2}},
  {15, {0,1,2,3}}
};

}
// END OF NAMESPACE