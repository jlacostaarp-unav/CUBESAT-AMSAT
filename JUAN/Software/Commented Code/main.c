/*
CODIGO MODIFICADO, ANOTADO Y REVISADO POR JUAN LACOSTA PARA SU PROYECTO FINAL DE GRADO
*/

#include "main.h"

int main(int argc, char * argv[]) {
// ============================================================================================================================================================================================================================================= 
// - DEVICE DETECTION
// ============================================================================================================================================================================================================================================= 

  char resbuffer[1000];
  const char testStr[] = "cat /proc/cpuinfo | grep 'Revision' | awk '{print $3}' | sed 's/^1000//' | grep '902120'";
  FILE *file_test = sopen(testStr);  // see if Pi Zero 2  
  fgets(resbuffer, 1000, file_test);
  fprintf(stderr, "Pi test result: %s\n", resbuffer);
  fclose(file_test);	
  
  fprintf(stderr, " %x ", resbuffer[0]);
  fprintf(stderr, " %x ", resbuffer[1]);	
  if ((resbuffer[0] == '9') && (resbuffer[1] == '0')) 
  {
    sleep(5);  // try sleep at start to help boot
    voltageThreshold = 3.7;
    printf("Pi Zero 2 detected");
  }
	
  printf("\n\nCubeSatSim v1.3.2 starting...\n\n");

// ============================================================================================================================================================================================================================================= 

  wiringPiSetup();	

// ============================================================================================================================================================================================================================================= 
// - LOADING INITIAL CONFIGURATION 		
// ============================================================================================================================================================================================================================================= 
   
  // Open configuration file with callsign and reset count	
  FILE * config_file = fopen("/home/pi/CubeSatSim/sim.cfg", "r");
  if (config_file == NULL) {
    printf("Creating config file.");
    config_file = fopen("/home/pi/CubeSatSim/sim.cfg", "w");
    fprintf(config_file, "%s %d", " ", 100);
    fclose(config_file);
    config_file = fopen("/home/pi/CubeSatSim/sim.cfg", "r");
  }

//  char * cfg_buf[100];

  fscanf(config_file, "%s %d %f %f %s %d %s %s %s", 
	  call, & reset_count, & lat_file, & long_file, sim_yes, & squelch, tx, rx, hab_yes);
  fclose(config_file);
  fprintf(stderr,"Config file /home/pi/CubeSatSim/sim.cfg contains %s %d %f %f %s %d %s %s %s\n", 
	  call, reset_count, lat_file, long_file, sim_yes, squelch, tx, rx, hab_yes);

  fprintf(stderr, "Transmit on %s Receive on %s\n", tx, rx);

//  program_radio();  // do in rpitx instead
	
  reset_count = (reset_count + 1) % 0xffff;

  if ((fabs(lat_file) > 0) && (fabs(lat_file) < 90.0) && (fabs(long_file) > 0) && (fabs(long_file) < 180.0)) {
    fprintf(stderr, "Valid latitude and longitude in config file\n");
// convert to APRS DDMM.MM format
//    latitude = toAprsFormat(lat_file);
//    longitude = toAprsFormat(long_file);
    latitude = lat_file;
    longitude = long_file;	  
    fprintf(stderr, "Lat/Long %f %f\n", latitude, longitude);		  
    fprintf(stderr, "Lat/Long in APRS DDMM.MM format: %07.2f/%08.2f\n", toAprsFormat(latitude), toAprsFormat(longitude));
    newGpsTime = millis();	  
   	  
  } else { // set default
//    latitude = toAprsFormat(latitude);
//    longitude = toAprsFormat(longitude);
      newGpsTime = millis();  
  }

// ============================================================================================================================================================================================================================================= 
	

// ============================================================================================================================================================================================================================================= 
// - SIM / HAB MODE ON
// ============================================================================================================================================================================================================================================= 

  if (strcmp(sim_yes, "yes") == 0) {
	  sim_mode = TRUE;
	  fprintf(stderr, "Sim mode is ON\n");
  }
  if (strcmp(hab_yes, "yes") == 0) {
	  hab_mode = TRUE;
	  fprintf(stderr, "HAB mode is ON\n");
  }
// ============================================================================================================================================================================================================================================= 

	
//  FILE * rpitx_stop = popen("sudo systemctl stop rpitx", "r");
  FILE * rpitx_stop = popen("sudo systemctl restart rpitx", "r");
  pclose(rpitx_stop);
	
//  FILE * file_deletes = popen("sudo rm /home/pi/CubeSatSim/ready /home/pi/CubeSatSim/cwready > /dev/null", "r");
//  pclose(file_deletes);	
	
// ============================================================================================================================================================================================================================================= 
// - COMPROBACIÓN DE BUSES DISPONIBLES (1 Y 3) PARA LA POSTERIOR UTILIZACIÓN DE LOS SENSORES
// ============================================================================================================================================================================================================================================= 

  printf("Test bus 1\n");
  fflush(stdout);	
  i2c_bus1 = (test_i2c_bus(1) != -1) ? 1 : OFF;
  printf("Test bus 3\n");	
  fflush(stdout);
  i2c_bus3 = (test_i2c_bus(3) != -1) ? 3 : OFF;
  printf("Finished testing\n");	
  fflush(stdout);

// ============================================================================================================================================================================================================================================= 

//  sleep(2);

//#ifdef HAB
  if (hab_mode)	
  	fprintf(stderr, "HAB mode enabled - in APRS balloon icon and no battery saver or low voltage shutdown\n");
//#endif
	
//  FILE * rpitx_restart = popen("sudo systemctl restart rpitx", "r");
//  pclose(rpitx_restart);
	
// ============================================================================================================================================================================================================================================= 
// - CONFIGURACIÓN MODO DE TELEMETRÍA
// ============================================================================================================================================================================================================================================= 

  mode = FSK;
  frameCnt = 1;

  if (argc > 1) {
    //	  strcpy(src_addr, argv[1]);
    if ( * argv[1] == 'b') {
      mode = BPSK;
      printf("Mode BPSK\n");
    } else if ( * argv[1] == 'a') {
      mode = AFSK;
      printf("Mode AFSK\n");
    } else if ( * argv[1] == 'm') {
      mode = CW;
      printf("Mode CW\n");
    } else {
      printf("Mode FSK\n");
    }

    if (argc > 2) {
      //		  printf("String is %s %s\n", *argv[2], argv[2]);
      loop = atoi(argv[2]);
      loop_count = loop;
    }
    printf("Looping %d times \n", loop);

    if (argc > 3) {
      if ( * argv[3] == 'n') {
        cw_id = OFF;
        printf("No CW id\n");
      }
    }
  } else {
	  
    FILE * mode_file = fopen("/home/pi/CubeSatSim/.mode", "r");
    if (mode_file != NULL) {	
      char mode_string;	
      mode_string = fgetc(mode_file);
      fclose(mode_file);
      printf("Mode file /home/pi/CubeSatSim/.mode contains %c\n", mode_string);

      if ( mode_string == 'b') {
        mode = BPSK;
        printf("Mode is BPSK\n");
      } else if ( mode_string == 'a') {
        mode = AFSK;
        printf("Mode is AFSK\n");
      } else if ( mode_string == 's') {
        mode = SSTV;
        printf("Mode is SSTV\n");
     } else if ( mode_string == 'm') {
        mode = CW;
        printf("Mode is CW\n");
      } else {
        printf("Mode is FSK\n");
      }	    
    }
  } 

// ============================================================================================================================================================================================================================================= 


  // Open telemetry file with STEM Payload Data
  telem_file = fopen("/home/pi/CubeSatSim/telem.txt", "a");
  if (telem_file == NULL) 
    printf("Error opening telem file\n");
  fclose(telem_file);
  printf("Opened telem file\n");
	

  battery_saver_mode = battery_saver_check();
/*
  if (battery_saver_mode == ON)	
  	fprintf(stderr, "\nBattery_saver_mode is ON\n\n");
  else
	fprintf(stderr, "\nBattery_saver_mode is OFF\n\n");
*/	

// ============================================================================================================================================================================================================================================= 
// - CONFIGURARCIÓN E INICIALIZACIÓN MODE AFSK CON TRANSCEPTOR AX-5043
// ============================================================================================================================================================================================================================================= 

  fflush(stderr);
  
  if (mode == AFSK)
  {
  // Check for SPI and AX-5043 Digital Transceiver Board	
    FILE * file = popen("sudo raspi-config nonint get_spi", "r");
//  printf("getc: %c \n", fgetc(file));
    if (fgetc(file) == 48) {
      printf("SPI is enabled!\n"); // VERIFICACIÓN SOPORTE SPI

      FILE * file2 = popen("ls /dev/spidev0.* 2>&1", "r");
              printf("Result getc: %c \n", getc(file2));

      if (fgetc(file2) != 'l') {
        printf("SPI devices present!\n"); // VERIFICACIÓN DISPOSITIVOS SPI
      //	  }

        // CONFIGURACIÓN SPI:
        setSpiChannel(SPI_CHANNEL);
        setSpiSpeed(SPI_SPEED);
        initializeSpi();

        ax25_init( & hax25, (uint8_t * ) dest_addr, 11, (uint8_t * ) call, 11, AX25_PREAMBLE_LEN, AX25_POSTAMBLE_LEN);
        if (init_rf()) {
          printf("AX5043 successfully initialized!\n");
          ax5043 = TRUE;
          cw_id = OFF;
//        mode = AFSK;
        //		cycle = OFF;
          printf("Mode AFSK with AX5043\n");
          transmit = TRUE;
//	sleep(10);  // just in case CW ID is sent      
        } else
          printf("AX5043 not present!\n");
          pclose(file2);	    
      }
    }
    pclose(file);
  }	

// ============================================================================================================================================================================================================================================= 


// ============================================================================================================================================================================================================================================= 
// - CONFIGURACIÓN E INICIALIZACIÓN LEDS (PUERTOS, PINES...)
// ============================================================================================================================================================================================================================================= 

  txLed = 0; // defaults for vB3 board without TFB
  txLedOn = LOW;
  txLedOff = HIGH;
  if (!ax5043) { // COMPRUEBA QUE EL TRANSCEPTOR ESTÁ PRESENTE
    pinMode(2, INPUT);
    pullUpDnControl(2, PUD_UP);

    if (digitalRead(2) != HIGH) {
      printf("vB3 with TFB Present\n");
      vB3 = TRUE;
      txLed = 3;
      txLedOn = LOW;
      txLedOff = HIGH;
      onLed = 0;
      onLedOn = LOW;
      onLedOff = HIGH;
      transmit = TRUE;
    } else {
      pinMode(3, INPUT);
      pullUpDnControl(3, PUD_UP);

      if (digitalRead(3) != HIGH) {
        printf("vB4 Present with UHF BPF\n");
        txLed = 2;
        txLedOn = HIGH;
        txLedOff = LOW;
        vB4 = TRUE;
        onLed = 0;
        onLedOn = HIGH;
        onLedOff = LOW;
        transmit = TRUE;
      } else {
        pinMode(26, INPUT);
        pullUpDnControl(26, PUD_UP);

        if (digitalRead(26) != HIGH) {
          printf("v1 Present with UHF BPF\n");
          txLed = 2;
          txLedOn = HIGH;
          txLedOff = LOW;
          vB5 = TRUE;
          onLed = 27;
          onLedOn = HIGH;
          onLedOff = LOW;
          transmit = TRUE;
        }
	else {
          pinMode(23, INPUT);
          pullUpDnControl(23, PUD_UP);
		
          if (digitalRead(23) != HIGH) {
            printf("v1 Present with VHF BPF\n");
            txLed = 2;
            txLedOn = HIGH;
            txLedOff = LOW;
            vB5 = TRUE;
            onLed = 27;
            onLedOn = HIGH;
            onLedOff = LOW;
            printf("VHF BPF not yet supported so no transmit\n");
	    transmit = FALSE;
          }		
	}
      }
    }
  }
  pinMode(txLed, OUTPUT);
  digitalWrite(txLed, txLedOff);
  #ifdef DEBUG_LOGGING
  printf("Tx LED Off\n");
  #endif
  pinMode(onLed, OUTPUT);
  digitalWrite(onLed, onLedOn);
  #ifdef DEBUG_LOGGING
  printf("Power LED On\n");
  #endif


/*
  if (mode == SSTV) {	
    pinMode(29, OUTPUT);  // make SR_FRS transmit
    digitalWrite(29, HIGH);
    pinMode(28, OUTPUT);
    digitalWrite(28, LOW);
  }	
*/

// ============================================================================================================================================================================================================================================= 


// ============================================================================================================================================================================================================================================= 
// - CONFIGURACIONES BUSES I2C
// ============================================================================================================================================================================================================================================= 

  config_file = fopen("sim.cfg", "w");
  fprintf(config_file, "%s %d %8.4f %8.4f %s %d %s %s %s", call, reset_count, lat_file, long_file, sim_yes, squelch, tx, rx, hab_yes);
  //    fprintf(config_file, "%s %d", call, reset_count);
  fclose(config_file);
  config_file = fopen("sim.cfg", "r");

  if (vB4) {
    map[BAT] = BUS;
    map[BUS] = BAT;
    snprintf(busStr, 10, "%d %d", i2c_bus1, test_i2c_bus(0));
  } else if (vB5) {
    map[MINUS_X] = MINUS_Y;
    map[PLUS_Z] = MINUS_X;	
    map[MINUS_Y] = PLUS_Z;		  

    if (access("/dev/i2c-11", W_OK | R_OK) >= 0) { // Test if I2C Bus 11 is present			
      printf("/dev/i2c-11 is present\n\n");
      snprintf(busStr, 10, "%d %d", test_i2c_bus(1), test_i2c_bus(11));
    } else {
      snprintf(busStr, 10, "%d %d", i2c_bus1, i2c_bus3);
    }
  } else {
    map[BUS] = MINUS_Z;
    map[BAT] = BUS;
    map[PLUS_Z] = BAT;
    map[MINUS_Z] = PLUS_Z;
    snprintf(busStr, 10, "%d %d", i2c_bus1, test_i2c_bus(0));
    voltageThreshold = 8.0;
  }

// ============================================================================================================================================================================================================================================= 
	
  // CHECK FOR CAMERA	
//  char cmdbuffer1[1000];
  FILE * file4 = popen("vcgencmd get_camera", "r");
  fgets(cmdbuffer, 1000, file4);
  char camera_present[] = "supported=1 detected=1";
  // printf("strstr: %s \n", strstr( & cmdbuffer1, camera_present));
  camera = (strstr( (const char *)& cmdbuffer, camera_present) != NULL) ? ON : OFF;
  //  printf("Camera result:%s camera: %d \n", & cmdbuffer1, camera);
  pclose(file4);

  #ifdef DEBUG_LOGGING
  printf("INFO: I2C bus status 0: %d 1: %d 3: %d camera: %d\n", i2c_bus0, i2c_bus1, i2c_bus3, camera);
  #endif
		
  FILE * file5 = popen("sudo rm /home/pi/CubeSatSim/camera_out.jpg > /dev/null 2>&1", "r");
  file5 = popen("sudo rm /home/pi/CubeSatSim/camera_out.jpg.wav > /dev/null 2>&1", "r");
  pclose(file5);
	

// ============================================================================================================================================================================================================================================= 
// - CONEXIÓN CON PLACA STEM 
// ============================================================================================================================================================================================================================================= 

  // try connecting to STEM Payload board using UART
  // /boot/config.txt and /boot/cmdline.txt must be set correctly for this to work	

  if (!ax5043 && !vB3 && !(mode == CW) && !(mode == SSTV)) // don't test for payload if AX5043 is present or CW or SSTV modes
  {
    payload = OFF;

    if ((uart_fd = serialOpen("/dev/ttyAMA0", 115200)) >= 0) {  // was 9600 // SE COMUNICA CON LA STEM PAYLOAD
      printf("Serial opened to Pico\n");	    
      payload = ON;
/*	    
      char c;
      int charss = (char) serialDataAvail(uart_fd);
      if (charss != 0)
        printf("Clearing buffer of %d chars \n", charss);
      while ((charss--> 0))
        c = (char) serialGetchar(uart_fd); // clear buffer

      unsigned int waitTime;
      int i;
      for (i = 0; i < 2; i++) {
	if (payload != ON) {
          serialPutchar(uart_fd, 'R');
          printf("Querying payload with R to reset\n");
          waitTime = millis() + 500;
          while ((millis() < waitTime) && (payload != ON)) {
            if (serialDataAvail(uart_fd)) {
              printf("%c", c = (char) serialGetchar(uart_fd));
              fflush(stdout);
              if (c == 'O') {
                printf("%c", c = (char) serialGetchar(uart_fd));
                fflush(stdout);
                if (c == 'K')
                  payload = ON;
              }
            }
            printf("\n");
            //        sleep(0.75);
          }
        }
      }
      if (payload == ON)  {	    
        printf("\nSTEM Payload is present!\n");
	sleep(2);  // delay to give payload time to get ready
      } 
      else {
        printf("\nSTEM Payload not present!\n -> Is STEM Payload programed and Serial1 set to 115200 baud?\n");
	printf("Turning on Payload anyway\n");
	payload = ON;      
	      
      }
*/	    
    } else {
      fprintf(stderr, "Unable to open UART: %s\n -> Did you configure /boot/config.txt and /boot/cmdline.txt?\n", strerror(errno));
    }
  }

// ============================================================================================================================================================================================================================================= 



// ============================================================================================================================================================================================================================================= 
// - MODO SIMULACIÓN DE TELEMETRÍA
// ============================================================================================================================================================================================================================================= 

  if ((i2c_bus3 == OFF) || (sim_mode == TRUE)) {
//  if (sim_mode == TRUE) {

    sim_mode = TRUE;
	    
    fprintf(stderr, "Simulated telemetry mode!\n");

    srand((unsigned int)time(0));

    axis[0] = rnd_float(-0.2, 0.2);
    if (axis[0] == 0)
      axis[0] = rnd_float(-0.2, 0.2);
    axis[1] = rnd_float(-0.2, 0.2);
    axis[2] = (rnd_float(-0.2, 0.2) > 0) ? 1.0 : -1.0;

    angle[0] = (float) atan(axis[1] / axis[2]);
    angle[1] = (float) atan(axis[2] / axis[0]);
    angle[2] = (float) atan(axis[1] / axis[0]);

    volts_max[0] = rnd_float(4.5, 5.5) * (float) sin(angle[1]);
    volts_max[1] = rnd_float(4.5, 5.5) * (float) cos(angle[0]);
    volts_max[2] = rnd_float(4.5, 5.5) * (float) cos(angle[1] - angle[0]);

    float amps_avg = rnd_float(150, 300);

    amps_max[0] = (amps_avg + rnd_float(-25.0, 25.0)) * (float) sin(angle[1]);
    amps_max[1] = (amps_avg + rnd_float(-25.0, 25.0)) * (float) cos(angle[0]);
    amps_max[2] = (amps_avg + rnd_float(-25.0, 25.0)) * (float) cos(angle[1] - angle[0]);

    batt = rnd_float(3.8, 4.3);
    speed = rnd_float(1.0, 2.5);
    eclipse = (rnd_float(-1, +4) > 0) ? 1.0 : 0.0;
    period = rnd_float(150, 300);
    tempS = rnd_float(20, 55);
    temp_max = rnd_float(50, 70);
    temp_min = rnd_float(10, 20);

    #ifdef DEBUG_LOGGING
    for (int i = 0; i < 3; i++)
      printf("axis: %f angle: %f v: %f i: %f \n", axis[i], angle[i], volts_max[i], amps_max[i]);
    printf("batt: %f speed: %f eclipse_time: %f eclipse: %f period: %f temp: %f max: %f min: %f\n", batt, speed, eclipse_time, eclipse, period, tempS, temp_max, temp_min);
    #endif

    time_start = (long int) millis();

    eclipse_time = (long int)(millis() / 1000.0);
    if (eclipse == 0.0)
      eclipse_time -= period / 2; // if starting in eclipse, shorten interval	
  }

  tx_freq_hz -= tx_channel * 50000;

  if (transmit == FALSE) {

    fprintf(stderr, "\nNo CubeSatSim Band Pass Filter detected.  No transmissions after the CW ID.\n");
    fprintf(stderr, " See http://cubesatsim.org/wiki for info about building a CubeSatSim\n\n");
  }

// ============================================================================================================================================================================================================================================= 


// ============================================================================================================================================================================================================================================= 
// - CONFIGURACIÓN MODOS FSK Y BPSK
// ============================================================================================================================================================================================================================================= 

   if (mode == FSK) {
      bitRate = 200;
      rsFrames = 1;
      payloads = 1;
      rsFrameLen = 64;
      headerLen = 6;
      dataLen = 58;
      syncBits = 10;
      syncWord = 0b0011111010;
      parityLen = 32;
      amplitude = 32767 / 3;
      samples = S_RATE / bitRate;
      bufLen = (frameCnt * (syncBits + 10 * (headerLen + rsFrames * (rsFrameLen + parityLen))) * samples);

      samplePeriod =  (int) (((float)((syncBits + 10 * (headerLen + rsFrames * (rsFrameLen + parityLen)))) / (float) bitRate) * 1000 - 500);
      sleepTime = 0.1f;
	   
      frameTime = ((float)((float)bufLen / (samples * frameCnt * bitRate))) * 1000; // frame time in ms
	      
      printf("\n FSK Mode, %d bits per frame, %d bits per second, %d ms per frame, %d ms sample period\n",
        bufLen / (samples * frameCnt), bitRate, frameTime, samplePeriod);
    } else if (mode == BPSK) {
      bitRate = 1200;
      rsFrames = 3;
      payloads = 6;
      rsFrameLen = 159;
      headerLen = 8;
      dataLen = 78;
      syncBits = 31;
      syncWord = 0b1000111110011010010000101011101;
      parityLen = 32;
      amplitude = 32767;
      samples = S_RATE / bitRate;
      bufLen = (frameCnt * (syncBits + 10 * (headerLen + rsFrames * (rsFrameLen + parityLen))) * samples);

         samplePeriod = ((float)((syncBits + 10 * (headerLen + rsFrames * (rsFrameLen + parityLen))))/(float)bitRate) * 1000 - 1800;
      //    samplePeriod = 3000;
      //    sleepTime = 3.0;
      //samplePeriod = 2200; // reduce dut to python and sensor querying delays
      sleepTime = 2.2f;
	   
      frameTime = ((float)((float)bufLen / (samples * frameCnt * bitRate))) * 1000; // frame time in ms

      printf("\n BPSK Mode, bufLen: %d,  %d bits per frame, %d bits per second, %d ms per frame %d ms sample period\n",
        bufLen, bufLen / (samples * frameCnt), bitRate, frameTime, samplePeriod);
	   
      sin_samples = S_RATE/freq_Hz;	 		
//      printf("Sin map: ");	 		
      for (int j = 0; j < sin_samples; j++) {	 		
        sin_map[j] = (short int)(amplitude * sin((float)(2 * M_PI * j / sin_samples)));	 		
//	printf(" %d", sin_map[j]);	 		
      }	 		
      printf("\n");
   }

// ============================================================================================================================================================================================================================================= 

	
  memset(voltage, 0, sizeof(voltage));
  memset(current, 0, sizeof(current));
  memset(sensor, 0, sizeof(sensor));
  memset(other, 0, sizeof(other));	
	

// ============================================================================================================================================================================================================================================= 
// - 1ª LECTURA DE SENSORES E INICIALIZACIÓN DE VALORES
// ============================================================================================================================================================================================================================================= 

  if (((mode == FSK) || (mode == BPSK))) // && !sim_mode)
      get_tlm_fox();	// fill transmit buffer with reset count 0 packets that will be ignored
  firstTime = 1;
	  
//  if (!sim_mode)  // always read sensors, even in sim mode
  {
    strcpy(pythonStr, pythonCmd);
    strcat(pythonStr, busStr);
    strcat(pythonConfigStr, pythonStr);
    strcat(pythonConfigStr, " c");  

    fprintf(stderr, "pythonConfigStr: %s\n", pythonConfigStr);
	
    file1 = sopen(pythonConfigStr);  // python sensor polling function	  

    fgets(cmdbuffer, 1000, file1);
    fprintf(stderr, "pythonStr result: %s\n", cmdbuffer);
  }

  for (int i = 0; i < 9; i++) {
    voltage_min[i] = 1000.0;
    current_min[i] = 1000.0;
    voltage_max[i] = -1000.0;
    current_max[i] = -1000.0;
  }
  for (int i = 0; i < 17; i++) {
    sensor_min[i] = 1000.0;
    sensor_max[i] = -1000.0;
 //   printf("Sensor min and max initialized!");
  }
  for (int i = 0; i < 3; i++) {
    other_min[i] = 1000.0;
    other_max[i] = -1000.0;
  }
	
  long int loopTime;
  loopTime = millis();	
	
  while (loop-- != 0) {
    fflush(stdout);
    fflush(stderr);
//    frames_sent++;
	  
    sensor_payload[0] = 0;
    memset(voltage, 0, sizeof(voltage));
    memset(current, 0, sizeof(current));
    memset(sensor, 0, sizeof(sensor));
    memset(other, 0, sizeof(other));	
	  
    FILE * uptime_file = fopen("/proc/uptime", "r");
    fscanf(uptime_file, "%f", & uptime_sec);

    uptime = (int) (uptime_sec + 0.5);
//    printf("Uptime sec: %f \n", uptime_sec);	  
//    #ifdef DEBUG_LOGGING
    printf("INFO: Reset Count: %d Uptime since Reset: %ld \n", reset_count, uptime);
//    #endif
    fclose(uptime_file);
	  
    printf("++++ Loop time: %5.3f sec +++++\n", (millis() - loopTime)/1000.0);
    fflush(stdout);
    loopTime = millis();

   {
      int count1;
      char * token;
      fputc('\n', file1);
      fgets(cmdbuffer, 1000, file1);
      fprintf(stderr, "Python read Result: %s\n", cmdbuffer);

      const char space[2] = " ";
      token = strtok(cmdbuffer, space);

      for (count1 = 0; count1 < 8; count1++) {
        if (token != NULL) {
          voltage[count1] = (float) atof(token);
          #ifdef DEBUG_LOGGING
//            printf("voltage: %f ", voltage[count1]);
          #endif
          token = strtok(NULL, space);
          if (token != NULL) {
            current[count1] = (float) atof(token);
            if ((current[count1] < 0) && (current[count1] > -0.5))
              current[count1] *= (-1.0f);
            #ifdef DEBUG_LOGGING
//              printf("current: %f\n", current[count1]);
            #endif
            token = strtok(NULL, space);
          }
        }
        if (voltage[map[BAT]] == 0.0)
		batteryVoltage = 4.5;
	else
		batteryVoltage = voltage[map[BAT]];
        batteryCurrent = current[map[BAT]];
	   
   }  

// ============================================================================================================================================================================================================================================= 

// ============================================================================================================================================================================================================================================= 
// - GENERACIÓN DE SIMULACIÓN PARA TESTEAR CON VALORES REALES
// ============================================================================================================================================================================================================================================= 

    if (sim_mode) { // simulated telemetry 

      double time = ((long int)millis() - time_start) / 1000.0;

      if ((time - eclipse_time) > period) {
        eclipse = (eclipse == 1) ? 0 : 1;
        eclipse_time = time;
        printf("\n\nSwitching eclipse mode! \n\n");
      }

      double Xi = eclipse * amps_max[0] * (float) sin(2.0 * 3.14 * time / (46.0 * speed)) + rnd_float(-2, 2);
      double Yi = eclipse * amps_max[1] * (float) sin((2.0 * 3.14 * time / (46.0 * speed)) + (3.14 / 2.0)) + rnd_float(-2, 2);
      double Zi = eclipse * amps_max[2] * (float) sin((2.0 * 3.14 * time / (46.0 * speed)) + 3.14 + angle[2]) + rnd_float(-2, 2);

      double Xv = eclipse * volts_max[0] * (float) sin(2.0 * 3.14 * time / (46.0 * speed)) + rnd_float(-0.2, 0.2);
      double Yv = eclipse * volts_max[1] * (float) sin((2.0 * 3.14 * time / (46.0 * speed)) + (3.14 / 2.0)) + rnd_float(-0.2, 0.2);
      double Zv = 2.0 * eclipse * volts_max[2] * (float) sin((2.0 * 3.14 * time / (46.0 * speed)) + 3.14 + angle[2]) + rnd_float(-0.2, 0.2);

      // printf("Yi: %f Zi: %f %f %f Zv: %f \n", Yi, Zi, amps_max[2], angle[2], Zv);

      current[map[PLUS_X]] = (Xi >= 0) ? Xi : 0;
      current[map[MINUS_X]] = (Xi >= 0) ? 0 : ((-1.0f) * Xi);
      current[map[PLUS_Y]] = (Yi >= 0) ? Yi : 0;
      current[map[MINUS_Y]] = (Yi >= 0) ? 0 : ((-1.0f) * Yi);
      current[map[PLUS_Z]] = (Zi >= 0) ? Zi : 0;
      current[map[MINUS_Z]] = (Zi >= 0) ? 0 : ((-1.0f) * Zi);

      voltage[map[PLUS_X]] = (Xv >= 1) ? Xv : rnd_float(0.9, 1.1);
      voltage[map[MINUS_X]] = (Xv <= -1) ? ((-1.0f) * Xv) : rnd_float(0.9, 1.1);
      voltage[map[PLUS_Y]] = (Yv >= 1) ? Yv : rnd_float(0.9, 1.1);
      voltage[map[MINUS_Y]] = (Yv <= -1) ? ((-1.0f) * Yv) : rnd_float(0.9, 1.1);
      voltage[map[PLUS_Z]] = (Zv >= 1) ? Zv : rnd_float(0.9, 1.1);
      voltage[map[MINUS_Z]] = (Zv <= -1) ? ((-1.0f) * Zv) : rnd_float(0.9, 1.1);

      // printf("temp: %f Time: %f Eclipse: %d : %f %f | %f %f | %f %f\n",tempS, time, eclipse, voltage[map[PLUS_X]], voltage[map[MINUS_X]], voltage[map[PLUS_Y]], voltage[map[MINUS_Y]], current[map[PLUS_Z]], current[map[MINUS_Z]]);

      tempS += (eclipse > 0) ? ((temp_max - tempS) / 50.0f) : ((temp_min - tempS) / 50.0f);
      tempS += +rnd_float(-1.0, 1.0);
      //  IHUcpuTemp = (int)((tempS + rnd_float(-1.0, 1.0)) * 10 + 0.5);
      other[IHU_TEMP] = tempS;

      voltage[map[BUS]] = rnd_float(5.0, 5.005);
      current[map[BUS]] = rnd_float(158, 171);

      //  float charging = current[map[PLUS_X]] + current[map[MINUS_X]] + current[map[PLUS_Y]] + current[map[MINUS_Y]] + current[map[PLUS_Z]] + current[map[MINUS_Z]];
      float charging = eclipse * (fabs(amps_max[0] * 0.707) + fabs(amps_max[1] * 0.707) + rnd_float(-4.0, 4.0));

      current[map[BAT]] = ((current[map[BUS]] * voltage[map[BUS]]) / batt) - charging;

      //  printf("charging: %f bat curr: %f bus curr: %f bat volt: %f bus volt: %f \n",charging, current[map[BAT]], current[map[BUS]], batt, voltage[map[BUS]]);

      batt -= (batt > 3.5) ? current[map[BAT]] / 30000 : current[map[BAT]] / 3000;
      if (batt < 3.0) {
        batt = 3.0;
        SafeMode = 1;
        printf("Safe Mode!\n");
      } else
        SafeMode= 0;

      if (batt > 4.5)
        batt = 4.5;

      voltage[map[BAT]] = batt + rnd_float(-0.01, 0.01);

      // end of simulated telemetry
    }
    else {
// code moved


/*	    
      int count1;
      char * token;
      fputc('\n', file1);
      fgets(cmdbuffer, 1000, file1);
      fprintf(stderr, "Python read Result: %s\n", cmdbuffer);

      const char space[2] = " ";
      token = strtok(cmdbuffer, space);

      for (count1 = 0; count1 < 8; count1++) {
        if (token != NULL) {
          voltage[count1] = (float) atof(token);
          #ifdef DEBUG_LOGGING
//            printf("voltage: %f ", voltage[count1]);
          #endif
          token = strtok(NULL, space);
          if (token != NULL) {
            current[count1] = (float) atof(token);
            if ((current[count1] < 0) && (current[count1] > -0.5))
              current[count1] *= (-1.0f);
            #ifdef DEBUG_LOGGING
//              printf("current: %f\n", current[count1]);
            #endif
            token = strtok(NULL, space);
          }
        }
        if (voltage[map[BAT]] == 0.0)
		batteryVoltage = 4.5;
	else
		batteryVoltage = voltage[map[BAT]];
        batteryCurrent = current[map[BAT]];
*/	      
      }
	
//      batteryVoltage = voltage[map[BAT]];
//      batteryCurrent = current[map[BAT]];
	    
      if (batteryVoltage < 3.7) {
        SafeMode = 1;
        printf("Safe Mode!\n");
      } else
        SafeMode = 0;

      FILE * cpuTempSensor = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
      if (cpuTempSensor) {
   //     double cpuTemp;
        fscanf(cpuTempSensor, "%lf", & cpuTemp);
        cpuTemp /= 1000;

        #ifdef DEBUG_LOGGING
//        printf("CPU Temp Read: %6.1f\n", cpuTemp);
        #endif

        other[IHU_TEMP] = (double)cpuTemp;

      //  IHUcpuTemp = (int)((cpuTemp * 10.0) + 0.5);
      }
      fclose(cpuTempSensor);
    } 

// ============================================================================================================================================================================================================================================= 

// ============================================================================================================================================================================================================================================= 
// - PROCESA DATOS RECIBIDOS DESDE PAYLOAD (extrae datos de sensores de la placa)
// ============================================================================================================================================================================================================================================= 

	  
      if (payload == ON) {  // -55
        STEMBoardFailure = 0;
        printf("get_payload_status: %d \n", get_payload_serial(FALSE));  // not debug // RECUPERA DATOS DE TELEMETRÍA O DE ESTADO
	fflush(stdout); 
	printf("String: %s\n", buffer2);       
	fflush(stdout);   
	      
	strcpy(sensor_payload, buffer2);      
/*  
        char c;
        unsigned int waitTime;
	int i, end, trys = 0;
	sensor_payload[0] = 0;
	sensor_payload[1] = 0;
	while (((sensor_payload[0] != 'O') || (sensor_payload[1] != 'K')) && (trys++ < 10)) {	      
          i = 0;
//	  serialPutchar(uart_fd, '?');
//	  sleep(0.05);  // added delay after ?
//          printf("%d Querying payload with ?\n", trys);
          waitTime = millis() + 500;
          end = FALSE;
          //  int retry = FALSE;
          while ((millis() < waitTime) && !end) {
            int chars = (char) serialDataAvail(uart_fd);
            while ((chars > 0) && !end) {
//	      printf("Chars: %d\ ", chars);
	      chars--;
	      c = (char) serialGetchar(uart_fd);
              //	printf ("%c", c);
              //	fflush(stdout);
              if (c != '\n') {
                sensor_payload[i++] = c;
              } else {
                end = TRUE;
              }
            }
          }
	
          sensor_payload[i++] = ' ';
          //  sensor_payload[i++] = '\n';
          sensor_payload[i] = '\0';
          printf(" Response from STEM Payload board: %s\n", sensor_payload);
		
		
	  sleep(0.1);  // added sleep between loops
	}
*/	  
	printf(" Response from STEM Payload board: %s\n", sensor_payload);
      
        if ((sensor_payload[0] == 'O') && (sensor_payload[1] == 'K')) // only process if valid payload response
        {
          int count1;
          char * token;
 
          const char space[2] = " ";
          token = strtok(sensor_payload, space);
//	  printf("token: %s\n", token);	
          for (count1 = 0; count1 < 17; count1++) {
            if (token != NULL) {
              sensor[count1] = (float) atof(token);
//              #ifdef DEBUG_LOGGING
                printf("sensor: %f ", sensor[count1]);  // LEE LOS DATOS DE LOS SENSORES
//              #endif
              token = strtok(NULL, space);
            }
          }
          printf("\n");
//	  if (sensor[XS1] != 0) {     		
	  if ((sensor[XS1] > -90.0) && (sensor[XS1] < 90.0) && (sensor[XS1] != 0.0))  { 
		if (sensor[XS1] != latitude) {  
			latitude = sensor[XS1];  
			printf("Latitude updated to %f \n", latitude); 
			newGpsTime = millis();  
	 	}
	  }
//	  if (sensor[XS2] != 0)  {
	  if ((sensor[XS2] > -180.0) && (sensor[XS2] < 180.0) && (sensor[XS2] != 0.0))  {   
		if (sensor[XS2] != longitude) {  		  
			longitude = sensor[XS2];  
			printf("Longitude updated to %f \n", longitude); 
			newGpsTime = millis();  
		}
	  }
        }
	else
		; //payload = OFF;  // turn off since STEM Payload is not responding
      }
      if ((millis() - newGpsTime) > 60000) {
		longitude += rnd_float(-0.05, 0.05) / 100.0;  // was .05
     		latitude += rnd_float(-0.05, 0.05) / 100.0;	      
       		printf("GPS Location with Rnd: %f, %f \n", latitude, longitude);    
	        printf("GPS Location with Rnd: APRS %07.2f, %08.2f \n", toAprsFormat(latitude), toAprsFormat(longitude));    
	      	newGpsTime = millis();  
      }
	  
      if ((sensor_payload[0] == 'O') && (sensor_payload[1] == 'K')) {
        for (int count1 = 0; count1 < 17; count1++) {
          if (sensor[count1] < sensor_min[count1])
            sensor_min[count1] = sensor[count1];
          if (sensor[count1] > sensor_max[count1])
            sensor_max[count1] = sensor[count1];
            //  printf("Smin %f Smax %f \n", sensor_min[count1], sensor_max[count1]);
        }
      }
//  }	  
	  
    #ifdef DEBUG_LOGGING
    fprintf(stderr, "INFO: Battery voltage: %5.2f V  Threshold %5.2f V Current: %6.1f mA Threshold: %6.1f mA\n", batteryVoltage, voltageThreshold, batteryCurrent, currentThreshold);
    #endif
//    if ((batteryVoltage > 1.0) && (batteryVoltage < batteryThreshold)) // no battery INA219 will give 0V, no battery plugged into INA219 will read < 1V
// fprintf(stderr, "\n\nbattery_saver_mode : %d current: %f\n", battery_saver_mode, batteryCurrent);

//#ifndef HAB
	  
    if ((batteryCurrent > currentThreshold) && (batteryVoltage < (voltageThreshold + 0.15)) && !sim_mode && !hab_mode)
    {
	    fprintf(stderr,"Battery voltage low - switch to battery saver\n");
	    if (battery_saver_mode == OFF)
	    	battery_saver(ON);
    } else if ((battery_saver_mode == ON) && (batteryCurrent < 0) && !sim_mode && !hab_mode)
    {
	    fprintf(stderr,"Battery is being charged - switch battery saver off\n");
	    if (battery_saver_mode == ON)
	    	 battery_saver(OFF);
    } 
    if ((batteryCurrent > currentThreshold) && (batteryVoltage < voltageThreshold) && !sim_mode && !hab_mode) // currentThreshold ensures that this won't happen when running on DC power.
    {
      fprintf(stderr, "Battery voltage too low: %f V - shutting down!\n", batteryVoltage);
      digitalWrite(txLed, txLedOff);
      digitalWrite(onLed, onLedOff);
      
      sleep(1);
      digitalWrite(onLed, onLedOn);
      sleep(1);
      digitalWrite(onLed, onLedOff);
      sleep(1);
      digitalWrite(onLed, onLedOn);
      sleep(1);
      digitalWrite(onLed, onLedOff);

      FILE * file6; // = popen("/home/pi/CubeSatSim/log > shutdown_log.txt", "r");
//      pclose(file6);
//      sleep(80);	    
      file6 = popen("sudo shutdown -h now > /dev/null 2>&1", "r");
      pclose(file6);
      sleep(10);
    }
//#endif
// ============================================================================================================================================================================================================================================= 

	  
// ============================================================================================================================================================================================================================================= 
// - WRITING TELEMETRY ON TEXT FILE
// ============================================================================================================================================================================================================================================= 

    FILE * fp = fopen("/home/pi/CubeSatSim/telem_string.txt", "w");
    if (fp != NULL)  {	  
    	printf("Writing telem_string.txt\n");
	if (batteryVoltage != 4.5)    
    		fprintf(fp, "BAT %4.2fV %5.1fmA\n", batteryVoltage, batteryCurrent);	
	else
    		fprintf(fp, "\n");	// don't show voltage and current if it isn't a sensor value
		
    	fclose(fp);	 
    } else 
	    printf("Error writing to telem_string.txt\n");

// ============================================================================================================================================================================================================================================= 

	    
/**/
    //  sleep(1);  // Delay 1 second
    ctr = 0;
    #ifdef DEBUG_LOGGING
//    fprintf(stderr, "INFO: Getting TLM Data\n");
    #endif

// ============================================================================================================================================================================================================================================= 
// - TRANSMISIÓN TELEMETRÍA SEGÚN MODO DE OPERACIÓN
// ============================================================================================================================================================================================================================================= 


    if ((mode == AFSK) || (mode == CW)) {
      get_tlm();
      sleep(25);
      fprintf(stderr, "INFO: Sleeping for 25 sec\n");	 
	    
      int rand_sleep = (int)rnd_float(0.0, 5.0);	    
      sleep(rand_sleep);	    
      fprintf(stderr, "INFO: Sleeping for extra %d sec\n", rand_sleep);	  
	    
    } else if ((mode == FSK) || (mode == BPSK)) {// FSK or BPSK
      get_tlm_fox();
    } else {  				// SSTV	    
      fprintf(stderr, "Sleeping\n");
      sleep(50);	    
    }

    #ifdef DEBUG_LOGGING
//    fprintf(stderr, "INFO: Getting ready to send\n");
    #endif
  }

  if (mode == BPSK) {
//    digitalWrite(txLed, txLedOn);
    #ifdef DEBUG_LOGGING
//    printf("Tx LED On 1\n");
    #endif
    printf("Sleeping to allow BPSK transmission to finish.\n");
    sleep((unsigned int)(loop_count * 5));
    printf("Done sleeping\n");
//    digitalWrite(txLed, txLedOff);
    #ifdef DEBUG_LOGGING
//    printf("Tx LED Off\n");
    #endif
  } else if (mode == FSK) {
    printf("Sleeping to allow FSK transmission to finish.\n");
    sleep((unsigned int)loop_count);
    printf("Done sleeping\n");
  }

  return 0;
}

