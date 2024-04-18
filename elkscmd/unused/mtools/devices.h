struct device {
  char *drv_dev;
  int (*drv_ifunc)() ;
  int drv_mode;
} ;
