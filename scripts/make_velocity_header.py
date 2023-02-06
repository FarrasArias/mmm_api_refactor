# map velocity into n levels
# off, and n-1 loudness levels

N = 32
velocity_map = []
for i in range(128):
  velocity_map.append( (i, int(i // (128./(N-1)) + (i>0))) )

with open("../src/mmm_api/enum/velocity.h", "w") as f:
  f.write("""#pragma once
#include <map>

// START OF NAMESPACE
namespace mmm {


const std::map<int,int> DEFAULT_VELOCITY_MAP = {
""")

  for x,y in velocity_map:
    f.write("\t{{{x},{y}}},\n".format(x=x,y=y))
  
  f.write("};")

  f.write("""

}
// END OF NAMESPACE
""")


