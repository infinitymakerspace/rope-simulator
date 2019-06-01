// Display address from shared info buffer
// cc -o display_addr display_addr.c -lncurses

#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../pru/iface.h"


// Read /dev/mem
void main(void) {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd == -1) {
    perror("Open /dev/mem failed");
    exit(-1);
  }
  // volatile uint32_t *vaddr = (uint32_t *)mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x48300000); // PWM reg

  // 64K buffer shared with the PRU
  volatile uint16_t *shared = (uint16_t *)mmap(NULL, 65536 * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x9fc00000); // shared
  if (shared == 0) {
    perror("Mmap /dev/mem failed");
    exit(-1);
  }

  // iface is at the start of the 8K PRU0 RAM
  volatile struct iface *iface = (volatile struct iface *) mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x4a300000); // PRU_ICSS
  if (iface == 0) {
    perror("Mmap /dev/mem failed on iface");
    exit(-1);
  }
  initscr();
  int row, col;
  getmaxyx(stdscr, row, col);
  clear();
  mvprintw(row-5, 0, "pppppppppiiiiiii");
  mvprintw(row-4, 0, "8765432107654321");
  char *labels[] = {"R1", "R2", "S1", "S2", "T1", "T2"};
  while (1) {
    uint16_t addr = iface->lastaddr;
    uint16_t il = addr & 0x7f; // Bottom 
    uint16_t plane = (addr >> 7) & 3; // SetAB, SetCD, Reset
    uint16_t strand = (addr >> 9) % 12;
    uint16_t mod = (addr >> 9) / 12;
    for (int i = 0; i < 6; i++) {
      mvprintw(i, 0, "%s .... .... .... .... .... .... .... .... .... .... .... ....", labels[i]);
    }
    attron(A_STANDOUT);
    if (mod < 6) {
      mvprintw(mod, 3 + strand * 5, "%04X", addr);
    }
    attroff(A_STANDOUT);
      
    // Show plane 5
    if (addr & (1 << 12)) {
      attron(A_STANDOUT);
    }
    // mvprintw(15, 0, "5                           ");
    attroff(A_STANDOUT);
    
    // Output binary
    for (int i = 0; i < 16; i++) {
      mvprintw(row-3, 15-i, (addr & (1 << i)) ? "1" : "0");
    }
    char *label = "!!!";
    if (mod < 6) {
      label = labels[mod];
    }
    mvprintw(row-2, 0, "%06x %s %2d %d %2x  %d %d    ", addr, label, strand, plane, il, iface->buf[0], iface->buf[1]);
    refresh();
    usleep(10000);
  }
}
