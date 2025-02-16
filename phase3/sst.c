#include "sst.h"

extern pcb_PTR test_pcb;
extern pcb_PTR ssi_pcb;
extern state_t uprocStates[UPROCMAX];
extern swpo_t swap_pool[POOLSIZE];

/**
 * Creates a child U-proc and then listens for requests
 * 
 * @param s pointer to the state of the U-proc to be created
 */
void SSTInitialize() {
  // Request the support structure from the SSI
  support_t *sup;
  ssi_payload_t payload_sup = {
    .service_code = GETSUPPORTPTR,
    .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload_sup, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &sup, 0);

  // Create the child U-proc by sending a request to SSI
  pcb_PTR p;
  ssi_create_process_t createProcess = {
    .state = &uprocStates[sup->sup_asid - 1],
    .support = sup,
  };
  ssi_payload_t payload = {
    .service_code = CREATEPROCESS,
    .arg = &createProcess,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &p, 0);

  // Invoke the SST handler
  SSTHandler(sup->sup_asid);
}

/**
 * Handles a request received from a user process
 * 
 * @param asid the ASID of the child U-proc of the SST
 */
void SSTHandler(int asid) {
  while (TRUE) {
    ssi_payload_PTR p_payload = NULL;
    // Listen for an incoming request to handle
    pcb_PTR sender = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &p_payload, 0);
    // Response to send back to the U-proc
    unsigned int response = 0;

    // Handle the request based on the service code
    switch(p_payload->service_code) {
      case GET_TOD:
        // Return the Time Of Day (TOD)
        STCK(response);
        break;
      case TERMINATE:
        // Terminate the SST and consequently the U-proc
        terminate(asid);
        break;
      case WRITEPRINTER:
        // Write a string to the printer
        writePrinter(asid, (sst_print_PTR) p_payload->arg);
        break;
      case WRITETERMINAL:
        // Write a string to the terminal
        writeTerminal(asid, (sst_print_PTR) p_payload->arg);
        break;
      default:
        // Error: Unknown service code
        break;
    }

    // Send the response to the process that made the request
    SYSCALL(SENDMESSAGE, (unsigned int) sender, response, 0);
  }
}

/**
 * Terminates the current process
 * This function releases the frames occupied by the U-proc and notifies the test process.
 */
void terminate(int asid) {
  // Free the frames occupied by the U-proc
  for (int i = 0; i < POOLSIZE; i++) {
    if (swap_pool[i].swpo_asid == asid) {
      swap_pool[i].swpo_asid = NOPROC;
    }
  }
  // Notify the test process about the termination
  SYSCALL(SENDMESSAGE, (unsigned int) test_pcb, 0, 0);
  
  // Send a termination request to the SSI
  ssi_payload_t termPayload = {
    .service_code = TERMPROCESS,
    .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &termPayload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, 0, 0);
}

/**
 * Writes a string of characters to a printer
 * 
 * @param asid the ASID of the process requesting the print
 * @param arg payload containing the string to be printed and its length
 */
void writePrinter(int asid, sst_print_PTR arg) {
  // Get the register for the printer to use
  dtpreg_t *base = (dtpreg_t *)DEV_REG_ADDR(PRNTINT, asid - 1);
  
  // Variable for operation status
  unsigned int status;
  
  // Get the string to print
  char *s = arg->string;

  // Send a message for each character in the string
  while (*s != EOS) {
    // Place the character into data0 register
    base->data0 = (unsigned int) *s;
    
    // Send a DOIO request to the SSI
    ssi_do_io_t doIO = {
      .commandAddr = &base->command,
      .commandValue = PRINTCHR,
    };
    ssi_payload_t ioPayload = {
      .service_code = DOIO,
      .arg = &doIO,
    };

    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &ioPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &status, 0);

    // Verify if the operation was successful
    if (status != READY) {
      PANIC();
    }

    s++;
  }
}

/**
 * Writes a string of characters to a terminal
 * 
 * @param asid the ASID of the process requesting the print
 * @param arg payload containing the string to be printed and its length
 */
void writeTerminal(int asid, sst_print_PTR arg) {
  // Get the register for the terminal to use
  termreg_t *base = (termreg_t *)DEV_REG_ADDR(TERMINT, asid - 1);
  
  // Variable for operation status
  unsigned int status;
  
  // Get the string to print
  char *s = arg->string;

  // Send a message for each character in the string
  while (*s != EOS) {
    // Send a DOIO request to the SSI
    ssi_do_io_t doIO = {
      .commandAddr = &base->transm_command,
      .commandValue = PRINTCHR | (((unsigned int) *s) << 8),
    };
    ssi_payload_t ioPayload = {
      .service_code = DOIO,
      .arg = &doIO,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &ioPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &status, 0);

    // Verify if the operation was successful
    if ((status & TERMSTATMASK) != RECVD) {
      PANIC();
    }

    s++;
  }
}