// ============================================================================================================================================================================================================================================= 

////////////////// ============================================================================================================================================================================================================================================= 
// - FUNCIONES EMPLEADAS EN EL CÓDIGO
////////////////// ============================================================================================================================================================================================================================================= 


//  Returns lower digit of a number which must be less than 99
//
int lower_digit(int number) {

  int digit = 0;
  if (number < 100)
    digit = number - ((int)(number / 10) * 10);
  else
    fprintf(stderr, "ERROR: Not a digit in lower_digit!\n");
  return digit;
}

// Returns upper digit of a number which must be less than 99
//
int upper_digit(int number) {

  int digit = 0;
  if (number < 100)

    digit = (int)(number / 10);
  else
    fprintf(stderr, "ERROR: Not a digit in upper_digit!\n");
  return digit;
}

static int init_rf() { // Configura módulo de radiofrecuencia AX5043
  int ret;
  fprintf(stderr, "Initializing AX5043\n");

  ret = ax5043_init( & hax5043, XTAL_FREQ_HZ, VCO_INTERNAL);
  if (ret != PQWS_SUCCESS) {
    fprintf(stderr,
      "ERROR: Failed to initialize AX5043 with error code %d\n", ret);
    //       exit(EXIT_FAILURE);
    return (0);
  }
  return (1);
}

