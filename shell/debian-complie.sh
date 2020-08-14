echo -e "\033[31m ======start complie debian======\033[0m"
RELEASE=stretch TARGET=desktop ARCH=arm64 ./mk-base-debian.sh
while [ $? = 1 ]; do
        echo -e "\033[31m ======debian failed, re-complie again======\033[0m"
        sleep 3
        RELEASE=stretch TARGET=desktop ARCH=arm64 ./mk-base-debian.sh
done