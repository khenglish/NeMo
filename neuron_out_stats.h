//
//  neuron_output.h
//  ROSS_TOP
//
//  Created by plaggm on 10/16/15.
//
//

#ifndef __ROSS_TOP__neuron_output__
#define __ROSS_TOP__neuron_output__

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
typedef struct NELE{
    long double timestamp;
    unsigned long nid;
    unsigned long cid;
    unsigned long cbt;
    struct NELE *next;
}neuEvtLog;

typedef struct CsvRow {
	char* data;
	struct CsvRow *next;

}csvRow;



void addEntry(neuEvtLog *newE, neuEvtLog* log, int cbt);

int saveLog(neuEvtLog* log, char* fileName);
int write_csv_dyn(csvRow rows[], char* headers[], int numCols, int numRows, char const *fileName);



void testCSV();
#endif /* defined(__ROSS_TOP__neuron_output__) */

void saveValidationData(int neuronID, int coreID, long double timestamp, int membranePot);