// ============================================================================================================================================================================================================================================= 
// - ENVIO DATOS TELEMETRIA EN CW Y APSK Y DISTINTOS MODOS (NO FOX TELEM)
// ============================================================================================================================================================================================================================================= 

/* Esta función get_tlm() se encarga de formatear y transmitir la telemetría del CubeSat en diferentes formatos, 
principalmente para radioaficionados usando el protocolo APRS (Automatic Packet Reporting System). 
Aquí sus principales funcionalidades:

Formateo de Datos:
  Convierte las mediciones de los sensores en formato de telemetría legible:
    Voltajes de bus y baterías
    Corrientes de los paneles solares (X, Y, Z)
    Temperatura
    Otros parámetros del sistema

Múltiples Modos de Transmisión:
  Soporta varios modos de operación:
    CW (Morse)
    AFSK (Audio Frequency Shift Keying para APRS)
    Transmisión digital usando el chip AX5043
    APRS con coordenadas geográficas

Gestión de Paquetes APRS:
  Construye paquetes APRS con:
    Indicativo del satélite
    Coordenadas (latitud/longitud)
    Datos de telemetría
    Información de estado

Sistema de Registro:
  Guarda los datos de telemetría en archivos locales
  Registra timestamps con las mediciones
  Mantiene un log de las transmisiones

Control de Hardware:
  Maneja el LED de transmisión
  Controla el hardware de radio (AX5043 o rpitx)
  Gestiona los tiempos entre transmisiones

Características de Seguridad:
  Modo de ahorro de batería
  Verificación de filtro paso banda antes de transmitir
  Reintentos en caso de fallos de transmisión

La diferencia principal con la siguiente función es que mientras get_tlm_fox() se centra en el protocolo Fox usado en
algunos CubeSats de AMSAT, esta función get_tlm() está más orientada a la transmisión de telemetría vía APRS, que es un protocolo 
más común entre radioaficionados y permite que más operadores en tierra puedan recibir y decodificar los datos del satélite.
También es notable que esta función está diseñada para ser más accesible para los radioaficionados, ya que usa formatos 
estándar como CW (Morse) y APRS, que pueden ser recibidos con equipamiento más común, mientras que el protocolo Fox requiere 
software específico para su decodificación */

