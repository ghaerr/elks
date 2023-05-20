#include "serial.h"
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "version.h"

static int serport = -1;

int open_serial(char const* device) {
  serport = open(device, O_RDWR | O_NOCTTY);
  if(serport < 0 ) {
    perror(device);
    goto out1;
  }

  struct termios tio;
  memset(&tio, 0, sizeof(tio));

  if(tcgetattr(serport, &tio) < 0) {
    perror(device);
    goto out2;
  }

  if(cfsetispeed(&tio, PROGRAMMER_BAUD) < 0) {
    perror(device);
    goto out2;
  }
  
  if(cfsetospeed(&tio, PROGRAMMER_BAUD) < 0) {
    perror(device);
    goto out2;
  }


  tio.c_cflag &= ~(CSIZE | CSTOPB | CRTSCTS);
  tio.c_cflag |= CS8 | CLOCAL | CREAD;
  tio.c_iflag &= (IGNBRK); // | IXON | IXOFF | IXANY);
  tio.c_oflag = 0;
  tio.c_lflag = 0;
  tio.c_cc[VMIN] = 1;

  sleep(2);
  if(tcflush(serport, TCIOFLUSH) < 0) {
    perror(device);
    goto out2;
  }

  if(tcsetattr(serport, TCSANOW, &tio) < 0) {
    perror(device);
    goto out2;
  }

  return 0;

out2:
  close(serport);

out1:
  serport = -1;
  return -1;
}


void close_serial(void) {
  close(serport);
  serport = -1;
}


void uart_write_byte(uint8_t byte) {
  if(write(serport, &byte, 1) < 0) {
    perror("serial comm");
    exit(1);
  }
}


uint8_t uart_read_byte() {
  uint8_t buf;
  if(read(serport, &buf, 1) < 0) {
    perror("serial comm");
    exit(1);
  }

  return buf;
}

