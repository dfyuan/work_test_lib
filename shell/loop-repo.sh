repo = ${/home/dfyuan/projects/rockchip/rktools/repo/repo}
echo -e "\033[31m ======start repo sync======\033[0m"
repo sync --no-clone-bundle
while [ $? = 1 ]; do
        echo -e "\033[31m ======sync failed, re-sync again======\033[0m"
        sleep 3
        repo sync --no-clone-bundle
done