void get_tlm(void) {

  FILE * txResult;

  for (int j = 0; j < frameCnt; j++) {
	  
    fflush(stdout);
    fflush(stderr);
	  
    int tlm[7][5];
    memset(tlm, 0, sizeof tlm);
	  
    tlm[1][A] = (int)(voltage[map[BUS]] / 15.0 + 0.5) % 100; // Current of 5V supply to Pi
    // ajusta el voltaje del bus dividiéndolo por un factor y limitándolo a dos dígitos
    tlm[1][B] = (int)(99.5 - current[map[PLUS_X]] / 10.0) % 100; // +X current [4]
    tlm[1][C] = (int)(99.5 - current[map[MINUS_X]] / 10.0) % 100; // X- current [10] 
    tlm[1][D] = (int)(99.5 - current[map[PLUS_Y]] / 10.0) % 100; // +Y current [7]

    tlm[2][A] = (int)(99.5 - current[map[MINUS_Y]] / 10.0) % 100; // -Y current [10] 
    tlm[2][B] = (int)(99.5 - current[map[PLUS_Z]] / 10.0) % 100; // +Z current [10] // was 70/2m transponder power, AO-7 didn't have a Z panel
    tlm[2][C] = (int)(99.5 - current[map[MINUS_Z]] / 10.0) % 100; // -Z current (was timestamp)
    tlm[2][D] = (int)(50.5 + current[map[BAT]] / 10.0) % 100; // NiMH Battery current

//    tlm[3][A] = abs((int)((voltage[map[BAT]] * 10.0) - 65.5) % 100);
    if (voltage[map[BAT]] > 4.6)	 
    	tlm[3][A] = (int)((voltage[map[BAT]] * 10.0) - 65.5) % 100;  // 7.0 - 10.0 V for old 9V battery
    else
    	tlm[3][A] = (int)((voltage[map[BAT]] * 10.0) + 44.5) % 100;  // 0 - 4.5 V for new 3 cell battery
	    
    tlm[3][B] = (int)(voltage[map[BUS]] * 10.0) % 100; // 5V supply to Pi

    tlm[4][A] = (int)((95.8 - other[IHU_TEMP]) / 1.48 + 0.5) % 100;  // was [B] but didn't display in online TLM spreadsheet
		
    tlm[6][B] = 0;
    tlm[6][D] = 49 + rand() % 3;
/*
    #ifdef DEBUG_LOGGING
    // Display tlm
    int k, j;
    for (k = 1; k < 7; k++) {
      for (j = 1; j < 5; j++) {
        printf(" %2d ", tlm[k][j]);
      }
      printf("\n");
    }
    #endif
*/	  

    char str[1000];
    char tlm_str[1000];
    char header_str[] = "\x03\xf0"; // hi hi ";
    char header_str3[] = "echo '";
    char header_str2[] = "-11>APCSS:";
    char header_str2b[30]; // for APRS coordinates
    char header_lat[10];
    char header_long[10];
    char header_str4[] = "hi hi ";
//    char footer_str1[] = "\' > t.txt && echo \'";
    char footer_str1[] = "\' > t.txt";
//    char footer_str[] = "-11>APCSS:010101/hi hi ' >> t.txt && touch /home/pi/CubeSatSim/ready";  // transmit is done by rpitx.py
    char footer_str[] = " && echo 'AMSAT-11>APCSS:010101/hi hi ' >> t.txt && touch /home/pi/CubeSatSim/ready";  // transmit is done by rpitx.py
    char footer_str2[] = " && touch /home/pi/CubeSatSim/ready";  
	  
    if (ax5043) {
      strcpy(str, header_str);
    } else {
      strcpy(str, header_str3);
//    }
      if (mode == AFSK) {
        strcat(str, call);
        strcat(str, header_str2);	    
      }	    
    }
//      printf("Str: %s \n", str);
      if (mode != CW) {
         //	sprintf(header_str2b, "=%7.2f%c%c%c%08.2f%cShi hi ",4003.79,'N',0x5c,0x5c,07534.33,'W');  // add APRS lat and long
        if (latitude > 0)
          sprintf(header_lat, "%07.2f%c", toAprsFormat(latitude), 'N'); // lat
        else
          sprintf(header_lat, "%07.2f%c", toAprsFormat(latitude) * (-1.0), 'S'); // lat
        if (longitude > 0)
          sprintf(header_long, "%08.2f%c", toAprsFormat(longitude) , 'E'); // long
        else
          sprintf(header_long, "%08.2f%c",toAprsFormat( longitude) * (-1.0), 'W'); // long
	      
        if (ax5043)
          sprintf(header_str2b, "=%s%c%sShi hi ", header_lat, 0x5c, header_long); // add APRS lat and long	    
        else
//#ifdef HAB
	if (hab_mode)	
		sprintf(header_str2b, "=%s%c%sOhi hi ", header_lat, 0x2f, header_long); // add APRS lat and long with Balloon HAB icon
//#else
	else
		sprintf(header_str2b, "=%s%c%c%sShi hi ", header_lat, 0x5c, 0x5c, header_long); // add APRS lat and long with Satellite icon	
//#endif		
          	           	    
        printf("\n\nString is %s \n\n", header_str2b);
        strcat(str, header_str2b);
      } else {
        strcat(str, header_str4);
      }
//    }
	printf("Str: %s \n", str);
   if (mode == CW) {
    int channel;
    for (channel = 1; channel < 7; channel++) {
      sprintf(tlm_str, "%d%d%d %d%d%d %d%d%d %d%d%d ",
        channel, upper_digit(tlm[channel][1]), lower_digit(tlm[channel][1]),
        channel, upper_digit(tlm[channel][2]), lower_digit(tlm[channel][2]),
        channel, upper_digit(tlm[channel][3]), lower_digit(tlm[channel][3]),
        channel, upper_digit(tlm[channel][4]), lower_digit(tlm[channel][4]));
      //        printf("%s",tlm_str);

//#ifdef HAB	    
//       if (mode != AFSK)
//#endif	
 //      if ((!hab_mode) || ((hab_mode) && (mode != AFSK)))       
         strcat(str, tlm_str);

    }
  } else {  // APRS
//#ifdef HAB
//    if ((mode == AFSK) && (hab_mode)) {
//      sprintf(tlm_str, "BAT %4.2f %5.1f ", batteryVoltage, batteryCurrent); 
      sprintf(tlm_str, "BAT %4.2f %5.1f ", voltage[map[BAT]] , current[map[BAT]] ); 
      strcat(str, tlm_str);
//    }  else
//      strcat(str, tlm_str);	// Is this needed???
   }  
//#endif
    // read payload sensor if available
/*
    char sensor_payload[500];

    if (payload == ON) {
      char c;
      unsigned int waitTime;
      int i, end, trys = 0;
      sensor_payload[0] = 0;
      sensor_payload[1] = 0;
      while (((sensor_payload[0] != 'O') || (sensor_payload[1] != 'K')) && (trys++ < 10)) {	      
        i = 0;
	serialPutchar(uart_fd, '?');
	sleep(0.05);  // added delay after ?
        printf("%d Querying payload with ?\n", trys);
        waitTime = millis() + 500;
        end = FALSE;
        //  int retry = FALSE;
        while ((millis() < waitTime) && !end) {
          int chars = (char) serialDataAvail(uart_fd);
          while ((chars > 0) && !end) {
//	    printf("Chars: %d\ ", chars);
	    chars--;
	    c = (char) serialGetchar(uart_fd);
            //	printf ("%c", c);
            //	fflush(stdout);
            if (c != '\n') {
              sensor_payload[i++] = c;
            } else {
              end = TRUE;
            }
          }
        }
	
        sensor_payload[i++] = ' ';
        //  sensor_payload[i++] = '\n';
        sensor_payload[i] = '\0';
	
        printf(" Response from STEM Payload board: %s\n", sensor_payload);
	sleep(0.1);  // added sleep between loops
      }	    

      if (mode != CW)
        strcat(str, sensor_payload); // append to telemetry string for transmission
    }
*/
    strcpy(sensor_payload, buffer2);      	  
    printf(" Response from STEM Payload board:: %s\n", sensor_payload);
//    printf(" Str so far: %s\n", str);   
	  
    if (mode != CW) {
        strcat(str, sensor_payload); // append to telemetry string for transmission
//	printf(" Str so far: %s\n", str);    
    }
    if (mode == CW) {

      char cw_str2[1000];
      char cw_header2[] = "echo '";
      char cw_footer2[] = "' > id.txt && gen_packets -M 20 id.txt -o morse.wav -r 48000 > /dev/null 2>&1 && cat morse.wav | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo /home/pi/rpitx/rpitx -i- -m RF -f 434.897e3";
      char cw_footer3[] = "' > cw.txt && touch /home/pi/CubeSatSim/cwready";  // transmit is done by rpitx.py

//    printf("Str str: %s \n", str);
//    fflush(stdout);
      strcat(str, cw_footer3);
//    printf("Str: %s \n", str);
//    fflush(stdout);	    
      printf("CW string to execute: %s\n", str);
      fflush(stdout);

      FILE * cw_file = popen(str, "r");
      pclose(cw_file);	    
	    
      while ((cw_file = fopen("/home/pi/CubeSatSim/cwready", "r")) != NULL) {  // wait for rpitx  to be done
        fclose(cw_file); 
//	printf("Sleeping while waiting for rpitx \n");
//	fflush(stdout);      
        sleep(5);	      
      }    
    } 
    else if (ax5043) {
      digitalWrite(txLed, txLedOn);
      fprintf(stderr, "INFO: Transmitting X.25 packet using AX5043\n");
      memcpy(data, str, strnlen(str, 256));
      printf("data: %s \n", data);	    
      int ret = ax25_tx_frame( & hax25, & hax5043, data, strnlen(str, 256));
      if (ret) {
        fprintf(stderr,
          "ERROR: Failed to transmit AX.25 frame with error code %d\n",
          ret);
        exit(EXIT_FAILURE);
      }
      ax5043_wait_for_transmit();
      digitalWrite(txLed, txLedOff);

      if (ret) {
        fprintf(stderr,
          "ERROR: Failed to transmit entire AX.25 frame with error code %d\n",
          ret);
        exit(EXIT_FAILURE);
      }
      sleep(4);  // was 2
	    
    } else {  // APRS using rpitx
	    
     if (payload == ON) {	    
      telem_file = fopen("/home/pi/CubeSatSim/telem.txt", "a");
      printf("Writing payload string\n");
      time_t timeStamp;
      time(&timeStamp);   // get timestamp 
//      printf("Timestamp: %s\n", ctime(&timeStamp));
	    
      char timeStampNoNl[31], bat_string[31];    
      snprintf(timeStampNoNl, 30, "%.24s", ctime(&timeStamp)); 
      printf("TimeStamp: %s\n", timeStampNoNl);
      snprintf(bat_string, 30, "BAT %4.2f %5.1f", batteryVoltage, batteryCurrent);	     
      fprintf(telem_file, "%s %s %s\n", timeStampNoNl, bat_string, sensor_payload);	 // write telemetry string to telem.txt file    
      fclose(telem_file);
    }	 	    
	    
      strcat(str, footer_str1);
//      strcat(str, call);
      if (battery_saver_mode == ON)	    
      	strcat(str, footer_str);  // add extra packet for rpitx transmission
      else
      	strcat(str, footer_str2);
	    
      fprintf(stderr, "String to execute: %s\n", str);
	    
      printf("\n\nTelemetry string is %s \n\n", str);	
	    
      if (transmit) {
        FILE * file2 = popen(str, "r");
        pclose(file2);
	      
	sleep(2);
	digitalWrite(txLed, txLedOff);
      
      } else {
        fprintf(stderr, "\nNo CubeSatSim Band Pass Filter detected.  No transmissions after the CW ID.\n");
        fprintf(stderr, " See http://cubesatsim.org/wiki for info about building a CubeSatSim\n\n");
      }

      sleep(3);
    }

  }

  return;
}
// ============================================================================================================================================================================================================================================= 


