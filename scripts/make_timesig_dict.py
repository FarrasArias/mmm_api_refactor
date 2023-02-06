import pandas as pd

def make_cpp_dict(vals, name):
    content = ["\t{{{{{num},{den}}},{index}}},".format(
        num=int(v.split("/")[0].replace('"','')), 
        den=int(v.split("/")[1].replace('"','')),
        index=ii) for ii,v in enumerate(vals)]
    s = """const std::map<std::tuple<int,int>,int> {name}  = {{
{content}
}};
""".format(name=name, content="\n".join(content))
    print(s)


df = pd.read_csv("scripts/ts.csv")

ts = df.iloc[:,0].to_numpy()[:36]
make_cpp_dict(ts, "YELLOW_TS_MAP")

ts = df.iloc[:,3].to_numpy()[:19]
make_cpp_dict(ts, "BLUE_TS_MAP")