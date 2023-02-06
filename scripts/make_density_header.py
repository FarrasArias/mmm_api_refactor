import numpy as np
from tqdm import tqdm

# NOTE : before running this you need to run the get_density_quantiles script

data = np.load("OPZ_DENSITY_90000.npz", allow_pickle=True)["data"]

# change : use the same quantile distribution for all drums

density = {}
for tt,inst,n in tqdm(data,leave=False):
  # using the same density quantiles for drum tracks
  if tt in [0,1,2]:
    inst = 0 
  if not (tt,inst) in density:
    density[(tt,inst)] = []
  density[(tt,inst)].append(n)

def qnt(x):
  qnts = [.1,.2,.3,.4,.5,.6,.7,.8,.9]
  return np.hstack([np.quantile(x,qnts), [2**30]]).astype(np.int32)

# duplicate the density lists for drum tracks
# onto other instruments
for inst in range(1,128):
  for k in range(3):
    density[(k,inst)] = density[(k,0)]

density = {k:qnt(v) for k,v in density.items()}

rows = []
for k,v in density.items():
  v = "{" + ",".join([str(vv) for vv in v]) + "}"
  rows.append( "{{{{{ka},{kb}}},{value}}}".format(ka=k[0],kb=k[1],value=v) )

fix_rows = []
for k,v in density.items():
  v = "{" + ",".join([str(np.min(np.where(v==vv)[0])) for vv in v]) + "}"
  fix_rows.append( "{{{{{ka},{kb}}},{value}}}".format(ka=k[0],kb=k[1],value=v) )


with open("src/mmm_api/enum/density_opz.h", "w") as f:
  f.write("""#pragma once

#include <map>
#include <tuple>
#include <vector>

// START OF NAMESPACE
namespace mmm {

std::map<std::tuple<int,int>,std::vector<int>> OPZ_DENSITY_QUANTILES = {
  """)
  f.write(",\n  ".join(rows))
  f.write("""};

std::map<std::tuple<int,int>,std::vector<int>> OPZ_FIX_DUPLICATE_QUANTILES = {
  """)
  f.write(",\n  ".join(fix_rows))
  f.write("""};

}
// END OF NAMESPACE
""")