// ============================================================================================================================================================================================================================================= 
// - ENVIO DATOS TELEMETRIA EN BPSK Y FSK (NECESITA PROGRAMA PARA DECODIFICACION -> FOX TELEM)
// ============================================================================================================================================================================================================================================= 

/* Esta función get_tlm_fox() es una parte crítica del software de un CubeSat de radioaficionados, específicamente parece estar 
relacionada con el sistema de telemetría. Voy a explicar sus principales funcionalidades:

Procesamiento de Telemetría:
  Recolecta datos de varios sensores del CubeSat incluyendo:
    Voltajes y corrientes de los paneles solares (X, Y, Z, tanto positivos como negativos)
    Voltaje y corriente de la batería
    Datos del bus de energía (PSU)
    Acelerómetros en los 3 ejes
    Giroscopios
    Temperatura
    Presión
    Altitud
    Humedad
    RSSI (intensidad de señal)

Modos de Transmisión:
  Soporta dos modos de transmisión:
    FSK (Frequency Shift Keying)
    BPSK (Binary Phase Shift Keying)

Estructura de Trama:
  Construye tramas de datos que incluyen:
    Cabecera con información de identificación
    Datos de telemetría actuales
    En modo BPSK, también incluye valores máximos y mínimos de los sensores
    Códigos de corrección de errores (Reed-Solomon)
    Codificación 8b10b para la transmisión

Estado del Sistema:
  Monitorea varios estados del CubeSat como:
    Despliegue de antenas (TX y RX)
    Modo seguro
    Fallos en diferentes subsistemas
    Conteo de comandos recibidos desde tierra
    Conteo de reinicios

Transmisión:
  Genera las formas de onda para la transmisión de radio
  Utiliza un socket para enviar los datos al hardware de transmisión
  Implementa delays y tiempos de espera para mantener el ciclo de transmisión
  Incluye mecanismos de reintento si la transmisión falla

Control de Energía:
  Monitorea el estado de los paneles solares
  Supervisa el sistema de energía del satélite
  Registra valores máximos y mínimos de voltajes y corrientes

Este código es típico de satélites amateurs tipo Fox o similares, donde es crucial mantener informados a los 
radioaficionados en tierra sobre el estado del satélite. La telemetría permite a los operadores en tierra 
monitorear la salud del satélite y detectar cualquier problema potencial */

