// UsbExMulti.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include "dirent.h"

// define a macro for short-hand error processing
#define CheckHr(s) if (FAILED(hr)){printf(s);return 0;}

inline bool exists(const char *filename)
	{
		std::ifstream infile(filename);
		if (infile.good())
		{
			infile.close();
			return true;
		}
		else
			return false;
	}


// main entry point of console program
int main(int argc, char *argv[])
{
	/*
	Parameters:
		1: File name of file to load
			File either contains values of each mirror actuator or is a list of
			files that each contain actuator values
		2: Multiplier
		3: Number of files to load (if 1st parameter contains list of file names)
		4: Sleep duration
			Time to wait between applying different patterns if more than one are 
			to be loaded to mirror

	*/

	if (argc>1)
		cout << "File to load: " << argv[1] << "\n";
	else
		return 0;

	double multiplier=1;
	if (argc>2)
	{
		multiplier = strtod(argv[2], NULL);
		cout << "Multiplier: " << multiplier << "\n";
	}

	SHORT numFilesToLoad=0;
	if (argc>3)
	{
		//Number of files to load (unsigned long)
		numFilesToLoad = strtoul(argv[3], NULL, 10);
		cout << "Will load " << numFilesToLoad << " files. \n";
	}

	SHORT sleepDuration=500;
	if (argc>4)
	{
		//Sleep duration. If <0, will wait for user to hit <enter>
		sleepDuration = strtol(argv[4], NULL, 10);
		cout << "Will sleep for " << sleepDuration << "\n";
	}

	USHORT combined	[NUM_ACTUATORS] = {0x0000};

	WIN32_FILE_ATTRIBUTE_DATA    fileInfo;
	FILETIME	fTime;
	SYSTEMTIME	sTime;


	// /*
	//READ IN FLATTENING FILE
	// */
	USHORT	fromFile	[NUM_ACTUATORS]  = {0x0000};
	char fileLine[32] = {0};
	ifstream input_file;
	char *flat_file = "D:/Ryan/BMC_Mirror/hex_values.txt";
	if (exists(flat_file))
	{
		input_file.open(flat_file);
		//Here, I'm just seeing how to extract file info so that maybe I can 
		//use creation or edit date to find files and then load to mirror
		GetFileAttributesEx(flat_file, GetFileExInfoStandard, &fileInfo);
		cout << "File size of flat_file: " << fileInfo.nFileSizeLow << "\n";
		fTime = fileInfo.ftLastWriteTime;
		FileTimeToSystemTime(&fTime, &sTime);
		cout << "File time: " << sTime.wDay << " day \n";
		cout << "File time: " << sTime.wMonth << " month \n";
		cout << "File time: " << sTime.wHour << " hour \n";
		cout << "File time: " << sTime.wMinute << " minute \n";
		cout << "File time: " << sTime.wSecond << " second \n";
		//cout << "File mod date of flat_file: " << fileInfo.ftLastWriteTime << "\n";
	}
	else
	{
		cout << "Flattening file does not exist.\n";
		return 0;
	}
	for (int i=0; i<NUM_ACTUATORS; i++)
	{
		//Reading in actuator values from the 'flattening' file
		input_file >> fileLine;
		fromFile[i] = strtoul(fileLine, NULL, 16); //Reading in hex values
		//cout << "Line " << fileLine << "  " << fromFile[i] << "\n";
	}
	input_file.close(); //Close file of actuator values to flatten mirror

	//Reading in mapping file
	USHORT	mapFromFile	[NUM_ACTUATORS]  = {0x0000};
	ifstream input_file2;
	//mapFile maps the position of actuator value in the file or in the array to 
	//the actual location on the mirror
	char *mapFile = "D:/Ryan/BMC_Mirror/MultiDM-04.map";
	if (exists(mapFile))
		input_file2.open(mapFile);
	else
	{
		cout << "Mapping file does not exist.\n";
		return 0;
	}
	for (int i=0; i<NUM_ACTUATORS; i++)
	{
		input_file2 >> fileLine;
		mapFromFile[i] = strtoul(fileLine, NULL, 10);
	}
	input_file2.close(); //Close mapping file

	USHORT	sMapDataFile	[NUM_ACTUATORS]  = {0x0000};    // this is what is sent to mirror
	int actuatorVal=0;
	int	fromAddFile	[NUM_ACTUATORS]  = {0x0000};
	ifstream add_file;
	bool inputFileExists;
	if (numFilesToLoad<2) //if only using one file of actuator values
	{
		inputFileExists = exists(argv[1]);
		if (!(inputFileExists))
		{
			cout << "Input file does not exist.\n";
			return 0;
		}
		add_file.open(argv[1]);
		for (int i=0; i<NUM_ACTUATORS; i++)
		{
			add_file >> fileLine;
			//reads in data from file (as long) and applies `multiplier':
			fromAddFile[i] = strtol(fileLine, NULL, 10) * multiplier;
			actuatorVal = fromFile[i] + fromAddFile[i];
			//Actuator value must be between 0 and 65535
			if (actuatorVal<0) 
				{
					cout << "Index " << mapFromFile[i] << " or " << i << " to " << actuatorVal << "\n";
					actuatorVal=0;
				}
				if (actuatorVal>65535)
				{
					cout << "Index " << mapFromFile[i] << " or " << i << " to " << actuatorVal << "\n";
					actuatorVal=65535;
				}
			combined[i] = actuatorVal; //has data in file plus the flattening values
		}
		add_file.close();
		for ( int j=0; j<NUM_ACTUATORS; j++ )
		{
			//Data from file plus flattening values that are mapped to actuator positions
			sMapDataFile[j] = combined[mapFromFile[j]]; 
		}
	}




	


	// Initializes the COM library on the current thread and 
	// identifies the concurrency model as single-thread apartment.
	// Applications must initialize the COM library before they
	// can call COM library functions 
	CoInitialize(NULL);

	long lCurDev = -1;				// current USB device index: -1 means no devices
	long lStatus = 0;				// CIUsbLib return status
	CComPtr<IHostDrv> pIHostDrv;	// CIUsbLib COM Pointer		

	// Creates a single uninitialized object of the class associated with a specified CLSID=__uuidof(CHostDrv)
	// This object will be found on the system if CIUsbLib.dll is registered via regsvr32.exe
	HRESULT hr = CoCreateInstance(__uuidof(CHostDrv), NULL, CLSCTX_INPROC, __uuidof(IHostDrv), (LPVOID *) &pIHostDrv);
	if(hr == REGDB_E_CLASSNOTREG)
	{
		printf("The CHostDrv class is not registered.\nUse regsvr32.exe to register CIUsbLib.dll\n");
		return 0;
	}
	else if (FAILED(hr))
	{
		printf("Error (0x%08x) creating CHostDrv object (CIUsbLib.dll)\n", hr);
		return 0;
	}

	// Check for USB devices supported by the CIUsbLib
	// The array lDevices is set by CIUsb_GetAvailableDevices to indicate which devices are present in the system
	// In order to recognize MULTI DM devices, {CiGenUSB.sys, CiGenUSB.inf} need to be installed properly
	long lDevices[MAX_USB_DEVICES] = {-1};
	hr = pIHostDrv->CIUsb_GetAvailableDevices(lDevices, sizeof(lDevices)/sizeof(long), &lStatus);
	CheckHr("Failure to get available USB devices.\n");

	

	// loop through devices found
	for (int i=0; i<MAX_USB_DEVICES; i++)
	{
		// check for device indices not equal to -1
		if (lDevices[i] != -1)
		{
			// if we have any present, check specifically for MULTI via CIUsb_STATUS_DEVICENAME
			char cDevName[4096] = {0};
			hr = pIHostDrv->CIUsb_GetStatus(i, CIUsb_STATUS_DEVICENAME, (long *) cDevName);
			CheckHr("Failure to get available USB device name.\n");

			// check the device for MULTI signature
			bool fFoundMulti = (strstr(cDevName, USB_DEVNAME)!=NULL);
			if (fFoundMulti)
			{
				// record device index
				lCurDev = i;
				// report devices present
				printf("Found: %s\n", cDevName);
				// bail after finding first device (simplest method)
				break;
			}
		}
	}

	// if lCurDev is still -1, we found none
	if (lCurDev == -1)
	{
		printf("No Multi DM devices were found.\n");
		return 0;
	}

	// reset the hardware: control signal FRESET is active low
	hr = pIHostDrv->CIUsb_SetControl(lCurDev, CIUsb_CONTROL_DEASSERT_FRESET, &lStatus);
	CheckHr("Failure to deassert MULTI hardware reset control.\n");
	hr = pIHostDrv->CIUsb_SetControl(lCurDev, CIUsb_CONTROL_ASSERT_FRESET,   &lStatus);
	CheckHr("Failure to assert MULTI hardware reset control.\n");

	// assert high voltage enable
	hr = pIHostDrv->CIUsb_SetControl(lCurDev, CIUsb_CONTROL_ASSERT_HV_ENAB,  &lStatus);
	CheckHr("Failure to enable MULTI hardware high voltage enable.\n");

	///////////////////////////////////////////////////////////////////////////////////
	// The following is the start of an example application sequence.
	// Five actuators will be poked using the index and value arrays.

	USHORT	sActData	[NUM_ACTUATORS]  = {0x0000};	// unmapped actuator data for sending to the DM
	USHORT	sMapData	[NUM_ACTUATORS]  = {0x0000};	// mapped actuator data for sending to the DM
	

	// iActIndex:  actuator index (raster=unmapped)
	// sActVAlues: values to set each actuator
	// this was used in original example program:
	#define NUM_TEST_POKES	5
	int		iActIndex	[NUM_TEST_POKES] = {     2,     15,     39,    125,    150}; 
	USHORT	sActVAlues	[NUM_TEST_POKES] = {0x1000, 0x2000, 0x3000, 0x4000, 0x5000};

	if (numFilesToLoad>1) //if loading in multiple files of actuator values
	{
		inputFileExists = exists(argv[1]);
		if (!(inputFileExists))
		{
			cout << "Input file does not exist.\n";
			return 0;
		}
		ifstream listOfFiles;
		char filenames[64] = {0};
		listOfFiles.open(argv[1]);
		for (int i=0; i<numFilesToLoad; i++) //loops through number of files to load
		{
			listOfFiles >> filenames;
			inputFileExists = exists(filenames);
			if (!(inputFileExists))
			{
				cout << "Input file does not exist.\n";
				return 0;
			}
			add_file.open(filenames);
			cout << "Reading file: " << filenames << "\n";
			for (int i=0; i<NUM_ACTUATORS; i++)
			{
				add_file >> fileLine;
				//read in data from file and apply multiplier:
				fromAddFile[i] = strtol(fileLine, NULL, 10) * multiplier;
				actuatorVal = fromFile[i] + fromAddFile[i]; //fromFile array has flattening data
				//sMapDataFile[i] = fromFile[mapFromFile[i]] + fromAddFile[mapFromFile[i]];
				if (actuatorVal<0)
				{
					cout << "Index " << mapFromFile[i] << " or " << i << " to " << actuatorVal << "\n";
					actuatorVal=0;
				}
				if (actuatorVal>65535)
				{
					cout << "Index " << mapFromFile[i] << " or " << i << " to " << actuatorVal << "\n";
					actuatorVal=65535;
				}
				combined[i] = actuatorVal;
			}
			for ( int j=0; j<NUM_ACTUATORS; j++ )
			{
				sMapDataFile[j] = combined[mapFromFile[j]]; // + fromAddFile[mapFromFile[j]];
			}
			add_file.close();
			hr = pIHostDrv->CIUsb_StepFrameData(lCurDev, (UCHAR *) sMapDataFile, NUM_ACTUATORS*sizeof(short), &lStatus);
			CheckHr("Failure to send MULTI frame data.\n");
			if (sleepDuration<0)
			{
				cin.ignore();
			}
			else
				Sleep(sleepDuration); //sleep for sleepDuration milliseconds
		}
		listOfFiles.close();
	}
	else
	{
		hr = pIHostDrv->CIUsb_StepFrameData(lCurDev, (UCHAR *) sMapDataFile, NUM_ACTUATORS*sizeof(short), &lStatus);
		CheckHr("Failure to send MULTI frame data.\n");
		if (sleepDuration<0)
		{
			cin.ignore();
		}
		else
			Sleep(sleepDuration);
	}
	
	/*
	for (int i=0; i<NUM_TEST_POKES; i++)
	{
		if (iActIndex[i] < 0 || iActIndex[i] >= NUM_ACTUATORS)
		{
			printf("Actuator index %d is out of range... skipping to next poke.\n", iActIndex[i]);
			continue;
		}

		// modify the actuator data at index iActIndex[i] with value sActVAlues[i]
		sActData[iActIndex[i]] = sActVAlues[i];

		// send the actuator data to the DM synchronously
		//hr = pIHostDrv->CIUsb_StepFrameData(lCurDev, (UCHAR *) sMapData, NUM_ACTUATORS*sizeof(short), &lStatus);
		hr = pIHostDrv->CIUsb_StepFrameData(lCurDev, (UCHAR *) sMapDataFile, NUM_ACTUATORS*sizeof(short), &lStatus);
		CheckHr("Failure to send MULTI frame data.\n");

		// check for framing errors
		if (lStatus == H_DEVICE_NOT_FOUND)
		{
			printf("Framing error: device not found");
			return 0;
		}
		else
		if (lStatus == H_DEVICE_TIMEOUT)
		{
			printf("Framing error: device timeout");
			return 0;
		}

		// print the loop status
		printf("Ouput index %d: = %d \n", i, sMapDataFile[i]);
		
		

	}
	*/
	//Pause
	Sleep(1000);
	cout << "Done... mirror reset.\n";
	// deassert high voltage enable
	hr = pIHostDrv->CIUsb_SetControl(lCurDev, CIUsb_CONTROL_DEASSERT_HV_ENAB,  &lStatus);
	CheckHr("Failure to deassert MULTI hardware high voltage enable.\n");
	return 0;
}

