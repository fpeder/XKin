/*
 * Copyright (c) 2012, Fabrizio Pedersoli
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *     
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "../include/libgesture.h"

char *infile = NULL;
char *outfile = NULL;

void             usage                 (void);
void             parse_args            (int,char**);
char**           split_string          (char*, int*);


int main (int argc, char *argv[]) 
{
	char **name;
	int i,num=0;
	CvHMM *mo;

	parse_args(argc,argv);

	name = split_string(infile, &num);
	mo = (CvHMM*)malloc(sizeof(CvHMM) * num);

	for (i=0; i<num; i++) {
		mo[i] = cvhmm_from_gesture_proto(name[i]);
		cvhmm_print(mo[i]);
	}
	cvhmm_write(outfile, mo, num);

	free(mo);

	return 0;
}

char **split_string (char *str, int *num)
{
	char **names, *tmp;
	int i=0;
	
	names = (char**)malloc(sizeof(char*) * 10);

	tmp = strtok(str, ":");
	while (tmp != NULL) {
		names[i] = (char*)malloc(sizeof(char)*25);
		strcpy(names[i++], tmp);
		if (i>10) 
			realloc(names, sizeof(char*) * i);
		tmp = strtok(NULL, ":");
	}
	*num=i;
	names[i] = NULL;
	
	return names;
}

void parse_args (int argc, char **argv)
{
	int c;

	opterr = 0;
	while ((c = getopt(argc,argv, "i:o:h")) != -1) {
		switch (c) {
		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'h':
		default:
			usage();
			exit(-1);
		}
	}
	if (infile == NULL || outfile == NULL) {
		usage();
		exit(-1);
	}
}

void usage (void)
{
	printf("usage: trainmodels -i [file1,...,fileN] [-o [file] [-h]\n");
	printf("  -i  sample gesture yml files \":\" seprated\n");
	printf("  -o  output models file\n");
	printf("  -h  show this message\n");
}


		