void get_tlm_fox() {

  int i;

  long int sync = syncWord;

  smaller = (int) (S_RATE / (2 * freq_Hz));

  short int b[dataLen];
  short int b_max[dataLen];
  short int b_min[dataLen];
	
  memset(b, 0, sizeof(b));
  memset(b_max, 0, sizeof(b_max));
  memset(b_min, 0, sizeof(b_min));
	
  short int h[headerLen];
  memset(h, 0, sizeof(h));

  memset(buffer, 0xa5, sizeof(buffer));

  short int rs_frame[rsFrames][223];
  unsigned char parities[rsFrames][parityLen], inputByte;

  int id, frm_type = 0x01, NormalModeFailure = 0, groundCommandCount = 0;
  int PayloadFailure1 = 0, PayloadFailure2 = 0;
  int PSUVoltage = 0, PSUCurrent = 0, Resets = 0, Rssi = 2048;
  int batt_a_v = 0, batt_b_v = 0, batt_c_v = 0, battCurr = 0;
  int posXv = 0, negXv = 0, posYv = 0, negYv = 0, posZv = 0, negZv = 0;
  int posXi = 0, negXi = 0, posYi = 0, negYi = 0, posZi = 0, negZi = 0;
  int head_offset = 0;

  short int buffer_test[bufLen];
  int buffSize;
  buffSize = (int) sizeof(buffer_test);
	
  if (mode == FSK)
    id = 7;
  else
    id = 0; // 99 in h[6]
	
  //  for (int frames = 0; frames < FRAME_CNT; frames++) 
  for (int frames = 0; frames < frameCnt; frames++) {
  
    if (firstTime != ON) {
      // delay for sample period

/**/
//      while ((millis() - sampleTime) < (unsigned int)samplePeriod)
     int startSleep = millis();	    
     if ((millis() - sampleTime) < ((unsigned int)frameTime - 250))  // was 250 100 500 for FSK
        sleep(2.0); // 0.5);  // 25);  // initial period
     while ((millis() - sampleTime) < ((unsigned int)frameTime - 250))  // was 250 100
        sleep(0.1); // 25); // 0.5);  // 25);
//        sleep((unsigned int)sleepTime);
/**/
      printf("Sleep period: %d\n", millis() - startSleep);
      fflush(stdout);
      
      sampleTime = (unsigned int) millis();
    } else
      printf("first time - no sleep\n");
	
//    if (mode == FSK) 
    {  // just moved
      for (int count1 = 0; count1 < 8; count1++) {
        if (voltage[count1] < voltage_min[count1])
          voltage_min[count1] = voltage[count1];
        if (current[count1] < current_min[count1])
          current_min[count1] = current[count1];
	      
        if (voltage[count1] > voltage_max[count1])
          voltage_max[count1] = voltage[count1];
        if (current[count1] > current_max[count1])
          current_max[count1] = current[count1];

//         printf("Vmin %4.2f Vmax %4.2f Imin %4.2f Imax %4.2f \n", voltage_min[count1], voltage_max[count1], current_min[count1], current_max[count1]);
      }
       for (int count1 = 0; count1 < 3; count1++) {
        if (other[count1] < other_min[count1])
          other_min[count1] = other[count1];
        if (other[count1] > other_max[count1])
          other_max[count1] = other[count1];

        //  printf("Other min %f max %f \n", other_min[count1], other_max[count1]);
      }
      	  if (mode == FSK)
	  {
	      if (loop % 32 == 0) {  // was 8
		printf("Sending MIN frame \n");
		frm_type = 0x03;
		for (int count1 = 0; count1 < 17; count1++) {
		  if (count1 < 3)
		    other[count1] = other_min[count1];
		  if (count1 < 8) {
		    voltage[count1] = voltage_min[count1];
		    current[count1] = current_min[count1];
		  }
		  if (sensor_min[count1] != 1000.0) // make sure values are valid
		    sensor[count1] = sensor_min[count1];
		}
	      }
	      if ((loop + 16) % 32 == 0) {  // was 8
		printf("Sending MAX frame \n");
		frm_type = 0x02;
		for (int count1 = 0; count1 < 17; count1++) {
		  if (count1 < 3)
		    other[count1] = other_max[count1];
		  if (count1 < 8) {
		    voltage[count1] = voltage_max[count1];
		    current[count1] = current_max[count1];
		  }
		  if (sensor_max[count1] != -1000.0) // make sure values are valid
		    sensor[count1] = sensor_max[count1];
		}
	      }
	  }
	  else
	  	frm_type = 0x02;  // BPSK always send MAX MIN frame
    } 	  
    sensor_payload[0] = 0;  // clear for next payload
	  
//   if (mode == FSK) {	// remove this 
//   }
    memset(rs_frame, 0, sizeof(rs_frame));
    memset(parities, 0, sizeof(parities));

    h[0] = (short int) ((h[0] & 0xf8) | (id & 0x07)); // 3 bits
     if (uptime != 0)	  // if uptime is 0, leave reset count at 0
    {
      h[0] = (short int) ((h[0] & 0x07) | ((reset_count & 0x1f) << 3));
      h[1] = (short int) ((reset_count >> 5) & 0xff);
      h[2] = (short int) ((h[2] & 0xf8) | ((reset_count >> 13) & 0x07));
    }
    h[2] = (short int) ((h[2] & 0x0e) | ((uptime & 0x1f) << 3));
    h[3] = (short int) ((uptime >> 5) & 0xff);
    h[4] = (short int) ((uptime >> 13) & 0xff);
    h[5] = (short int) ((h[5] & 0xf0) | ((uptime >> 21) & 0x0f));
    h[5] = (short int) ((h[5] & 0x0f) | (frm_type << 4));

    if (mode == BPSK)
      h[6] = 99;

    posXi = (int)(current[map[PLUS_X]] + 0.5) + 2048;
    posYi = (int)(current[map[PLUS_Y]] + 0.5) + 2048;
    posZi = (int)(current[map[PLUS_Z]] + 0.5) + 2048;
    negXi = (int)(current[map[MINUS_X]] + 0.5) + 2048;
    negYi = (int)(current[map[MINUS_Y]] + 0.5) + 2048;
    negZi = (int)(current[map[MINUS_Z]] + 0.5) + 2048;

    posXv = (int)(voltage[map[PLUS_X]] * 100);
    posYv = (int)(voltage[map[PLUS_Y]] * 100);
    posZv = (int)(voltage[map[PLUS_Z]] * 100);
    negXv = (int)(voltage[map[MINUS_X]] * 100);
    negYv = (int)(voltage[map[MINUS_Y]] * 100);
    negZv = (int)(voltage[map[MINUS_Z]] * 100);

    batt_c_v = (int)(voltage[map[BAT]] * 100);
    battCurr = (int)(current[map[BAT]] + 0.5) + 2048;
    PSUVoltage = (int)(voltage[map[BUS]] * 100);
    PSUCurrent = (int)(current[map[BUS]] + 0.5) + 2048;

    if (payload == ON)
      STEMBoardFailure = 0;

    // read payload sensor if available

    encodeA(b, 0 + head_offset, batt_a_v);
    encodeB(b, 1 + head_offset, batt_b_v);
    encodeA(b, 3 + head_offset, batt_c_v);

    encodeB(b, 4 + head_offset, (int)(sensor[ACCEL_X] * 100 + 0.5) + 2048); // Xaccel
    encodeA(b, 6 + head_offset, (int)(sensor[ACCEL_Y] * 100 + 0.5) + 2048); // Yaccel
    encodeB(b, 7 + head_offset, (int)(sensor[ACCEL_Z] * 100 + 0.5) + 2048); // Zaccel

    encodeA(b, 9 + head_offset, battCurr);

    encodeB(b, 10 + head_offset, (int)(sensor[TEMP] * 10 + 0.5)); // Temp	  

    if (mode == FSK) {
      encodeA(b, 12 + head_offset, posXv);
      encodeB(b, 13 + head_offset, negXv);
      encodeA(b, 15 + head_offset, posYv);
      encodeB(b, 16 + head_offset, negYv);
      encodeA(b, 18 + head_offset, posZv);
      encodeB(b, 19 + head_offset, negZv);

      encodeA(b, 21 + head_offset, posXi);
      encodeB(b, 22 + head_offset, negXi);
      encodeA(b, 24 + head_offset, posYi);
      encodeB(b, 25 + head_offset, negYi);
      encodeA(b, 27 + head_offset, posZi);
      encodeB(b, 28 + head_offset, negZi);
    } else // BPSK
    {
      encodeA(b, 12 + head_offset, posXv);
      encodeB(b, 13 + head_offset, posYv);
      encodeA(b, 15 + head_offset, posZv);
      encodeB(b, 16 + head_offset, negXv);
      encodeA(b, 18 + head_offset, negYv);
      encodeB(b, 19 + head_offset, negZv);

      encodeA(b, 21 + head_offset, posXi);
      encodeB(b, 22 + head_offset, posYi);
      encodeA(b, 24 + head_offset, posZi);
      encodeB(b, 25 + head_offset, negXi);
      encodeA(b, 27 + head_offset, negYi);
      encodeB(b, 28 + head_offset, negZi);
	    
      encodeA(b_max, 12 + head_offset, (int)(voltage_max[map[PLUS_X]] * 100));
      encodeB(b_max, 13 + head_offset, (int)(voltage_max[map[PLUS_Y]] * 100));
      encodeA(b_max, 15 + head_offset, (int)(voltage_max[map[PLUS_Z]] * 100));
      encodeB(b_max, 16 + head_offset, (int)(voltage_max[map[MINUS_X]] * 100));
      encodeA(b_max, 18 + head_offset, (int)(voltage_max[map[MINUS_Y]] * 100));
      encodeB(b_max, 19 + head_offset, (int)(voltage_max[map[MINUS_Z]] * 100));

      encodeA(b_max, 21 + head_offset, (int)(current_max[map[PLUS_X]] + 0.5) + 2048);
      encodeB(b_max, 22 + head_offset, (int)(current_max[map[PLUS_Y]] + 0.5) + 2048);
      encodeA(b_max, 24 + head_offset, (int)(current_max[map[PLUS_Z]] + 0.5) + 2048);
      encodeB(b_max, 25 + head_offset, (int)(current_max[map[MINUS_X]] + 0.5) + 2048);
      encodeA(b_max, 27 + head_offset, (int)(current_max[map[MINUS_Y]] + 0.5) + 2048);
      encodeB(b_max, 28 + head_offset, (int)(current_max[map[MINUS_Z]] + 0.5) + 2048);	    

      encodeA(b_max, 9 + head_offset, (int)(current_max[map[BAT]] + 0.5) + 2048);
      encodeA(b_max, 3 + head_offset, (int)(voltage_max[map[BAT]] * 100));
      encodeA(b_max, 30 + head_offset, (int)(voltage_max[map[BUS]] * 100));
      encodeB(b_max, 46 + head_offset, (int)(current_max[map[BUS]] + 0.5) + 2048);
	    
      encodeB(b_max, 37 + head_offset, (int)(other_max[RSSI] + 0.5) + 2048);	    
      encodeA(b_max, 39 + head_offset, (int)(other_max[IHU_TEMP] * 10 + 0.5));
      encodeB(b_max, 31 + head_offset, ((int)(other_max[SPIN] * 10)) + 2048);
	    
      if (sensor_min[0] != 1000.0) // make sure values are valid
      {	        	    
	      encodeB(b_max, 4 + head_offset, (int)(sensor_max[ACCEL_X] * 100 + 0.5) + 2048); // Xaccel
	      encodeA(b_max, 6 + head_offset, (int)(sensor_max[ACCEL_Y] * 100 + 0.5) + 2048); // Yaccel
	      encodeB(b_max, 7 + head_offset, (int)(sensor_max[ACCEL_Z] * 100 + 0.5) + 2048); // Zaccel	    

	      encodeA(b_max, 33 + head_offset, (int)(sensor_max[PRES] + 0.5)); // Pressure
	      encodeB(b_max, 34 + head_offset, (int)(sensor_max[ALT] * 10.0 + 0.5)); // Altitude
	      encodeB(b_max, 40 + head_offset, (int)(sensor_max[GYRO_X] + 0.5) + 2048);
	      encodeA(b_max, 42 + head_offset, (int)(sensor_max[GYRO_Y] + 0.5) + 2048);
	      encodeB(b_max, 43 + head_offset, (int)(sensor_max[GYRO_Z] + 0.5) + 2048);

	      encodeA(b_max, 48 + head_offset, (int)(sensor_max[XS1] * 10 + 0.5) + 2048);
	      encodeB(b_max, 49 + head_offset, (int)(sensor_max[XS2] * 10 + 0.5) + 2048);
	      encodeB(b_max, 10 + head_offset, (int)(sensor_max[TEMP] * 10 + 0.5)); 	
	      encodeA(b_max, 45 + head_offset, (int)(sensor_max[HUMI] * 10 + 0.5));
      }	  
      else
      {	        	    
	      encodeB(b_max, 4 + head_offset, 2048); // 0
	      encodeA(b_max, 6 + head_offset, 2048); // 0
	      encodeB(b_max, 7 + head_offset, 2048); // 0	    

	      encodeB(b_max, 40 + head_offset, 2048);
	      encodeA(b_max, 42 + head_offset, 2048);
	      encodeB(b_max, 43 + head_offset, 2048);

	      encodeA(b_max, 48 + head_offset, 2048);
	      encodeB(b_max, 49 + head_offset, 2048);
      }	  	      
      encodeA(b_min, 12 + head_offset, (int)(voltage_min[map[PLUS_X]] * 100));
      encodeB(b_min, 13 + head_offset, (int)(voltage_min[map[PLUS_Y]] * 100));
      encodeA(b_min, 15 + head_offset, (int)(voltage_min[map[PLUS_Z]] * 100));
      encodeB(b_min, 16 + head_offset, (int)(voltage_min[map[MINUS_X]] * 100));
      encodeA(b_min, 18 + head_offset, (int)(voltage_min[map[MINUS_Y]] * 100));
      encodeB(b_min, 19 + head_offset, (int)(voltage_min[map[MINUS_Z]] * 100));

      encodeA(b_min, 21 + head_offset, (int)(current_min[map[PLUS_X]] + 0.5) + 2048);
      encodeB(b_min, 22 + head_offset, (int)(current_min[map[PLUS_Y]] + 0.5) + 2048);
      encodeA(b_min, 24 + head_offset, (int)(current_min[map[PLUS_Z]] + 0.5) + 2048);
      encodeB(b_min, 25 + head_offset, (int)(current_min[map[MINUS_X]] + 0.5) + 2048);
      encodeA(b_min, 27 + head_offset, (int)(current_min[map[MINUS_Y]] + 0.5) + 2048);
      encodeB(b_min, 28 + head_offset, (int)(current_min[map[MINUS_Z]] + 0.5) + 2048);	
	    
      encodeA(b_min, 9 + head_offset, (int)(current_min[map[BAT]] + 0.5) + 2048);
      encodeA(b_min, 3 + head_offset, (int)(voltage_min[map[BAT]] * 100));
      encodeA(b_min, 30 + head_offset, (int)(voltage_min[map[BUS]] * 100));
      encodeB(b_min, 46 + head_offset, (int)(current_min[map[BUS]] + 0.5) + 2048);
	    
      encodeB(b_min, 31 + head_offset, ((int)(other_min[SPIN] * 10)) + 2048);
      encodeB(b_min, 37 + head_offset, (int)(other_min[RSSI] + 0.5) + 2048);	    
      encodeA(b_min, 39 + head_offset, (int)(other_min[IHU_TEMP] * 10 + 0.5));
	    
      if (sensor_min[0] != 1000.0) // make sure values are valid
      {	        
	      encodeB(b_min, 4 + head_offset, (int)(sensor_min[ACCEL_X] * 100 + 0.5) + 2048); // Xaccel
	      encodeA(b_min, 6 + head_offset, (int)(sensor_min[ACCEL_Y] * 100 + 0.5) + 2048); // Yaccel
	      encodeB(b_min, 7 + head_offset, (int)(sensor_min[ACCEL_Z] * 100 + 0.5) + 2048); // Zaccel	

	      encodeA(b_min, 33 + head_offset, (int)(sensor_min[PRES] + 0.5)); // Pressure
	      encodeB(b_min, 34 + head_offset, (int)(sensor_min[ALT] * 10.0 + 0.5)); // Altitude
	      encodeB(b_min, 40 + head_offset, (int)(sensor_min[GYRO_X] + 0.5) + 2048);
	      encodeA(b_min, 42 + head_offset, (int)(sensor_min[GYRO_Y] + 0.5) + 2048);
	      encodeB(b_min, 43 + head_offset, (int)(sensor_min[GYRO_Z] + 0.5) + 2048);

	      encodeA(b_min, 48 + head_offset, (int)(sensor_min[XS1] * 10 + 0.5) + 2048);
	      encodeB(b_min, 49 + head_offset, (int)(sensor_min[XS2] * 10 + 0.5) + 2048);
	      encodeB(b_min, 10 + head_offset, (int)(sensor_min[TEMP] * 10 + 0.5)); 	    
	      encodeA(b_min, 45 + head_offset, (int)(sensor_min[HUMI] * 10 + 0.5));
    }      
      else
      {	        	    
	      encodeB(b_min, 4 + head_offset, 2048); // 0
	      encodeA(b_min, 6 + head_offset, 2048); // 0
	      encodeB(b_min, 7 + head_offset, 2048); // 0	    

	      encodeB(b_min, 40 + head_offset, 2048);
	      encodeA(b_min, 42 + head_offset, 2048);
	      encodeB(b_min, 43 + head_offset, 2048);

	      encodeA(b_min, 48 + head_offset, 2048);
	      encodeB(b_min, 49 + head_offset, 2048);
      }	 
    }    
    encodeA(b, 30 + head_offset, PSUVoltage);

    encodeB(b, 31 + head_offset, ((int)(other[SPIN] * 10)) + 2048);

    encodeA(b, 33 + head_offset, (int)(sensor[PRES] + 0.5)); // Pressure
    encodeB(b, 34 + head_offset, (int)(sensor[ALT] * 10.0 + 0.5)); // Altitude

    encodeA(b, 36 + head_offset, Resets);
    encodeB(b, 37 + head_offset, (int)(other[RSSI] + 0.5) + 2048);

    encodeA(b, 39 + head_offset, (int)(other[IHU_TEMP] * 10 + 0.5));

    encodeB(b, 40 + head_offset, (int)(sensor[GYRO_X] + 0.5) + 2048);
    encodeA(b, 42 + head_offset, (int)(sensor[GYRO_Y] + 0.5) + 2048);
    encodeB(b, 43 + head_offset, (int)(sensor[GYRO_Z] + 0.5) + 2048);

    encodeA(b, 45 + head_offset, (int)(sensor[HUMI] * 10 + 0.5)); // in place of sensor1

    encodeB(b, 46 + head_offset, PSUCurrent);
    encodeA(b, 48 + head_offset, (int)(sensor[XS1] * 10 + 0.5) + 2048);
    encodeB(b, 49 + head_offset, (int)(sensor[XS2] * 10 + 0.5) + 2048);

    FILE * command_count_file = fopen("/home/pi/CubeSatSim/command_count.txt", "r");
    if (command_count_file != NULL) {	
      char count_string[10];	
      if ( (fgets(count_string, 10, command_count_file)) != NULL)
	   groundCommandCount = atoi(count_string); 
      fclose(command_count_file);	    
    } else
	    printf("Error opening command_count.txt!\n");
    
    printf("Command count: %d\n", groundCommandCount);	  
    
    int status = STEMBoardFailure + SafeMode * 2 + sim_mode * 4 + PayloadFailure1 * 8 +
      (i2c_bus0 == OFF) * 16 + (i2c_bus1 == OFF) * 32 + (i2c_bus3 == OFF) * 64 + (camera == OFF) * 128 + groundCommandCount * 256;

    encodeA(b, 51 + head_offset, status);
    encodeB(b, 52 + head_offset, rxAntennaDeployed + txAntennaDeployed * 2);

    if (txAntennaDeployed == 0) {
      txAntennaDeployed = 1;
      printf("TX Antenna Deployed!\n");
    }
    if (rxAntennaDeployed == 0) {
      rxAntennaDeployed = 1;
      printf("RX Antenna Deployed!\n");
    }	  
    
    if (mode == BPSK) {  // wod field experiments
      unsigned long val = 0xffff;
      encodeA(b, 64 + head_offset, 0xff & val); 
      encodeA(b, 65 + head_offset, val >> 8); 	    
      encodeA(b, 63 + head_offset, 0x00); 
      encodeA(b, 62 + head_offset, 0x01);
      encodeB(b, 74 + head_offset, 0xfff); 
    }	  
    short int data10[headerLen + rsFrames * (rsFrameLen + parityLen)];
    short int data8[headerLen + rsFrames * (rsFrameLen + parityLen)];

    int ctr1 = 0;
    int ctr3 = 0;
    for (i = 0; i < rsFrameLen; i++) {
      for (int j = 0; j < rsFrames; j++) {
        if (!((i == (rsFrameLen - 1)) && (j == 2))) // skip last one for BPSK
        {
          if (ctr1 < headerLen) {
            rs_frame[j][i] = h[ctr1];
            update_rs(parities[j], h[ctr1]);
            //      				printf("header %d rs_frame[%d][%d] = %x \n", ctr1, j, i, h[ctr1]);
            data8[ctr1++] = rs_frame[j][i];
            //				printf ("data8[%d] = %x \n", ctr1 - 1, rs_frame[j][i]);
          } else {
	     if (mode == FSK)
	     {
            	rs_frame[j][i] = b[ctr3 % dataLen];
            	update_rs(parities[j], b[ctr3 % dataLen]);
	     }  else // BPSK
		if ((int)(ctr3/dataLen) == 3)  
		{
            		rs_frame[j][i] = b_max[ctr3 % dataLen];
            		update_rs(parities[j], b_max[ctr3 % dataLen]);
		}
		else if ((int)(ctr3/dataLen) == 4)  
		{
            		rs_frame[j][i] = b_min[ctr3 % dataLen];
            		update_rs(parities[j], b_min[ctr3 % dataLen]);
		}		
		else
		{
            		rs_frame[j][i] = b[ctr3 % dataLen];
            		update_rs(parities[j], b[ctr3 % dataLen]);
		}
	     {
	    }
		  
            //  				printf("%d rs_frame[%d][%d] = %x %d \n", 
            //  					ctr1, j, i, b[ctr3 % DATA_LEN], ctr3 % DATA_LEN);
            data8[ctr1++] = rs_frame[j][i];
            //			printf ("data8[%d] = %x \n", ctr1 - 1, rs_frame[j][i]);
            ctr3++;
          }
        }
      }
    }

    #ifdef DEBUG_LOGGING
    //	printf("\nAt end of data8 write, %d ctr1 values written\n\n", ctr1);
    /*
    	  printf("Parities ");
    		for (int m = 0; m < parityLen; m++) {
    		 	printf("%d ", parities[0][m]);
    		}
    		printf("\n");
    */
    #endif
	   
    int ctr2 = 0;
    memset(data10, 0, sizeof(data10));

    for (i = 0; i < dataLen * payloads + headerLen; i++) // 476 for BPSK
    {
      data10[ctr2] = (Encode_8b10b[rd][((int) data8[ctr2])] & 0x3ff);
      nrd = (Encode_8b10b[rd][((int) data8[ctr2])] >> 10) & 1;
      //		printf ("data10[%d] = encoded data8[%d] = %x \n",
      //		 	ctr2, ctr2, data10[ctr2]); 

      rd = nrd; // ^ nrd;
      ctr2++;
    }
//    {
      for (i = 0; i < parityLen; i++) {
        for (int j = 0; j < rsFrames; j++) {
          if ((uptime != 0) || (i != 0))	// don't correctly update parties if uptime is 0 so the frame will fail the FEC check and be discarded  
            data10[ctr2++] = (Encode_8b10b[rd][((int) parities[j][i])] & 0x3ff);
	  nrd = (Encode_8b10b[rd][((int) parities[j][i])] >> 10) & 1;
        //	printf ("data10[%d] = encoded parities[%d][%d] = %x \n",
        //		 ctr2 - 1, j, i, data10[ctr2 - 1]); 

          rd = nrd;
        }
      }
 //   }
    #ifdef DEBUG_LOGGING
    // 	printf("\nAt end of data10 write, %d ctr2 values written\n\n", ctr2);
    #endif

    int data;
    int val;
    //int offset = 0;

    #ifdef DEBUG_LOGGING
    //	printf("\nAt start of buffer loop, syncBits %d samples %d ctr %d\n", syncBits, samples, ctr);
    #endif

    for (i = 1; i <= syncBits * samples; i++) {
      write_wave(ctr, buffer);
      //		printf("%d ",ctr);
      if ((i % samples) == 0) {
        int bit = syncBits - i / samples + 1;
        val = sync;
        data = val & 1 << (bit - 1);
        //   	printf ("%d i: %d new frame %d sync bit %d = %d \n",
        //  		 ctr/SAMPLES, i, frames, bit, (data > 0) );
        if (mode == FSK) {
          phase = ((data != 0) * 2) - 1;
          //		printf("Sending a %d\n", phase);
        } else {
          if (data == 0) {
            phase *= -1;
            if ((ctr - smaller) > 0) {
              for (int j = 1; j <= smaller; j++)
                buffer[ctr - j] = buffer[ctr - j] * 0.4;
            }
            flip_ctr = ctr;
          }
        }
      }
    }
    #ifdef DEBUG_LOGGING
    //	printf("\n\nValue of ctr after header: %d Buffer Len: %d\n\n", ctr, buffSize);
    #endif
    for (i = 1; i <= (10 * (headerLen + dataLen * payloads + rsFrames * parityLen) * samples); i++) // 572   
    {
      write_wave(ctr, buffer);
      if ((i % samples) == 0) {
        int symbol = (int)((i - 1) / (samples * 10));
        int bit = 10 - (i - symbol * samples * 10) / samples + 1;
        val = data10[symbol];
        data = val & 1 << (bit - 1);
        //		printf ("%d i: %d new frame %d data10[%d] = %x bit %d = %d \n",
        //	    		 ctr/SAMPLES, i, frames, symbol, val, bit, (data > 0) );
        if (mode == FSK) {
          phase = ((data != 0) * 2) - 1;
          //			printf("Sending a %d\n", phase);
        } else {
          if (data == 0) {
            phase *= -1;
            if ((ctr - smaller) > 0) {
              for (int j = 1; j <= smaller; j++)
                buffer[ctr - j] = buffer[ctr - j] * 0.4;
            }
            flip_ctr = ctr;
          }
        }
      }
    }
  }
  #ifdef DEBUG_LOGGING
  //	printf("\nValue of ctr after looping: %d Buffer Len: %d\n", ctr, buffSize);
  //	printf("\ctr/samples = %d ctr/(samples*10) = %d\n\n", ctr/samples, ctr/(samples*10));
  #endif

  int error = 0;
  // int count;
  //  for (count = 0; count < dataLen; count++) {
  //      printf("%02X", b[count]);
  //  }
  //  printf("\n");

  // socket write

  if (!socket_open && transmit) {
    printf("Opening socket!\n");
 //   struct sockaddr_in address;
 //   int valread;
    struct sockaddr_in serv_addr;
    //    char *hello = "Hello from client"; 
    //    char buffer[1024] = {0}; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("\n Socket creation error \n");
      error = 1;
    }

    memset( & serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if (inet_pton(AF_INET, "127.0.0.1", & serv_addr.sin_addr) <= 0) {
      printf("\nInvalid address/ Address not supported \n");
      error = 1;
    }

    if (connect(sock, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0) {
      printf("\nConnection Failed \n");
      printf("Error: %s \n", strerror(errno));
      error = 1;
      sleep(2.0);  // sleep if socket connection refused

    // try again
      error = 0;
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        error = 1;
      }

      memset( & serv_addr, '0', sizeof(serv_addr));

      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(PORT);

      // Convert IPv4 and IPv6 addresses from text to binary form 
      if (inet_pton(AF_INET, "127.0.0.1", & serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        error = 1;
      }

      if (connect(sock, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        printf("Error: %s \n", strerror(errno));
        error = 1;
 //       sleep(1.0);  // sleep if socket connection refused
      }	    
    }
    if (error == 1)
    ; //rpitxStatus = -1;
    else
      socket_open = 1;
  }

  if (!error && transmit) {
    //	digitalWrite (0, LOW);
 //   printf("Sending %d buffer bytes over socket after %d ms!\n", ctr, (long unsigned int)millis() - start);
    start = millis();
    int sock_ret = send(sock, buffer, (unsigned int)(ctr * 2 + 2), 0);
    printf("socket send 1 %d ms bytes: %d \n\n", (unsigned int)millis() - start, sock_ret);
    fflush(stdout);	  
    
    if (sock_ret < (ctr * 2 + 2)) {
  //    printf("Not resending\n");
      sleep(0.5);
      sock_ret = send(sock, &buffer[sock_ret], (unsigned int)(ctr * 2 + 2 - sock_ret), 0);
      printf("socket send 2 %d ms bytes: %d \n\n", millis() - start, sock_ret);
    }
	  
    loop_count++;	  
    if ((firstTime == 1) || (((loop_count % 180) == 0) && (mode == FSK)) || (((loop_count % 80) == 0) && (mode == BPSK))) // do first time and was every 180 samples
    {
      int max;
      if (mode == FSK)
	      if (sim_mode)
 		max = 6;
              else if (firstTime == 1)
	      	max = 4;  // 5; // was 6
              else
		max = 3;
      else  
 	      if (firstTime == 1)
	      	max = 5;  // 5; // was 6
              else
		max = 4;  
	    
      for (int times = 0; times < max; times++) 	    
      {
	      start = millis();  // send frame until buffer fills
	      sock_ret = send(sock, buffer, (unsigned int)(ctr * 2 + 2), 0);
	      printf("socket send %d in %d ms bytes: %d \n\n",times + 2, (unsigned int)millis() - start, sock_ret);
	      
	      if ((millis() - start) > 500) {
		      printf("Buffer over filled!\n");
		      break;
	      }

	      if (sock_ret < (ctr * 2 + 2)) {
	  //    printf("Not resending\n");
		sleep(0.5);
		sock_ret = send(sock, &buffer[sock_ret], (unsigned int)(ctr * 2 + 2 - sock_ret), 0);
		printf("socket resend %d in %d ms bytes: %d \n\n",times, millis() - start, sock_ret);
	      }
      }
      sampleTime = (unsigned int) millis(); // resetting time for sleeping
      fflush(stdout);
//      if (firstTime == 1)
//	      max -= 1;
    }

    if (sock_ret == -1) {
      printf("Error: %s \n", strerror(errno));
      socket_open = 0;
      //rpitxStatus = -1;
    }
  }
  if (!transmit) {
    fprintf(stderr, "\nNo CubeSatSim Band Pass Filter detected.  No transmissions after the CW ID.\n");
    fprintf(stderr, " See http://cubesatsim.org/wiki for info about building a CubeSatSim\n\n");
  }

  if (socket_open == 1)	
    firstTime = 0;
//  else if (frames_sent > 0) //5)
//    firstTime = 0;

  return;
}

// ============================================================================================================================================================================================================================================= 


// code by https://stackoverflow.com/questions/25161377/open-a-cmd-program-with-full-functionality-i-o/25177958#25177958

FILE *sopen(const char *program)
    {
        int fds[2];
        pid_t pid;

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) // UNIX comunicacion bidireccional
            return NULL;

        switch(pid=vfork()) { // Vfork crea un hijo
        case -1:    /* Error */
            close(fds[0]);
            close(fds[1]);
            return NULL;
        case 0:     /* child */
            close(fds[0]);
            dup2(fds[1], 0);
            dup2(fds[1], 1);
            close(fds[1]);
            execl("/bin/sh", "sh", "-c", program, NULL);
            _exit(127);
        }
        /* parent */
        close(fds[1]);
        return fdopen(fds[0], "r+");
    }

 /*
 * TelemEncoding.h
 *
 *  Created on: Feb 3, 2014
 *      Author: fox
 */

/* 
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define false 0
#define true 1
*/

// ============================================================================================================================================================================================================================================= 
// - CREACIÓN ONDAS SINUSOIDALES CON MODULACIONES SEGÚN CONDICIONES DE MODO Y RANGO
// ============================================================================================================================================================================================================================================= 

void write_wave(int i, short int *buffer)
{
		if (mode == FSK)
		{
			if ((ctr - flip_ctr) < smaller)
				buffer[ctr++] = (short int)(0.1 * phase * (ctr - flip_ctr) / smaller);
			else
				buffer[ctr++] = (short int)(0.25 * amplitude * phase);
		}
		else
		{
			if ((ctr - flip_ctr) < smaller)
//  		 		buffer[ctr++] = (short int)(amplitude * 0.4 * phase * sin((float)(2*M_PI*i*freq_Hz/S_RATE)));	 	  		 	buffer[ctr++] = (short int)(amplitude * 0.4 * phase * sin((float)(2*M_PI*i*freq_Hz/S_RATE)));	
  		 		buffer[ctr++] = (short int)(phase * sin_map[ctr % sin_samples] / 2);
		else
//  		 		buffer[ctr++] = (short int)(amplitude * 0.4 * phase * sin((float)(2*M_PI*i*freq_Hz/S_RATE)));	 	 		 	buffer[ctr++] = (short int)(amplitude * phase * sin((float)(2*M_PI*i*freq_Hz/S_RATE)));	
  		 		buffer[ctr++] = (short int)(phase * sin_map[ctr % sin_samples]); 		 } 			
//		printf("%d %d \n", i, buffer[ctr - 1]);

}
// ============================================================================================================================================================================================================================================= 


// ============================================================================================================================================================================================================================================= 
// - CODIFICADOR 
// ============================================================================================================================================================================================================================================= 

int encodeA(short int  *b, int index, int val) { // Codifica un valor de 16 bits (val) en 12 bits
//    printf("Encoding A\n");
    b[index] = val & 0xff;
    b[index + 1] = (short int) ((b[index + 1] & 0xf0) | ((val >> 8) & 0x0f));
    return 0;	
}

int encodeB(short int  *b, int index, int val) { // Sobrescribe mas ampliamente los datos en b
//    printf("Encoding B\n");
    b[index] =  (short int) ((b[index] & 0x0f)  |  ((val << 4) & 0xf0));
    b[index + 1] = (val >> 4 ) & 0xff;
    return 0;	
}

// ============================================================================================================================================================================================================================================= 


int twosToInt(int val,int len) {   // Convert twos compliment to integer
// from https://www.raspberrypi.org/forums/viewtopic.php?t=55815
	
      if(val & (1 << (len - 1)))
         val = val - (1 << len);

      return(val);
}

float rnd_float(double min,double max) {   // returns 2 decimal point random number
	
	int val = (rand() % ((int)(max*100) - (int)(min*100) + 1)) + (int)(min*100);
	float ret = ((float)(val)/100);
	
      return(ret);
}

// ============================================================================================================================================================================================================================================= 
// -VERIFICA FUNCIONALIDAD DE UN BUS
// ============================================================================================================================================================================================================================================= 

int test_i2c_bus(int bus)
{
	int output = bus; // return bus number if OK, otherwise return -1
	char busDev[20] = "/dev/i2c-";
	char busS[5];
	snprintf(busS, 5, "%d", bus);  
	strcat (busDev, busS);	
	printf("I2C Bus Tested: %s \n", busDev);
	
	if (access(busDev, W_OK | R_OK) >= 0)  {   // Test if I2C Bus is present
//	  	printf("bus is present\n\n");	    
    	  	char result[128];		
    	  	const char command_start[] = "timeout 2 i2cdetect -y ";  // was  5 10
		char command[50];
		strcpy (command, command_start);
    	 	strcat (command, busS);
//     	 	printf("Command: %s \n", command);
    	 	FILE *i2cdetect = popen(command, "r");
	
    	 	while (fgets(result, 128, i2cdetect) != NULL) {
    	 		;
//       	 	printf("result: %s", result);
    	 	}	
    	 	int error = pclose(i2cdetect)/256;
//      	 	printf("%s error: %d \n", &command, error);
    	 	if (error != 0) 
    	 	{	
    	 		printf("ERROR: %s bus has a problem \n  Check I2C wiring and pullup resistors \n", busDev);
			if (bus == 3)
				printf("-> If this is a CubeSatSim Lite, then this error is normal!\n");
			output = -1;
    		}								
	} else
	{
    	 	printf("ERROR: %s bus has a problem \n  Check software to see if I2C enabled \n", busDev);
		output = -1; 
	}
	return(output);	// return bus number or -1 if there is a problem with the bus
}
// ============================================================================================================================================================================================================================================= 

float toAprsFormat(float input) {
// converts decimal coordinate (latitude or longitude) to APRS DDMM.MM format	(GRADOS (DD)| MINUTOS DECIMALES(MM.MM))
    int dd = (int) input;
    int mm1 = (int)((input - dd) * 60.0);
    int mm2 = (int)((input - dd - (float)mm1/60.0) * 60.0 * 60.0);
    float output = dd * 100 + mm1 + (float)mm2 * 0.01;
    return(output);	
}


//#define GET_IMAGE_DEBUG

//#define DEBUG

//#define PICOW true
//int led_pin = LED_BUILTIN;

/*
void loop() {

  char input_file[] = "/cam.jpg"; 
  char output_file[] = "/cam.bin"; 
  
  get_image_file();

  Serial.println("Got image from ESP-32-CAM!");

  delay(1000);

}
*/

// ============================================================================================================================================================================================================================================= 
// - DETECCION Y PROCESADO DE IMAGENES (YA TOMADAS)
// ============================================================================================================================================================================================================================================= 

int get_payload_serial(int debug_camera)  {
 
  index1 = 0;
  flag_count = 0;
  start_flag_detected = FALSE;
  start_flag_complete = FALSE;
  end_flag_detected = FALSE;
  jpeg_start = 0;
 
// #ifdef GET_IMAGE_DEBUG
 if (debug_camera)
  printf("Received from Payload:\n");
 // #endif
  finished = FALSE;

  unsigned long time_start = millis();	    
  while ((!finished) && ((millis() - time_start) < CAMERA_TIMEOUT)) {
	  
    if (serialDataAvail(uart_fd)) {
	      char octet = (char) serialGetchar(uart_fd);
              printf("%c", octet);
              fflush(stdout);	  

//   if (Serial2.available()) {      // If anything comes in Serial2
//      byte octet = Serial2.read();
///      if ((!start_flag_detected) && (debug_camera))
///        Serial.write(octet);

     if (start_flag_complete) {
//       printf("Start flag complete detected\n");
       buffer2[index1++] = octet;
       if (octet == end_flag[flag_count]) {  // looking for end flag
//         if (end_flag_detected) {
            flag_count++;
#ifdef GET_IMAGE_DEBUG  
//       if (debug_camera)  
            printf("Found part of end flag!\n");
#endif
            if (flag_count >= strlen(end_flag)) {  // complete image           
///              buffer2[index1++] = octet;
//              Serial.println("\nFound end flag");
//              Serial.println(octet, HEX);
///              while(!Serial2.available()) { }     // Wait for another byte
//              octet = Serial2.read(); 
//              buffer2[index1++] = octet;
//              Serial.println(octet, HEX);
//              while(!Serial2.available()) { }     // Wait for another byte
///              int received_crc = Serial2.read(); 
//              buffer2[index1++] = octet;
/*                            
              Serial.print("\nFile length: ");
              Serial.println(index1 - (int)strlen(end_flag));
//              index1 -= 1; // 40;
//              Serial.println(buffer2[index1 - 1], HEX); 
//              int received_crc = buffer2[index1];
//              index1 -= 1;

              uint8_t * data = (uint8_t *) &buffer2[0];
#ifdef GET_IMAGE_DEBUG
       Serial.println(buffer2[0], HEX);
      Serial.println(buffer2[index1 - 1], HEX);
      Serial.println(index1);            
 #endif  
     if (debug_camera) {                
      Serial.print("\nCRC received:");
      Serial.println(received_crc, HEX);   
    }
           
              int calculated_crc = CRC8.smbus(data, index1);
 //             Serial.println(calculated_crc, HEX);
              if (received_crc == calculated_crc)
                Serial.println("CRC check succeeded!");
              else  
               Serial.println("CRC check failed!"); 

*/		    
//              index1 -= 40;                         
              index1 -= strlen(end_flag);
	      buffer2[index1++] = 0;
	      printf(" Payload length: %d \n",index1); 	    

//              write_jpg();
	      finished = TRUE;	    
              index1 = 0;           
              start_flag_complete = FALSE;
              start_flag_detected = FALSE; // get ready for next image 
              end_flag_detected = FALSE;
              flag_count = 0; 
//              delay(6000);
            }
         } else {
	   if (flag_count > 1)    
             printf("Resetting. Not end flag.\n");    	       
           flag_count = 0;
	       
         }
 ///        buffer2[index1++] = octet;
           
//#ifdef GET_IMAGE_DEBUG 
    if (debug_camera) {   
           char hexValue[5];
           if (octet != 0x66) {
             sprintf(hexValue, "%02X", octet);
//             printf(hexValue);
           } else {
//             Serial.println("\n********************************************* Got a 66!");
             printf("66");
           } 
//             Serial.write(octet);
    }
//#endif             
           if (index1 > 1000) {
             index1 = 0; 
	     printf("Resetting index!\n");	   
	   }
//         }
    } else if (octet == start_flag[flag_count]) {  // looking for start flag
          start_flag_detected = TRUE;
          flag_count++;
#ifdef GET_IMAGE_DEBUG  
          printf("Found part of start flag! \n");
#endif  
          if (flag_count >= strlen(start_flag)) {
            flag_count = 0;
            start_flag_complete = TRUE;
#ifdef GET_IMAGE_DEBUG  
            printf("Found all of start flag!\n");        
#endif  
          }
      } else {  // not the flag, keep looking
          start_flag_detected = FALSE;
          flag_count = 0;
#ifdef GET_IMAGE_DEBUG  
          printf("Resetting. Not start flag.\n");        
#endif  
       } 
    }
//    Serial.println("writing to Serial2");
  }
  if (debug_camera)                 
      printf("\nComplete\n");
  fflush(stdout);	
  return(finished);
}

// ============================================================================================================================================================================================================================================= 




// ============================================================================================================================================================================================================================================= 
// - CONFIGURACION MODULO DE RADIO DE TRANSMISION Y RECEPCION
// ============================================================================================================================================================================================================================================= 

void program_radio() {
// if (sr_frs_present) {	
  printf("Programming FM module!\n");	

  pinMode(28, OUTPUT);	
  pinMode(29, OUTPUT);
  digitalWrite(29, HIGH);  // enable SR_FRS
  digitalWrite(28, HIGH);  // stop transmit	
	
if ((uart_fd = serialOpen("/dev/ttyAMA0", 9600)) >= 0) {  // was 9600
  printf("serial opened 9600\n");  
  for (int i = 0; i < 5; i++) {
     sleep(1); // delay(500);
//#ifdef APRS_VHF
//     char vhf_string[] = "AT+DMOSETGROUP=0,144.3900,144.3900,0,3,0,0\r\n";	
//     serialPrintf(uart_fd, vhf_string);	
//     mySerial.println("AT+DMOSETGROUP=0,144.3900,144.3900,0,3,0,0\r");    // can change to 144.39 for standard APRS	  
//    mySerial.println("AT+DMOSETGROUP=0,145.0000,145.0000,0,3,0,0\r");    // can change to 145 for testing ASPRS	  
//#else
     char uhf_string[] = "AT+DMOSETGROUP=0,435.0000,434.9000,0,3,0,0\r\n";
     char uhf_string1a[] = "AT+DMOSETGROUP=0,";	// changed frequency to verify
     char comma[] = ",";
     char uhf_string1b[] = ",0,";	// changed frequency to verify
     char uhf_string1[] = "AT+DMOSETGROUP=0,435.0000,434.9000,0,";	// changed frequency to verify
     char uhf_string2[] = ",0,0\r\n";	
     char sq_string[2];
     sq_string[0] = '0' + squelch;
     sq_string[1] = 0;
	  
//     serialPrintf(uart_fd, uhf_string);	
/**/	  
	serialPrintf(uart_fd, uhf_string1a);	
	serialPrintf(uart_fd, rx);	
	serialPrintf(uart_fd, comma);	
	serialPrintf(uart_fd, tx);	
	serialPrintf(uart_fd, uhf_string1b);	
	serialPrintf(uart_fd, sq_string);	
	serialPrintf(uart_fd, uhf_string2);	
/**/	  
	  
//     mySerial.println("AT+DMOSETGROUP=0,435.1000,434.9900,0,3,0,0\r");   // squelch set to 3
//#endif	  
   sleep(1);
   char mic_string[] = "AT+DMOSETMIC=8,0\r\n";  
   serialPrintf(uart_fd, mic_string);
//   mySerial.println("AT+DMOSETMIC=8,0\r");  // was 8
	
  }
 }
//#ifdef APRS_VHF	  	
// printf("Programming FM tx 144.39, rx on 144.39 MHz\n");
//#else
 printf("Programming FM tx 434.9, rx on 435.0 MHz\n");
//#endif	
//  digitalWrite(PTT_PIN, LOW);  // transmit carrier for 0.5 sec
//  sleep(0.5);
//  digitalWrite(PTT_PIN, HIGH);	
  digitalWrite(29, LOW);  // disable SR_FRS	
  pinMode(28, INPUT);
  pinMode(29, INPUT);

  serialClose(uart_fd);	
}

// ============================================================================================================================================================================================================================================= 


// ============================================================================================================================================================================================================================================= 
// - CONFIGURACION MODO AHORRO DE BATERIA
// ============================================================================================================================================================================================================================================= 

int battery_saver_check() { // Verifica si esta on o off el ahorro de bateria
	FILE *file = fopen("/home/pi/CubeSatSim/battery_saver", "r"); // se crea cuando se activa el modo ahorro
	if (file == NULL) {
		fprintf(stderr,"Battery saver mode is OFF!\n");
		return(OFF);
	} 
	fclose(file);
	fprintf(stderr,"Battery saver mode is ON!\n");
	return(ON);
}

void battery_saver(int setting) { // Activa o apaga el modo ahorro
if (setting == ON) {
	if ((mode == AFSK) || (mode == SSTV) || (mode == CW)) {
		if (battery_saver_check() == OFF) {
			FILE *command = popen("touch /home/pi/CubeSatSim/battery_saver", "r");
		  	pclose(command);
			fprintf(stderr,"Turning Battery saver mode ON\n");  
//			command = popen("if ! grep -q force_turbo=1 /boot/config.txt ; then sudo sh -c 'echo force_turbo=1 >> /boot/config.txt'; fi", "r");
//		  	pclose(command);
			command = popen("sudo reboot now", "r");
		  	pclose(command);
			sleep(60);
			return;  
		} else
			fprintf(stderr, "Nothing to do for battery_saver\n");
	}  
  } else if (setting == OFF) {
	if ((mode == AFSK) || (mode == SSTV) || (mode == CW)) {
		if (battery_saver_check() == ON) {
			FILE *command = popen("rm /home/pi/CubeSatSim/battery_saver", "r");
		  	pclose(command);
			fprintf(stderr,"Turning Battery saver mode OFF\n"); 
//			command = popen("sudo sed -i ':a;N;$!ba;s/\'$'\n''force_turbo=1//g' /boot/config.txt", "r");
//		  	pclose(command);
			command = popen("sudo reboot now", "r");
		  	pclose(command);
			sleep(60);
			return; 
		} else
			fprintf(stderr, "Nothing to do for battery_saver\n");
	}  
  } else {
	  fprintf(stderr,"battery_saver function error");
	  return;
  }
  return;
}
// ============================================================================================================================================================================================================================================= 




////////////////// ============================================================================================================================================================================================================================================= 

