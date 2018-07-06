# Resetting USB on a Linux machine


if [[ $EUID != 0 ]] ; then
  echo This must be run as root!
  exit 1
fi

  cd /sys/bus/pci/drivers/xhci_hcd
  echo Resetting devices from $xhci...

  for i in ????:??:??.? ; do
    echo -n "$i" > unbind
    echo -n "$i" > bind
  done
