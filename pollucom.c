/*
 * Sensus Pollucom E reader program
 *
 * reads data from an IR head and sends data to stdout
 *
 * Nov 2018 J. Seyfried
 *
 * based on:
 *  https://www.mikrocontroller.net/topic/113984#3901715
 *  https://wiki.volkszaehler.org/hardware/channels/meters/warming/sensus_pollucom
 *  https://github.com/UDOOboard/serial_libraries_examples/blob/master/c/c_serial_example_bidirectional.c
 */
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define debug_printf(fmt, ...) if (verbose) printf(fmt, ##__VA_ARGS__)
#define read_delay usleep((7 + 25) * 10000)

int set_interface_attribs (int fd, int speed)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag |= IGNBRK;         // ignore break signal
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
        tty.c_iflag &= ~(IXON | IXOFF | IXANY | BRKINT ); // shut off xon/xoff ctrl
        tty.c_iflag &= ~( IGNPAR | PARMRK | INPCK | ISTRIP | INLCR | IGNCR | ICRNL );
        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= PARENB;          // we need even parity checking for the PolluCom E
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;        // no hardware flow control
        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes", errno);
}



// https://stackoverflow.com/questions/28133020/how-to-convert-bcd-to-decimal
int bcd_decimal(unsigned char hex)
{
    assert(((hex & 0xF0) >> 4) < 10);  // More significant nybble is valid
    assert((hex & 0x0F) < 10);         // Less significant nybble is valid
    int dec = ((hex & 0xF0) >> 4) * 10 + (hex & 0x0F);
    return dec;
}

/*
 * Convert 4 BCD bytes to int (including cast to double)
 *
 * @param hex pointer to the 4 BCD bytes
 * (note: the next 4 bytes must be accessible. Otherwise, a segfault
 *  could be triggered)
 */
double bcd_bytes_4(unsigned char* hex)
{
  return         (double)bcd_decimal(hex[0]) +     100*(double)bcd_decimal(hex[1])
         + 10000*(double)bcd_decimal(hex[2]) + 1000000*(double)bcd_decimal(hex[3]);
}

/*
 * return the next two bytes (lo+hi in this order) as a double value. 
 */
double bytes_2(unsigned char* hex)
{
  return         (double)(hex[0]) +     256*(double)(hex[1]);
}
/*
 * return the next 3 bytes (lo+med+hi in this order) as a double value
 */
double bytes_3(unsigned char* hex)
{
  return         (double)(hex[0]) + (double)(hex[1]<<8) + (double)(hex[2]<<16);
}


int main(int argc, char** argv)
{
  int verbose=0;
  #ifdef DEBUG
    verbose = 1;
  #endif

  char buf[256] = "";
  char device[256]="/dev/ttyUSB0";
  char out[10] = "";
  ssize_t ret_out;
  int o, len=0;

  while ((o=getopt(argc, argv, "vhd:")) != -1)
    switch (o)
    {
      case 'v': verbose=1; break;
      case 'd': strncpy(device, optarg, 255); break;
      case 'h': printf("Sensus PolluCom E IR reader\nUsage: %s [-v] [-d device]\n-v uses verbose output \n-d specifies device (default: /dev/ttyUSB0\n",argv[0]);
                exit(0);
    }

  //  init serial port
  debug_printf("Opening device... ");
  int fd = open (device, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0)
  {
      printf ("error %d opening %s: %s\n", errno, device, strerror (errno));
      return -1;
  }
  debug_printf("ok.\n");
  set_interface_attribs (fd, B2400);      // set speed to 2,400 bps, 8n1 (no parity)
  set_blocking (fd, 0);                   // set no blocking
  debug_printf("Sending init-char... ");
  ret_out = write (fd, "/", 1);           // send wakeup
  debug_printf("ok. (%d - Errno %d)\n",ret_out, errno);
  read_delay;             // wait before reding response
  debug_printf("Sending init string... ");
  ret_out = write (fd, "UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU", 132);           // sent wakeup
  debug_printf("ok. (%d - Errno %d)\n",ret_out, errno);
  debug_printf("Reading response... \n");

  read_delay;             // wait before reding response
  len = read(fd, buf, sizeof buf);

  if (len < 0)
  {
    printf("Errno: %d\n", errno);
    exit(EXIT_FAILURE);
  }
  // Initialisiere MBus -> ACK mit 'E5'
  debug_printf ("ok, read %d bytes\nWriting init...", len);
  out[0] = 0x10;
  out[1] = 0x40; // Init MBus
  out[2] = 0x00;
  out[3] = 0x40; // Checksumme = in[1] + in [2]
  out[4] = 0x16;
  ret_out = write (fd, out, 5);           // sent
  debug_printf("ok. (%d - Errno %d)\n",ret_out, errno);
  debug_printf("Reading response\n");
  read_delay;             // wait before reding response
  len = read (fd, buf, sizeof buf);
  if (len < 0)
  {
    printf("Errno: %d\n", errno);
    exit(EXIT_FAILURE);
  }
  debug_printf("ok (%d bytes).\n", len);
  if( buf[0] == 0xE5 ) {
    // Get Data from MBus
    out[0] = 0x10;
    out[1] = 0x5B;  // GetData
    out[2] = 0x00;
    out[3] = 0x5B;   // Checksumme = in[1] + in [2]
    out[4] = 0x16;
    debug_printf ("ok, read %d bytes\nWriting read request...\n", len);

    ret_out = write (fd, out, 5);
    debug_printf("ok. (%d - Errno %d)\n",ret_out, errno);
    debug_printf("Reading response... \n");
    read_delay;             // wait before reding response
    len = read(fd, buf, sizeof buf);
    // Zaehlerstand
    if (verbose) {
      debug_printf ("ok, read %d bytes\nHex-Dump: \n", len);
      for (int i=0; i<len; i++)
      {
         printf("%02x ",buf[i]);
         if ((i+1)%10 == 0) printf ("\n");
      }
      printf("\nConverting:\n");
    }
    double energie    = bcd_bytes_4(&buf[21]);
    double volumen    = bcd_bytes_4(&buf[27])/1000;
    double durchfluss = bcd_bytes_4(&buf[33])/1000;
    double leistung   = bcd_bytes_4(&buf[39]);
    double temp_in    = bytes_2(&buf[45])/10;
    double temp_out   = bytes_2(&buf[49])/10;
    double temp_diff  = bytes_3(&buf[53])/1000;
    printf ("Energie[kWh]: %.0f\nVolumen: %.3f\nDurchfluss[cbm/h]: %.3f\nLeistung[W]: %.0f\n",
                energie, volumen, durchfluss, leistung);
    printf ("Durchflusstemp.[°C]: %.1f\nRuecklauftemp.[°C]: %.1f\nTemperaturdiff.[K]: %.3f\n",
                temp_in, temp_out, temp_diff);
  } else {
   printf("No data received!\n");
  }
  close (fd);

}

