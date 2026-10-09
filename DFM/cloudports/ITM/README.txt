Percepio DFM cloudport module for ITM transfer

For Arm Cortex-M devices, this allows for transferring the DFM alert data using your debug probe.
This requires that the SWO pin is connected to the debug port, normally the case on development boards.
In general, ITM data can also be transferred using an ETM trace port, but that option has not been tested.

Usage:

1. Make sure your build includes the DFM library, including this dfmCloudPort.c.

2. Make sure the local config and include folders in this directory are included in the compiler's include paths. 

3. You need a way to receive the ITM data on host and store it to a binary file. 

  - If using IAR Embedded Workbench, you can enable ITM data logging to file in the SWO Configuration dialog.
    Under ITM stimulus ports, enable the ITM port used (by default 2, third checkbox from the right) both under
    "Enabled ports:" and under "To Log File:".

    Make sure no other ITM port is logged to this file, and that no other code writes to ITM port 2.

    If you prefer a different ITM port, make sure to update the ITM port setting in config/dfmCloudPortConfig.h.

  - If using a different ITM logging solution, make sure only the actual data written to the ITM port is included
    in the log file. No SWO protocol headers or similar should be included.

4. Run the script bin2alerts.py (see below) to process the log file and ingest the alerts into Percepio Detect.
   This script is found in the Percepio Detect receiver folder. Do not use the original percepio-receiver script.

   To ensure the log file is available, start the bin2alerts.py script AFTER the device (or debug session) has started.
   If using IAR, the log file is cleared automatically at the start of each debug session.
   Otherwise, make sure to delete old log files in between sessions.

The receiver script (bin2alert.py)

   The bin2alerts.py has multiple arguments. Run it with the -h argument to see usage information:

   python.exe .\bin2alerts.py -h
   usage: bin2alerts [-h] [-f FOLDER] [-e EOF] [-d DEVICE_NAME] [-v] inputfile

   Extracts DFM alert files from a binary file, with certain controls to verify the data structure. Also allows for pretty-printing all the data with -v flag.

   positional arguments:
      inputfile             The log file to read, containing DFM data.

      options:
        -h, --help            show this help message and exit
        -f FOLDER, --folder FOLDER
                              The folder where the output data should be saved.
        -e EOF, --eof EOF     What to do at end of file:
                                  wait: keeps waiting for more data (exit using Ctrl-C).
                                  exit: exits directly at end of file (default).
        -d DEVICE_NAME, --device_name DEVICE_NAME
                              Sets the Device name, replacing the name set in the target-side DFM library.
        -v, --verbose         Enables verbose output for verification and debugging.

Troubleshooting:

- No data is received

Make sure ITM/SWO output is enabled on the device. This is often handled by
your debugging tool, but not always. Otherwise, you may need to manually start
"SWO trace" or similar in your debugger software to enable ITM output.

- Errors from bin2alerts.py

Some low-cost debug probes are not 100% reliable at the maximum SWO clock. 
Data transmission errors are detected and reported by the bin2alerts script.
In that case, try reducing the SWO clock frequency.

For example, if using the older STLINK v2, the default clock frequency of
2 MHz seems to be too high. Use 500 KHz or 1 MHz for reliable data transfer.

- Alerts are not complete, missing the last alert file

If using IAR, we have noted that the ITM data is not always flushed to the
log file right away, but can remain buffered until more ITM data arrives. 
This was noted when using an STLINK v2 with IAR EWARM 9.60.3.
A workaround is to include the following in your dfmConfig.h:

extern void vDfmCloudPortFlushWithDummyData(void);
#define DFM_CFG_AFTER_ALERT_SEND(pxAlert) vDfmCloudPortFlushWithDummyData();

This will inject dummy data in between the DFM data chunks to flush the IAR
buffer to the log file. The bin2alert script ignores any such dummy data in
between DFM data chunks.



