#include "sockutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void die(std::string message) {
  perror(message.c_str());
  exit(1);
}

void copyData(int from, int to) {
  char buf[1024];
  int amount;

  while ((amount = read(from, buf, sizeof(buf))) > 0) {
    if (write(to, buf, amount) != amount) {
      die("write");
      return;
    }
  }

  if (amount < 0) {
    die("read");
  }
}