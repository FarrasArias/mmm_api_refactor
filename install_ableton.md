ssh-keygen -f ~/.ssh/mmm4live_rsa
ssh-keygen -f ~/.ssh/mmm_api_rsa

echo "mmm4live key"
cat ~/.ssh/mmm4live_rsa.pub

echo "mmm_api key"
cat ~/.ssh/mmm_api_rsa.pub

# add keys to ssh config
echo $'Host github.com-MMM4LIVE\n\tHostname github.com\n\tIdentityFile=~/.ssh/mmm4live_rsa' >> ~/.ssh/config

echo $'Host github.com-MMM_API\n\tHostname github.com\n\tIdentityFile=~/.ssh/mmm_api_rsa' >> ~/.ssh/config

echo "ask jeff to add these to the repos"
read varname

git clone git@github.com:jeffreyjohnens/MMM4LIVE.git

# fix the submodule urls and fetch them
cd MMM4LIVE
sed -i '' -e 's|https://bltzr@github.com/|git@github.com:|' .gitmodules
echo $'\tbranch = MMM4Live_updated' >> .gitmodules

read varname
git submodule update --init --recursive

# build
# go into MMM.compose
# remove #set(CMAKE_PREFIX_PATH  /usr/local/lib/) from cmake it kills us
cd source/projects/MMM.compose
cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` .


# steps
git clone git@github.com:jeffreyjohnens/MMM4LIVE.git

# fix submodule urls and fetch
cd MMM4LIVE
sed -i '' -e 's|https://bltzr@github.com/|git@github.com:|' .gitmodules
echo $'\tbranch = MMM4Live_updated' >> .gitmodules
git submodule update --init --recursive

# build
cd source/projects/MMM.compose
# manually remove set(CMAKE_PREFIX_PATH  /usr/local/lib/)
cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` .



