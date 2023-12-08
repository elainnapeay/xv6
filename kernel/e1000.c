#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "e1000_dev.h"
#include "net.h"

#define TX_RING_SIZE 16
static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
static struct mbuf *tx_mbufs[TX_RING_SIZE];

#define RX_RING_SIZE 16
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));
static struct mbuf *rx_mbufs[RX_RING_SIZE];

// remember where the e1000's registers live.
static volatile uint32 *regs;

struct spinlock e1000_lock;

// called by pci_init().
// xregs is the memory address at which the
// e1000's registers are mapped.
void
e1000_init(uint32 *xregs)
{
  int i;

  initlock(&e1000_lock, "e1000");

  regs = xregs;

  // Reset the device
  regs[E1000_IMS] = 0; // disable interrupts
  regs[E1000_CTL] |= E1000_CTL_RST;
  regs[E1000_IMS] = 0; // redisable interrupts
  __sync_synchronize();

  // [E1000 14.5] Transmit initialization
  memset(tx_ring, 0, sizeof(tx_ring));
  for (i = 0; i < TX_RING_SIZE; i++) {
    tx_ring[i].status = E1000_TXD_STAT_DD;
    tx_mbufs[i] = 0;
  }
  regs[E1000_TDBAL] = (uint64) tx_ring;
  if(sizeof(tx_ring) % 128 != 0)
    panic("e1000");
  regs[E1000_TDLEN] = sizeof(tx_ring);
  regs[E1000_TDH] = regs[E1000_TDT] = 0;
  
  // [E1000 14.4] Receive initialization
  memset(rx_ring, 0, sizeof(rx_ring));
  for (i = 0; i < RX_RING_SIZE; i++) {
    rx_mbufs[i] = mbufalloc(0);
    if (!rx_mbufs[i])
      panic("e1000");
    rx_ring[i].addr = (uint64) rx_mbufs[i]->head;
  }
  regs[E1000_RDBAL] = (uint64) rx_ring;
  if(sizeof(rx_ring) % 128 != 0)
    panic("e1000");
  regs[E1000_RDH] = 0;
  regs[E1000_RDT] = RX_RING_SIZE - 1;
  regs[E1000_RDLEN] = sizeof(rx_ring);

  // filter by qemu's MAC address, 52:54:00:12:34:56
  regs[E1000_RA] = 0x12005452;
  regs[E1000_RA+1] = 0x5634 | (1<<31);
  // multicast table
  for (int i = 0; i < 4096/32; i++)
    regs[E1000_MTA + i] = 0;

  // transmitter control bits.
  regs[E1000_TCTL] = E1000_TCTL_EN |  // enable
    E1000_TCTL_PSP |                  // pad short packets
    (0x10 << E1000_TCTL_CT_SHIFT) |   // collision stuff
    (0x40 << E1000_TCTL_COLD_SHIFT);
  regs[E1000_TIPG] = 10 | (8<<10) | (6<<20); // inter-pkt gap

  // receiver control bits.
  regs[E1000_RCTL] = E1000_RCTL_EN | // enable receiver
    E1000_RCTL_BAM |                 // enable broadcast
    E1000_RCTL_SZ_2048 |             // 2048-byte rx buffers
    E1000_RCTL_SECRC;                // strip CRC
  
  // ask e1000 for receive interrupts.
  regs[E1000_RDTR] = 0; // interrupt after every received packet (no timer)
  regs[E1000_RADV] = 0; // interrupt after every packet (no timer)
  regs[E1000_IMS] = (1 << 7); // RXDW -- Receiver Descriptor Write Back
}

  //
 // Transmits an Ethernet frame using the e1000 network interface.
 //
int e1000_transmit(struct mbuf *m) {
  // Acquire the e1000 lock to ensure exclusive access to the transmit ring
  acquire(&e1000_lock);

  // Get the current index of the transmit ring
  int indx = regs[E1000_TDT];

  // Overflow detection
  if((tx_ring[indx].status & E1000_RXD_STAT_DD) == 0) {
    // Check if the transmit ring is full
    release(&e1000_lock);
    return -1; // Return -1 to indicate overflow
  }

  if(tx_mbufs[indx] != 0) {
    // Check if there is an existing mbuf at the current index
    mbuffree(tx_mbufs[indx]); // Free the existing mbuf
  }

  tx_mbufs[indx] = m; // Set the mbuf at the current index to the new mbuf
  tx_ring[indx].addr = (uint64) m->head; // Set the address of the mbuf in the transmit ring
  tx_ring[indx].length = m->len; // Set the length of the mbuf in the transmit ring
  tx_ring[indx].cmd |= E1000_TXD_CMD_EOP; // Set the End of Packet flag in the transmit ring
  tx_ring[indx].cmd |= E1000_TXD_CMD_RS; // Set the Report Status flag in the transmit ring

  // Update the transmit descriptor tail index
  regs[E1000_TDT] = (indx + 1) % TX_RING_SIZE;

  // Release the e1000 lock
  release(&e1000_lock);

  // Return 0 to indicate successful transmission
  return 0;
}

//
 //Receives incoming packets from the network interface.
 //This function processes the received packets from the network interface.
 //It retrieves the packet data from the receive ring buffer and passes it to the network layer for further processing.
 // The function also updates the receive ring buffer to prepare for receiving new packets.
 //
static void
e1000_recv(void) {
  struct mbuf *buf; // Declare a pointer to an mbuf structure

  int indx = (regs[E1000_RDT] + 1) % RX_RING_SIZE; // Calculate the index of the next receive descriptor in the receive ring

  while((rx_ring[indx].status & E1000_RXD_STAT_DD)) { // Check if the receive descriptor has the DD (Descriptor Done) status bit set
    acquire(&e1000_lock); // Acquire the e1000 lock to ensure exclusive access to the receive ring

    buf = rx_mbufs[indx]; // Assign the mbuf pointer to the current index of the receive ring
    mbufput(buf, rx_ring[indx].length); // Pass the mbuf and its length to the mbufput function to process the received packet

    // refill a new mbuf
    rx_mbufs[indx] = mbufalloc(0); // Allocate a new mbuf for the next received packet

    rx_ring[indx].addr = (uint64) rx_mbufs[indx]->head; // Set the address of the new mbuf in the receive ring
    rx_ring[indx].status = 0; // Clear the status of the receive descriptor

    regs[E1000_RDT] = indx; // Update the receive descriptor tail index

    release(&e1000_lock); // Release the e1000 lock

    net_rx(buf); // Pass the processed mbuf to the net_rx function for further processing

    indx = (regs[E1000_RDT] + 1) % RX_RING_SIZE; // Calculate the index of the next receive descriptor in the receive ring
  }
}

void
e1000_intr(void)
{
  // tell the e1000 we've seen this interrupt;
  // without this the e1000 won't raise any
  // further interrupts.
  regs[E1000_ICR] = 0xffffffff;

  e1000_recv();
}
